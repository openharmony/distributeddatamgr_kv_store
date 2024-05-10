/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef RELATIONAL_STORE
#include "sqlite_single_ver_relational_storage_executor.h"

#include <algorithm>
#include <optional>

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "data_transformer.h"
#include "db_common.h"
#include "log_table_manager_factory.h"
#include "relational_row_data_impl.h"
#include "res_finalizer.h"
#include "runtime_context.h"
#include "sqlite_meta_executor.h"
#include "sqlite_relational_utils.h"
#include "value_hash_calc.h"

namespace DistributedDB {
namespace {
static constexpr const char *DATAKEY = "DATA_KEY";
static constexpr const char *DEVICE_FIELD = "DEVICE";
static constexpr const char *CLOUD_GID_FIELD = "CLOUD_GID";
static constexpr const char *SHARING_RESOURCE = "SHARING_RESOURCE";
static constexpr const char *FLAG_IS_CLOUD = "FLAG & 0x02 = 0"; // see if 1th bit of a flag is cloud
// set 1th bit of flag to one which is local, clean 5th bit of flag to one which is wait compensated sync
static constexpr const char *SET_FLAG_LOCAL_AND_CLEAN_WAIT_COMPENSATED_SYNC = "(CASE WHEN data_key = -1 and "
    "FLAG & 0x02 = 0x02 THEN (FLAG | 0x02) & (~0x10) & (~0x20) ELSE (FLAG | 0x02 | 0x20) & (~0x10) END)";
static constexpr const char *FLAG_IS_LOGIC_DELETE = "FLAG & 0x08 != 0"; // see if 3th bit of a flag is logic delete
static constexpr const char *DATA_IS_DELETE = "data_key = -1 AND FLAG & 0X08 = 0"; // see if data is delete
static constexpr const int SET_FLAG_ZERO_MASK = ~0x04; // clear 2th bit of flag
static constexpr const int SET_FLAG_ONE_MASK = 0x04; // set 2th bit of flag
static constexpr const int SET_CLOUD_FLAG = ~0x02; // set 1th bit of flag to 0
static constexpr const int DATA_KEY_INDEX = 0;
static constexpr const int TIMESTAMP_INDEX = 3;
static constexpr const int W_TIMESTAMP_INDEX = 4;
static constexpr const int FLAG_INDEX = 5;
static constexpr const int HASH_KEY_INDEX = 6;
static constexpr const int CLOUD_GID_INDEX = 7;
static constexpr const int VERSION_INDEX = 8;
static constexpr const int STATUS_INDEX = 9;

int PermitSelect(void *a, int b, const char *c, const char *d, const char *e, const char *f)
{
    if (b != SQLITE_SELECT && b != SQLITE_READ && b != SQLITE_FUNCTION) {
        return SQLITE_DENY;
    }
    return SQLITE_OK;
}
}
SQLiteSingleVerRelationalStorageExecutor::SQLiteSingleVerRelationalStorageExecutor(sqlite3 *dbHandle, bool writable,
    DistributedTableMode mode)
    : SQLiteStorageExecutor(dbHandle, writable, false), mode_(mode), isLogicDelete_(false),
      assetLoader_(nullptr), putDataMode_(PutDataMode::SYNC), markFlagOption_(MarkFlagOption::DEFAULT)
{
    bindCloudFieldFuncMap_[TYPE_INDEX<int64_t>] = &CloudStorageUtils::BindInt64;
    bindCloudFieldFuncMap_[TYPE_INDEX<bool>] = &CloudStorageUtils::BindBool;
    bindCloudFieldFuncMap_[TYPE_INDEX<double>] = &CloudStorageUtils::BindDouble;
    bindCloudFieldFuncMap_[TYPE_INDEX<std::string>] = &CloudStorageUtils::BindText;
    bindCloudFieldFuncMap_[TYPE_INDEX<Bytes>] = &CloudStorageUtils::BindBlob;
    bindCloudFieldFuncMap_[TYPE_INDEX<Asset>] = &CloudStorageUtils::BindAsset;
    bindCloudFieldFuncMap_[TYPE_INDEX<Assets>] = &CloudStorageUtils::BindAsset;
}

int CheckTableConstraint(const TableInfo &table, DistributedTableMode mode, TableSyncType syncType)
{
    std::string trimedSql = DBCommon::TrimSpace(table.GetCreateTableSql());
    if (DBCommon::HasConstraint(trimedSql, "WITHOUT ROWID", " ),", " ,;")) {
        LOGE("[CreateDistributedTable] Not support create distributed table without rowid.");
        return -E_NOT_SUPPORT;
    }
    std::vector<FieldInfo> fieldInfos = table.GetFieldInfos();
    for (const auto &field : fieldInfos) {
        if (DBCommon::CaseInsensitiveCompare(field.GetFieldName(), std::string(DBConstant::SQLITE_INNER_ROWID))) {
            LOGE("[CreateDistributedTable] Not support create distributed table with _rowid_ column.");
            return -E_NOT_SUPPORT;
        }
    }

    if (mode == DistributedTableMode::COLLABORATION || syncType == CLOUD_COOPERATION) {
        if (DBCommon::HasConstraint(trimedSql, "CHECK", " ,", " (")) {
            LOGE("[CreateDistributedTable] Not support create distributed table with 'CHECK' constraint.");
            return -E_NOT_SUPPORT;
        }

        if (DBCommon::HasConstraint(trimedSql, "ON CONFLICT", " )", " ")) {
            LOGE("[CreateDistributedTable] Not support create distributed table with 'ON CONFLICT' constraint.");
            return -E_NOT_SUPPORT;
        }

        if (mode == DistributedTableMode::COLLABORATION) {
            if (DBCommon::HasConstraint(trimedSql, "REFERENCES", " )", " ")) {
                LOGE("[CreateDistributedTable] Not support create distributed table with 'FOREIGN KEY' constraint.");
                return -E_NOT_SUPPORT;
            }
        }

        if (syncType == CLOUD_COOPERATION) {
            int errCode = CloudStorageUtils::ConstraintsCheckForCloud(table, trimedSql);
            if (errCode != E_OK) {
                LOGE("ConstraintsCheckForCloud failed, errCode = %d", errCode);
                return errCode;
            }
        }
    }

    if (mode == DistributedTableMode::SPLIT_BY_DEVICE && syncType == DEVICE_COOPERATION) {
        if (table.GetPrimaryKey().size() > 1) {
            LOGE("[CreateDistributedTable] Not support create distributed table with composite primary keys.");
            return -E_NOT_SUPPORT;
        }
    }

    return E_OK;
}

namespace {
int GetExistedDataTimeOffset(sqlite3 *db, const std::string &tableName, bool isMem, int64_t &timeOffset)
{
    std::string sql = "SELECT get_sys_time(0) - max(" + std::string(DBConstant::SQLITE_INNER_ROWID) + ") - 1 FROM '" +
        tableName + "';";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, isMem);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        timeOffset = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
        errCode = E_OK;
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}
}

int SQLiteSingleVerRelationalStorageExecutor::GeneLogInfoForExistedData(sqlite3 *db, const std::string &tableName,
    const std::string &calPrimaryKeyHash, TableInfo &tableInfo)
{
    int64_t timeOffset = 0;
    int errCode = GetExistedDataTimeOffset(db, tableName, isMemDb_, timeOffset);
    if (errCode != E_OK) {
        return errCode;
    }
    std::string timeOffsetStr = std::to_string(timeOffset);
    std::string logTable = DBConstant::RELATIONAL_PREFIX + tableName + "_log";
    std::string sql = "INSERT OR REPLACE INTO " + logTable + " SELECT " + std::string(DBConstant::SQLITE_INNER_ROWID) +
        ", '', '', " + timeOffsetStr + " + " + std::string(DBConstant::SQLITE_INNER_ROWID) + ", " +
        timeOffsetStr + " + " + std::string(DBConstant::SQLITE_INNER_ROWID) + ", 0x02|0x20, " +
        calPrimaryKeyHash + ", '', ";
    if (tableInfo.GetTableSyncType() == TableSyncType::DEVICE_COOPERATION) {
        sql += "'', ''";
    } else {
        sql += tableInfo.GetTrackerTable().GetExtendName().empty() ? "''" : tableInfo.GetTrackerTable().GetExtendName();
        sql += ", case when (SELECT count(1)<>0 FROM " + logTable + ")" +
            " then ((SELECT CASE WHEN MAX(cursor) IS NULL THEN 0 ELSE MAX(cursor) END FROM " + logTable + ") + " +
            std::string(DBConstant::SQLITE_INNER_ROWID) +
            ") ELSE " + std::string(DBConstant::SQLITE_INNER_ROWID) + " end";
    }
    sql += ", '', '', 0";
    sql += " FROM '" + tableName + "' AS a WHERE 1=1;";
    return SQLiteUtils::ExecuteRawSQL(db, sql);
}

int SQLiteSingleVerRelationalStorageExecutor::CreateDistributedTable(DistributedTableMode mode, bool isUpgraded,
    const std::string &identity, TableInfo &table, TableSyncType syncType)
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }

    const std::string tableName = table.GetTableName();
    int errCode = SQLiteUtils::AnalysisSchema(dbHandle_, tableName, table);
    if (errCode != E_OK) {
        LOGE("[CreateDistributedTable] analysis table schema failed. %d", errCode);
        return errCode;
    }

    if (mode == DistributedTableMode::SPLIT_BY_DEVICE && !isUpgraded) {
        bool isEmpty = false;
        errCode = SQLiteUtils::CheckTableEmpty(dbHandle_, tableName, isEmpty);
        if (errCode != E_OK) {
            LOGE("[CreateDistributedTable] check table empty failed. error=%d", errCode);
            return -E_NOT_SUPPORT;
        }
        if (!isEmpty) {
            LOGW("[CreateDistributedTable] generate %.3s log for existed data, table type %d",
                DBCommon::TransferStringToHex(DBCommon::TransferHashString(tableName)).c_str(),
                static_cast<int>(syncType));
        }
    }

    errCode = CheckTableConstraint(table, mode, syncType);
    if (errCode != E_OK) {
        LOGE("[CreateDistributedTable] check table constraint failed.");
        return errCode;
    }

    // create log table
    auto tableManager = LogTableManagerFactory::GetTableManager(mode, syncType);
    errCode = tableManager->CreateRelationalLogTable(dbHandle_, table);
    if (errCode != E_OK) {
        LOGE("[CreateDistributedTable] create log table failed");
        return errCode;
    }

    if (!isUpgraded) {
        std::string calPrimaryKeyHash = tableManager->CalcPrimaryKeyHash("a.", table, identity);
        errCode = GeneLogInfoForExistedData(dbHandle_, tableName, calPrimaryKeyHash, table);
    }
    if (errCode != E_OK) {
        return errCode;
    }

    // add trigger
    errCode = tableManager->AddRelationalLogTableTrigger(dbHandle_, table, identity);
    if (errCode != E_OK) {
        LOGE("[CreateDistributedTable] Add relational log table trigger failed.");
        return errCode;
    }
    return SetLogTriggerStatus(true);
}

int SQLiteSingleVerRelationalStorageExecutor::UpgradeDistributedTable(const std::string &tableName,
    DistributedTableMode mode, bool &schemaChanged, RelationalSchemaObject &schema, TableSyncType syncType)
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }
    TableInfo newTableInfo;
    int errCode = SQLiteUtils::AnalysisSchema(dbHandle_, tableName, newTableInfo);
    if (errCode != E_OK) {
        LOGE("[UpgradeDistributedTable] analysis table schema failed. %d", errCode);
        return errCode;
    }

    if (CheckTableConstraint(newTableInfo, mode, syncType)) {
        LOGE("[UpgradeDistributedTable] Not support create distributed table when violate constraints.");
        return -E_NOT_SUPPORT;
    }

    // new table should has same or compatible upgrade
    TableInfo tableInfo = schema.GetTable(tableName);
    errCode = tableInfo.CompareWithTable(newTableInfo, schema.GetSchemaVersion());
    if (errCode == -E_RELATIONAL_TABLE_INCOMPATIBLE) {
        LOGE("[UpgradeDistributedTable] Not support with incompatible upgrade.");
        return -E_SCHEMA_MISMATCH;
    } else if (errCode == -E_RELATIONAL_TABLE_EQUAL) {
        LOGD("[UpgradeDistributedTable] schema has not changed.");
        // update table if tableName changed
        schema.RemoveRelationalTable(tableName);
        tableInfo.SetTableName(tableName);
        schema.AddRelationalTable(tableInfo);
        return E_OK;
    }

    schemaChanged = true;
    errCode = AlterAuxTableForUpgrade(tableInfo, newTableInfo);
    if (errCode != E_OK) {
        LOGE("[UpgradeDistributedTable] Alter aux table for upgrade failed. %d", errCode);
    }

    schema.AddRelationalTable(newTableInfo);
    return errCode;
}

namespace {
int GetDeviceTableName(sqlite3 *handle, const std::string &tableName, const std::string &device,
    std::vector<std::string> &deviceTables)
{
    if (device.empty() && tableName.empty()) { // device and table name should not both be empty
        return -E_INVALID_ARGS;
    }
    std::string devicePattern = device.empty() ? "%" : device;
    std::string tablePattern = tableName.empty() ? "%" : tableName;
    std::string deviceTableName = DBConstant::RELATIONAL_PREFIX + tablePattern + "_" + devicePattern;

    const std::string checkSql = "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE '" +
        deviceTableName + "';";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(handle, checkSql, stmt);
    if (errCode != E_OK) {
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(stmt, false);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            LOGE("Get table name failed. %d", errCode);
            break;
        }
        std::string realTableName;
        errCode = SQLiteUtils::GetColumnTextValue(stmt, 0, realTableName); // 0: table name result column index
        if (errCode != E_OK || realTableName.empty()) { // sqlite might return a row with NULL
            continue;
        }
        if (realTableName.rfind("_log") == (realTableName.length() - 4)) { // 4:suffix length of "_log"
            continue;
        }
        deviceTables.emplace_back(realTableName);
    } while (true);

    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

std::vector<FieldInfo> GetUpgradeFields(const TableInfo &oldTableInfo, const TableInfo &newTableInfo)
{
    std::vector<FieldInfo> fields;
    auto itOld = oldTableInfo.GetFields().begin();
    auto itNew = newTableInfo.GetFields().begin();
    for (; itNew != newTableInfo.GetFields().end(); itNew++) {
        if (itOld == oldTableInfo.GetFields().end() || itOld->first != itNew->first) {
            fields.emplace_back(itNew->second);
            continue;
        }
        itOld++;
    }
    return fields;
}

int UpgradeFields(sqlite3 *db, const std::vector<std::string> &tables, std::vector<FieldInfo> &fields)
{
    if (db == nullptr) {
        return -E_INVALID_ARGS;
    }

    std::sort(fields.begin(), fields.end(), [] (const FieldInfo &a, const FieldInfo &b) {
        return a.GetColumnId()< b.GetColumnId();
    });
    int errCode = E_OK;
    for (const auto &table : tables) {
        for (const auto &field : fields) {
            std::string alterSql = "ALTER TABLE " + table + " ADD '" + field.GetFieldName() + "' ";
            alterSql += "'" + field.GetDataType() + "'";
            alterSql += field.IsNotNull() ? " NOT NULL" : "";
            alterSql += field.HasDefaultValue() ? " DEFAULT " + field.GetDefaultValue() : "";
            alterSql += ";";
            errCode = SQLiteUtils::ExecuteRawSQL(db, alterSql);
            if (errCode != E_OK) {
                LOGE("Alter table failed. %d", errCode);
                break;
            }
        }
    }
    return errCode;
}

IndexInfoMap GetChangedIndexes(const TableInfo &oldTableInfo, const TableInfo &newTableInfo)
{
    IndexInfoMap indexes;
    auto itOld = oldTableInfo.GetIndexDefine().begin();
    auto itNew = newTableInfo.GetIndexDefine().begin();
    auto itOldEnd = oldTableInfo.GetIndexDefine().end();
    auto itNewEnd = newTableInfo.GetIndexDefine().end();

    while (itOld != itOldEnd && itNew != itNewEnd) {
        if (itOld->first == itNew->first) {
            if (itOld->second != itNew->second) {
                indexes.insert({itNew->first, itNew->second});
            }
            itOld++;
            itNew++;
        } else if (itOld->first < itNew->first) {
            indexes.insert({itOld->first, {}});
            itOld++;
        } else {
            indexes.insert({itNew->first, itNew->second});
            itNew++;
        }
    }

    while (itOld != itOldEnd) {
        indexes.insert({itOld->first, {}});
        itOld++;
    }

    while (itNew != itNewEnd) {
        indexes.insert({itNew->first, itNew->second});
        itNew++;
    }

    return indexes;
}

int UpgradeIndexes(sqlite3 *db, const std::vector<std::string> &tables, const IndexInfoMap &indexes)
{
    if (db == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    for (const auto &table : tables) {
        for (const auto &index : indexes) {
            if (index.first.empty()) {
                continue;
            }
            std::string realIndexName = table + "_" + index.first;
            std::string deleteIndexSql = "DROP INDEX IF EXISTS " + realIndexName;
            errCode = SQLiteUtils::ExecuteRawSQL(db, deleteIndexSql);
            if (errCode != E_OK) {
                LOGE("Drop index failed. %d", errCode);
                return errCode;
            }

            if (index.second.empty()) { // empty means drop index only
                continue;
            }

            auto it = index.second.begin();
            std::string indexDefine = *it++;
            while (it != index.second.end()) {
                indexDefine += ", " + *it++;
            }
            std::string createIndexSql = "CREATE INDEX IF NOT EXISTS " + realIndexName + " ON " + table +
                "(" + indexDefine + ");";
            errCode = SQLiteUtils::ExecuteRawSQL(db, createIndexSql);
            if (errCode != E_OK) {
                LOGE("Create index failed. %d", errCode);
                break;
            }
        }
    }
    return errCode;
}
}

int SQLiteSingleVerRelationalStorageExecutor::AlterAuxTableForUpgrade(const TableInfo &oldTableInfo,
    const TableInfo &newTableInfo)
{
    std::vector<FieldInfo> upgradeFields = GetUpgradeFields(oldTableInfo, newTableInfo);
    IndexInfoMap upgradeIndexes = GetChangedIndexes(oldTableInfo, newTableInfo);
    std::vector<std::string> deviceTables;
    int errCode = GetDeviceTableName(dbHandle_, oldTableInfo.GetTableName(), {}, deviceTables);
    if (errCode != E_OK) {
        LOGE("Get device table name for alter table failed. %d", errCode);
        return errCode;
    }

    LOGD("Begin to alter table: upgrade fields[%zu], indexes[%zu], deviceTable[%zu]", upgradeFields.size(),
        upgradeIndexes.size(), deviceTables.size());
    errCode = UpgradeFields(dbHandle_, deviceTables, upgradeFields);
    if (errCode != E_OK) {
        LOGE("upgrade fields failed. %d", errCode);
        return errCode;
    }

    errCode = UpgradeIndexes(dbHandle_, deviceTables, upgradeIndexes);
    if (errCode != E_OK) {
        LOGE("upgrade indexes failed. %d", errCode);
    }

    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::StartTransaction(TransactType type)
{
    if (dbHandle_ == nullptr) {
        LOGE("Begin transaction failed, dbHandle is null.");
        return -E_INVALID_DB;
    }
    int errCode = SQLiteUtils::BeginTransaction(dbHandle_, type);
    if (errCode != E_OK) {
        LOGE("Begin transaction failed, errCode = %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::Commit()
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }

    return SQLiteUtils::CommitTransaction(dbHandle_);
}

int SQLiteSingleVerRelationalStorageExecutor::Rollback()
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = SQLiteUtils::RollbackTransaction(dbHandle_);
    if (errCode != E_OK) {
        LOGE("sqlite single ver storage executor rollback fail! errCode = [%d]", errCode);
    }
    return errCode;
}

void SQLiteSingleVerRelationalStorageExecutor::SetTableInfo(const TableInfo &tableInfo)
{
    table_ = tableInfo;
}

static int GetLogData(sqlite3_stmt *logStatement, LogInfo &logInfo)
{
    logInfo.dataKey = sqlite3_column_int64(logStatement, 0);  // 0 means dataKey index

    std::vector<uint8_t> dev;
    int errCode = SQLiteUtils::GetColumnBlobValue(logStatement, 1, dev);  // 1 means dev index
    if (errCode != E_OK) {
        return errCode;
    }
    logInfo.device = std::string(dev.begin(), dev.end());

    std::vector<uint8_t> oriDev;
    errCode = SQLiteUtils::GetColumnBlobValue(logStatement, 2, oriDev);  // 2 means ori_dev index
    if (errCode != E_OK) {
        return errCode;
    }
    logInfo.originDev = std::string(oriDev.begin(), oriDev.end());
    logInfo.timestamp = static_cast<uint64_t>(sqlite3_column_int64(logStatement, 3));  // 3 means timestamp index
    logInfo.wTimestamp = static_cast<uint64_t>(sqlite3_column_int64(logStatement, 4));  // 4 means w_timestamp index
    logInfo.flag = static_cast<uint64_t>(sqlite3_column_int64(logStatement, 5));  // 5 means flag index
    logInfo.flag &= (~DataItem::LOCAL_FLAG);
    logInfo.flag &= (~DataItem::UPDATE_FLAG);
    return SQLiteUtils::GetColumnBlobValue(logStatement, 6, logInfo.hashKey);  // 6 means hashKey index
}

namespace {
void GetCloudLog(sqlite3_stmt *logStatement, VBucket &logInfo, uint32_t &totalSize, bool isShared)
{
    logInfo.insert_or_assign(CloudDbConstant::MODIFY_FIELD,
        static_cast<int64_t>(sqlite3_column_int64(logStatement, TIMESTAMP_INDEX)));
    logInfo.insert_or_assign(CloudDbConstant::CREATE_FIELD,
        static_cast<int64_t>(sqlite3_column_int64(logStatement, W_TIMESTAMP_INDEX)));
    totalSize += sizeof(int64_t) + sizeof(int64_t);
    if (sqlite3_column_text(logStatement, CLOUD_GID_INDEX) != nullptr) {
        std::string cloudGid = reinterpret_cast<const std::string::value_type *>(
            sqlite3_column_text(logStatement, CLOUD_GID_INDEX));
        if (!cloudGid.empty()) {
            logInfo.insert_or_assign(CloudDbConstant::GID_FIELD, cloudGid);
            totalSize += cloudGid.size();
        }
    }
    std::string version;
    SQLiteUtils::GetColumnTextValue(logStatement, VERSION_INDEX, version);
    logInfo.insert_or_assign(CloudDbConstant::VERSION_FIELD, version);
    totalSize += version.size();
}

void GetCloudExtraLog(sqlite3_stmt *logStatement, VBucket &flags)
{
    flags.insert_or_assign(CloudDbConstant::ROWID,
        static_cast<int64_t>(sqlite3_column_int64(logStatement, DATA_KEY_INDEX)));
    flags.insert_or_assign(CloudDbConstant::TIMESTAMP,
        static_cast<int64_t>(sqlite3_column_int64(logStatement, TIMESTAMP_INDEX)));
    flags.insert_or_assign(CloudDbConstant::FLAG,
        static_cast<int64_t>(sqlite3_column_int64(logStatement, FLAG_INDEX)));
    Bytes hashKey;
    (void)SQLiteUtils::GetColumnBlobValue(logStatement, HASH_KEY_INDEX, hashKey);
    flags.insert_or_assign(CloudDbConstant::HASH_KEY, hashKey);
    flags.insert_or_assign(CloudDbConstant::STATUS,
        static_cast<int64_t>(sqlite3_column_int(logStatement, STATUS_INDEX)));
}

void GetCloudGid(sqlite3_stmt *logStatement, std::vector<std::string> &cloudGid)
{
    if (sqlite3_column_text(logStatement, CLOUD_GID_INDEX) == nullptr) {
        return;
    }
    std::string gid = reinterpret_cast<const std::string::value_type *>(
        sqlite3_column_text(logStatement, CLOUD_GID_INDEX));
    if (gid.empty()) {
        LOGW("[Relational] Get cloud gid is null.");
        return;
    }
    cloudGid.emplace_back(gid);
}
}

static size_t GetDataItemSerialSize(DataItem &item, size_t appendLen)
{
    // timestamp and local flag: 3 * uint64_t, version(uint32_t), key, value, origin dev and the padding size.
    // the size would not be very large.
    static const size_t maxOrigDevLength = 40;
    size_t devLength = std::max(maxOrigDevLength, item.origDev.size());
    size_t dataSize = (Parcel::GetUInt64Len() * 3 + Parcel::GetUInt32Len() + Parcel::GetVectorCharLen(item.key) +
        Parcel::GetVectorCharLen(item.value) + devLength + appendLen);
    return dataSize;
}

int SQLiteSingleVerRelationalStorageExecutor::GetKvData(const Key &key, Value &value) const
{
    static const std::string SELECT_META_VALUE_SQL = "SELECT value FROM " + DBConstant::RELATIONAL_PREFIX +
        "metadata WHERE key=?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, SELECT_META_VALUE_SQL, statement);
    if (errCode != E_OK) {
        goto END;
    }

    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, false); // first arg.
    if (errCode != E_OK) {
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = -E_NOT_FOUND;
        goto END;
    } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        goto END;
    }

    errCode = SQLiteUtils::GetColumnBlobValue(statement, 0, value); // only one result.
    END:
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::PutKvData(const Key &key, const Value &value) const
{
    static const std::string INSERT_META_SQL = "INSERT OR REPLACE INTO " + DBConstant::RELATIONAL_PREFIX +
        "metadata VALUES(?,?);";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, INSERT_META_SQL, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, false);  // 1 means key index
    if (errCode != E_OK) {
        LOGE("[SingleVerExe][BindPutKv]Bind key error:%d", errCode);
        goto ERROR;
    }

    errCode = SQLiteUtils::BindBlobToStatement(statement, 2, value, true);  // 2 means value index
    if (errCode != E_OK) {
        LOGE("[SingleVerExe][BindPutKv]Bind value error:%d", errCode);
        goto ERROR;
    }
    errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
ERROR:
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteMetaData(const std::vector<Key> &keys) const
{
    static const std::string REMOVE_META_VALUE_SQL = "DELETE FROM " + DBConstant::RELATIONAL_PREFIX +
        "metadata WHERE key=?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, REMOVE_META_VALUE_SQL, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    for (const auto &key : keys) {
        errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, false); // first arg.
        if (errCode != E_OK) {
            break;
        }

        errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
        if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            break;
        }
        errCode = E_OK;
        SQLiteUtils::ResetStatement(statement, false, errCode);
    }
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return CheckCorruptedStatus(errCode);
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteMetaDataByPrefixKey(const Key &keyPrefix) const
{
    static const std::string REMOVE_META_VALUE_BY_KEY_PREFIX_SQL = "DELETE FROM " + DBConstant::RELATIONAL_PREFIX +
        "metadata WHERE key>=? AND key<=?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, REMOVE_META_VALUE_BY_KEY_PREFIX_SQL, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SQLiteUtils::BindPrefixKey(statement, 1, keyPrefix); // 1 is first arg.
    if (errCode == E_OK) {
        errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
        }
    }
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return CheckCorruptedStatus(errCode);
}

int SQLiteSingleVerRelationalStorageExecutor::GetAllMetaKeys(std::vector<Key> &keys) const
{
    static const std::string SELECT_ALL_META_KEYS = "SELECT key FROM " + DBConstant::RELATIONAL_PREFIX + "metadata;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, SELECT_ALL_META_KEYS, statement);
    if (errCode != E_OK) {
        LOGE("[Relational][GetAllKey] Get statement failed:%d", errCode);
        return errCode;
    }
    errCode = SqliteMetaExecutor::GetAllKeys(statement, isMemDb_, keys);
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetLogInfoPre(sqlite3_stmt *queryStmt, const DataItem &dataItem,
    LogInfo &logInfoGet)
{
    if (queryStmt == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = SQLiteUtils::BindBlobToStatement(queryStmt, 1, dataItem.hashKey);  // 1 means hashkey index.
    if (errCode != E_OK) {
        return errCode;
    }
    if (mode_ != DistributedTableMode::COLLABORATION) {
        errCode = SQLiteUtils::BindTextToStatement(queryStmt, 2, dataItem.dev);  // 2 means device index.
        if (errCode != E_OK) {
            return errCode;
        }
    }

    errCode = SQLiteUtils::StepWithRetry(queryStmt, isMemDb_);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        errCode = -E_NOT_FOUND;
    } else {
        errCode = GetLogData(queryStmt, logInfoGet);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::SaveSyncLog(sqlite3_stmt *statement, sqlite3_stmt *queryStmt,
    const DataItem &dataItem, int64_t rowid)
{
    LogInfo logInfoGet;
    int errCode = GetLogInfoPre(queryStmt, dataItem, logInfoGet);
    LogInfo logInfoBind;
    logInfoBind.hashKey = dataItem.hashKey;
    logInfoBind.device = dataItem.dev;
    logInfoBind.timestamp = dataItem.timestamp;
    logInfoBind.flag = dataItem.flag;

    if (errCode == -E_NOT_FOUND) { // insert
        logInfoBind.wTimestamp = dataItem.writeTimestamp;
        logInfoBind.originDev = dataItem.dev;
    } else if (errCode == E_OK) { // update
        logInfoBind.wTimestamp = logInfoGet.wTimestamp;
        logInfoBind.originDev = logInfoGet.originDev;
    } else {
        return errCode;
    }

    // bind
    SQLiteUtils::BindInt64ToStatement(statement, 1, rowid);  // 1 means dataKey index
    std::vector<uint8_t> originDev(logInfoBind.originDev.begin(), logInfoBind.originDev.end());
    SQLiteUtils::BindBlobToStatement(statement, 2, originDev);  // 2 means ori_dev index
    SQLiteUtils::BindInt64ToStatement(statement, 3, logInfoBind.timestamp);  // 3 means timestamp index
    SQLiteUtils::BindInt64ToStatement(statement, 4, logInfoBind.wTimestamp);  // 4 means w_timestamp index
    SQLiteUtils::BindInt64ToStatement(statement, 5, logInfoBind.flag);  // 5 means flag index
    SQLiteUtils::BindBlobToStatement(statement, 6, logInfoBind.hashKey);  // 6 means hashKey index
    errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        return E_OK;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteSyncDataItem(const DataItem &dataItem,
    RelationalSyncDataInserter &inserter, sqlite3_stmt *&stmt)
{
    if (stmt == nullptr) {
        int errCode = inserter.GetDeleteSyncDataStmt(dbHandle_, stmt);
        if (errCode != E_OK) {
            LOGE("[DeleteSyncDataItem] Get statement fail!, errCode:%d", errCode);
            return errCode;
        }
    }

    int errCode = SQLiteUtils::BindBlobToStatement(stmt, 1, dataItem.hashKey); // 1 means hash_key index
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }
    if (mode_ != DistributedTableMode::COLLABORATION) {
        errCode = SQLiteUtils::BindTextToStatement(stmt, 2, dataItem.dev); // 2 means device index
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(stmt, true, errCode);
            return errCode;
        }
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
    SQLiteUtils::ResetStatement(stmt, false, errCode);  // Finalize outside.
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::SaveSyncDataItem(const DataItem &dataItem, SaveSyncDataStmt &saveStmt,
    RelationalSyncDataInserter &inserter, int64_t &rowid)
{
    if ((dataItem.flag & DataItem::DELETE_FLAG) != 0) {
        return DeleteSyncDataItem(dataItem, inserter, saveStmt.rmDataStmt);
    }
    if ((mode_ == DistributedTableMode::COLLABORATION && inserter.GetLocalTable().GetIdentifyKey().size() == 1u &&
        inserter.GetLocalTable().GetIdentifyKey().at(0) == "rowid") ||
        (mode_ == DistributedTableMode::SPLIT_BY_DEVICE && inserter.GetLocalTable().GetPrimaryKey().size() == 1u &&
        inserter.GetLocalTable().GetPrimaryKey().at(0) == "rowid") ||
        inserter.GetLocalTable().GetAutoIncrement()) {  // No primary key of auto increment
        int errCode = DeleteSyncDataItem(dataItem, inserter, saveStmt.rmDataStmt);
        if (errCode != E_OK) {
            LOGE("Delete no pk data before insert failed, errCode=%d.", errCode);
            return errCode;
        }
    }

    int errCode = inserter.BindInsertStatement(saveStmt.saveDataStmt, dataItem);
    if (errCode != E_OK) {
        LOGE("Bind data failed, errCode=%d.", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(saveStmt.saveDataStmt, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        rowid = SQLiteUtils::GetLastRowId(dbHandle_);
        errCode = E_OK;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteSyncLog(const DataItem &dataItem,
    RelationalSyncDataInserter &inserter, sqlite3_stmt *&stmt)
{
    if (stmt == nullptr) {
        int errCode = inserter.GetDeleteLogStmt(dbHandle_, stmt);
        if (errCode != E_OK) {
            LOGE("[DeleteSyncLog] Get statement fail!");
            return errCode;
        }
    }

    int errCode = SQLiteUtils::BindBlobToStatement(stmt, 1, dataItem.hashKey); // 1 means hashkey index
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }
    if (mode_ != DistributedTableMode::COLLABORATION) {
        errCode = SQLiteUtils::BindTextToStatement(stmt, 2, dataItem.dev); // 2 means device index
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(stmt, true, errCode);
            return errCode;
        }
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
    SQLiteUtils::ResetStatement(stmt, false, errCode);  // Finalize outside.
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::ProcessMissQueryData(const DataItem &item,
    RelationalSyncDataInserter &inserter, sqlite3_stmt *&rmDataStmt, sqlite3_stmt *&rmLogStmt)
{
    int errCode = DeleteSyncDataItem(item, inserter, rmDataStmt);
    if (errCode != E_OK) {
        return errCode;
    }
    return DeleteSyncLog(item, inserter, rmLogStmt);
}

int SQLiteSingleVerRelationalStorageExecutor::GetSyncDataPre(const DataItem &dataItem, sqlite3_stmt *queryStmt,
    DataItem &itemGet)
{
    LogInfo logInfoGet;
    int errCode = GetLogInfoPre(queryStmt, dataItem, logInfoGet);
    itemGet.timestamp = logInfoGet.timestamp;
    SQLiteUtils::ResetStatement(queryStmt, false, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CheckDataConflictDefeated(const DataItem &dataItem,
    sqlite3_stmt *queryStmt, bool &isDefeated)
{
    if ((dataItem.flag & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) != DataItem::REMOTE_DEVICE_DATA_MISS_QUERY &&
        mode_ == DistributedTableMode::SPLIT_BY_DEVICE) {
        isDefeated = false; // no need to solve conflict except miss query data
        return E_OK;
    }

    DataItem itemGet;
    int errCode = GetSyncDataPre(dataItem, queryStmt, itemGet);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("Failed to get raw data. %d", errCode);
        return errCode;
    }
    isDefeated = (dataItem.timestamp <= itemGet.timestamp); // defeated if item timestamp is earlier then raw data
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::SaveSyncDataItem(RelationalSyncDataInserter &inserter,
    SaveSyncDataStmt &saveStmt, DataItem &item)
{
    bool isDefeated = false;
    int errCode = CheckDataConflictDefeated(item, saveStmt.queryStmt, isDefeated);
    if (errCode != E_OK) {
        LOGE("check data conflict failed. %d", errCode);
        return errCode;
    }

    if (isDefeated) {
        LOGD("Data was defeated.");
        return E_OK;
    }
    if ((item.flag & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) != 0) {
        return ProcessMissQueryData(item, inserter, saveStmt.rmDataStmt, saveStmt.rmLogStmt);
    }
    int64_t rowid = -1;
    errCode = SaveSyncDataItem(item, saveStmt, inserter, rowid);
    if (errCode == E_OK || errCode == -E_NOT_FOUND) {
        errCode = SaveSyncLog(saveStmt.saveLogStmt, saveStmt.queryStmt, item, rowid);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::SaveSyncDataItems(RelationalSyncDataInserter &inserter)
{
    SaveSyncDataStmt saveStmt;
    int errCode = inserter.PrepareStatement(dbHandle_, saveStmt);
    if (errCode != E_OK) {
        LOGE("Prepare insert sync data statement failed.");
        return errCode;
    }

    errCode = inserter.Iterate([this, &saveStmt, &inserter] (DataItem &item) -> int {
        if (item.neglect) { // Do not save this record if it is neglected
            return E_OK;
        }
        int errCode = SaveSyncDataItem(inserter, saveStmt, item);
        if (errCode != E_OK) {
            LOGE("save sync data item failed. err=%d", errCode);
            return errCode;
        }
        // Need not reset rmDataStmt and rmLogStmt here.
        return saveStmt.ResetStatements(false);
    });

    int ret = saveStmt.ResetStatements(true);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteSingleVerRelationalStorageExecutor::SaveSyncItems(RelationalSyncDataInserter &inserter, bool useTrans)
{
    if (useTrans) {
        int errCode = StartTransaction(TransactType::IMMEDIATE);
        if (errCode != E_OK) {
            return errCode;
        }
    }

    int errCode = SetLogTriggerStatus(false);
    if (errCode != E_OK) {
        goto END;
    }

    errCode = SaveSyncDataItems(inserter);
    if (errCode != E_OK) {
        LOGE("Save sync data items failed. errCode=%d", errCode);
        goto END;
    }

    errCode = SetLogTriggerStatus(true);
END:
    if (useTrans) {
        if (errCode == E_OK) {
            errCode = Commit();
        } else {
            (void)Rollback(); // Keep the error code of the first scene
        }
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetDataItemForSync(sqlite3_stmt *stmt, DataItem &dataItem,
    bool isGettingDeletedData) const
{
    RowDataWithLog data;
    int errCode = GetLogData(stmt, data.logInfo);
    if (errCode != E_OK) {
        LOGE("relational data value transfer to kv fail");
        return errCode;
    }

    if (!isGettingDeletedData) {
        for (size_t cid = 0; cid < table_.GetFields().size(); ++cid) {
            DataValue value;
            errCode = SQLiteRelationalUtils::GetDataValueByType(stmt, cid + DBConstant::RELATIONAL_LOG_TABLE_FIELD_NUM,
                value);
            if (errCode != E_OK) {
                return errCode;
            }
            data.rowData.push_back(std::move(value)); // sorted by cid
        }
    }

    errCode = DataTransformer::SerializeDataItem(data,
        isGettingDeletedData ? std::vector<FieldInfo>() : table_.GetFieldInfos(), dataItem);
    if (errCode != E_OK) {
        LOGE("relational data value transfer to kv fail");
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetMissQueryData(sqlite3_stmt *fullStmt, DataItem &item)
{
    int errCode = GetDataItemForSync(fullStmt, item, false);
    if (errCode != E_OK) {
        return errCode;
    }
    item.value = {};
    item.flag |= DataItem::REMOTE_DEVICE_DATA_MISS_QUERY;
    return E_OK;
}

namespace {
int StepNext(bool isMemDB, sqlite3_stmt *stmt, Timestamp &timestamp)
{
    if (stmt == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = SQLiteUtils::StepWithRetry(stmt, isMemDB);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        timestamp = INT64_MAX;
        errCode = E_OK;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        timestamp = static_cast<uint64_t>(sqlite3_column_int64(stmt, 3));  // 3 means timestamp index
        errCode = E_OK;
    }
    return errCode;
}

int AppendData(const DataSizeSpecInfo &sizeInfo, size_t appendLength, size_t &overLongSize, size_t &dataTotalSize,
    std::vector<DataItem> &dataItems, DataItem &&item)
{
    // If one record is over 4M, ignore it.
    if (item.value.size() > DBConstant::MAX_VALUE_SIZE) {
        overLongSize++;
    } else {
        // If dataTotalSize value is bigger than blockSize value , reserve the surplus data item.
        dataTotalSize += GetDataItemSerialSize(item, appendLength);
        if ((dataTotalSize > sizeInfo.blockSize && !dataItems.empty()) || dataItems.size() >= sizeInfo.packetSize) {
            return -E_UNFINISHED;
        } else {
            dataItems.push_back(item);
        }
    }
    return E_OK;
}
}

int SQLiteSingleVerRelationalStorageExecutor::GetQueryDataAndStepNext(bool isFirstTime, bool isGettingDeletedData,
    sqlite3_stmt *queryStmt, DataItem &item, Timestamp &queryTime)
{
    if (!isFirstTime) { // For the first time, never step before, can get nothing
        int errCode = GetDataItemForSync(queryStmt, item, isGettingDeletedData);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return StepNext(isMemDb_, queryStmt, queryTime);
}

int SQLiteSingleVerRelationalStorageExecutor::GetMissQueryDataAndStepNext(sqlite3_stmt *fullStmt, DataItem &item,
    Timestamp &missQueryTime)
{
    int errCode = GetMissQueryData(fullStmt, item);
    if (errCode != E_OK) {
        return errCode;
    }
    return StepNext(isMemDb_, fullStmt, missQueryTime);
}

int SQLiteSingleVerRelationalStorageExecutor::GetSyncDataByQuery(std::vector<DataItem> &dataItems, size_t appendLength,
    const DataSizeSpecInfo &sizeInfo, std::function<int(sqlite3 *, sqlite3_stmt *&, sqlite3_stmt *&, bool &)> getStmt,
    const TableInfo &tableInfo)
{
    baseTblName_ = tableInfo.GetTableName();
    SetTableInfo(tableInfo);
    sqlite3_stmt *queryStmt = nullptr;
    sqlite3_stmt *fullStmt = nullptr;
    bool isGettingDeletedData = false;
    int errCode = getStmt(dbHandle_, queryStmt, fullStmt, isGettingDeletedData);
    if (errCode != E_OK) {
        return errCode;
    }

    Timestamp queryTime = 0;
    Timestamp missQueryTime = (fullStmt == nullptr ? INT64_MAX : 0);

    bool isFirstTime = true;
    size_t dataTotalSize = 0;
    size_t overLongSize = 0;
    do {
        DataItem item;
        if (queryTime < missQueryTime) {
            errCode = GetQueryDataAndStepNext(isFirstTime, isGettingDeletedData, queryStmt, item, queryTime);
        } else if (queryTime == missQueryTime) {
            errCode = GetQueryDataAndStepNext(isFirstTime, isGettingDeletedData, queryStmt, item, queryTime);
            if (errCode != E_OK) {
                break;
            }
            errCode = StepNext(isMemDb_, fullStmt, missQueryTime);
        } else {
            errCode = GetMissQueryDataAndStepNext(fullStmt, item, missQueryTime);
        }

        if (errCode == E_OK && !isFirstTime) {
            errCode = AppendData(sizeInfo, appendLength, overLongSize, dataTotalSize, dataItems, std::move(item));
        }

        if (errCode != E_OK) {
            break;
        }

        isFirstTime = false;
        if (queryTime == INT64_MAX && missQueryTime == INT64_MAX) {
            errCode = -E_FINISHED;
            break;
        }
    } while (true);
    LOGI("Get sync data finished, rc:%d, record size:%zu, overlong size:%zu, isDeleted:%d",
        errCode, dataItems.size(), overLongSize, isGettingDeletedData);
    SQLiteUtils::ResetStatement(queryStmt, true, errCode);
    SQLiteUtils::ResetStatement(fullStmt, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CheckDBModeForRelational()
{
    std::string journalMode;
    int errCode = SQLiteUtils::GetJournalMode(dbHandle_, journalMode);

    for (auto &c : journalMode) { // convert to lowercase
        c = static_cast<char>(std::tolower(c));
    }

    if (errCode == E_OK && journalMode != "wal") {
        LOGE("Not support journal mode %s for relational db, expect wal mode.", journalMode.c_str());
        return -E_NOT_SUPPORT;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteDistributedDeviceTable(const std::string &device,
    const std::string &tableName)
{
    std::vector<std::string> deviceTables;
    int errCode = GetDeviceTableName(dbHandle_, tableName, device, deviceTables);
    if (errCode != E_OK) {
        LOGE("Get device table name for alter table failed. %d", errCode);
        return errCode;
    }

    LOGD("Begin to delete device table: deviceTable[%zu]", deviceTables.size());
    for (const auto &table : deviceTables) {
        std::string deleteSql = "DROP TABLE IF EXISTS " + table + ";"; // drop the found table
        errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, deleteSql);
        if (errCode != E_OK) {
            LOGE("Delete device data failed. %d", errCode);
            break;
        }
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteDistributedAllDeviceTableLog(const std::string &tableName)
{
    std::string deleteLogSql =
        "DELETE FROM " + DBConstant::RELATIONAL_PREFIX + tableName +
        "_log WHERE flag&0x02=0 AND (cloud_gid = '' OR cloud_gid IS NULL)";
    return SQLiteUtils::ExecuteRawSQL(dbHandle_, deleteLogSql);
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteDistributedDeviceTableLog(const std::string &device,
    const std::string &tableName)
{
    std::string deleteLogSql = "DELETE FROM " + DBConstant::RELATIONAL_PREFIX + tableName + "_log WHERE device = ?";
    sqlite3_stmt *deleteLogStmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, deleteLogSql, deleteLogStmt);
    if (errCode != E_OK) {
        LOGE("Get delete device data log statement failed. %d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::BindTextToStatement(deleteLogStmt, 1, device);
    if (errCode != E_OK) {
        LOGE("Bind device to delete data log statement failed. %d", errCode);
        SQLiteUtils::ResetStatement(deleteLogStmt, true, errCode);
        return errCode;
    }

    errCode = SQLiteUtils::StepWithRetry(deleteLogStmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    } else {
        LOGE("Delete data log failed. %d", errCode);
    }

    SQLiteUtils::ResetStatement(deleteLogStmt, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteDistributedLogTable(const std::string &tableName)
{
    if (tableName.empty()) {
        return -E_INVALID_ARGS;
    }
    std::string logTableName = DBConstant::RELATIONAL_PREFIX + tableName + "_log";
    std::string deleteSql = "DROP TABLE IF EXISTS " + logTableName + ";";
    int errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, deleteSql);
    if (errCode != E_OK) {
        LOGE("Delete distributed log table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::IsTableOnceDropped(const std::string &tableName, int execCode,
    bool &onceDropped)
{
    if (execCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) { // The table in schema was dropped
        onceDropped = true;
        return E_OK;
    } else if (execCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::string keyStr = DBConstant::TABLE_IS_DROPPED + tableName;
        Key key;
        DBCommon::StringToVector(keyStr, key);
        Value value;

        int errCode = GetKvData(key, value);
        if (errCode == E_OK) {
            // if user drop table first, then create again(but don't create distributed table), will reach this branch
            onceDropped = true;
            return E_OK;
        } else if (errCode == -E_NOT_FOUND) {
            onceDropped = false;
            return E_OK;
        } else {
            LOGE("[IsTableOnceDropped] query is table dropped failed, %d", errCode);
            return errCode;
        }
    } else {
        return execCode;
    }
}

int SQLiteSingleVerRelationalStorageExecutor::CleanResourceForDroppedTable(const std::string &tableName)
{
    int errCode = DeleteDistributedDeviceTable({}, tableName); // Clean the auxiliary tables for the dropped table
    if (errCode != E_OK) {
        LOGE("Delete device tables for missing distributed table failed. %d", errCode);
        return errCode;
    }
    errCode = DeleteDistributedLogTable(tableName);
    if (errCode != E_OK) {
        LOGE("Delete log tables for missing distributed table failed. %d", errCode);
        return errCode;
    }
    errCode = DeleteTableTrigger(tableName);
    if (errCode != E_OK) {
        LOGE("Delete trigger for missing distributed table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CheckAndCleanDistributedTable(const std::vector<std::string> &tableNames,
    std::vector<std::string> &missingTables)
{
    if (tableNames.empty()) {
        return E_OK;
    }
    const std::string checkSql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt *stmt = nullptr;
    int ret = E_OK;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, checkSql, stmt);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, ret);
        return errCode;
    }
    for (const auto &tableName : tableNames) {
        errCode = SQLiteUtils::BindTextToStatement(stmt, 1, tableName); // 1: tablename bind index
        if (errCode != E_OK) {
            LOGE("Bind table name to check distributed table statement failed. %d", errCode);
            break;
        }

        errCode = SQLiteUtils::StepWithRetry(stmt, false);
        bool onceDropped = false;
        errCode = IsTableOnceDropped(tableName, errCode, onceDropped);
        if (errCode != E_OK) {
            LOGE("query is table once dropped failed. %d", errCode);
            break;
        }
        SQLiteUtils::ResetStatement(stmt, false, ret);
        if (onceDropped) { // The table in schema was once dropped
            errCode = CleanResourceForDroppedTable(tableName);
            if (errCode != E_OK) {
                break;
            }
            missingTables.emplace_back(tableName);
        }
    }
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return CheckCorruptedStatus(errCode);
}

int SQLiteSingleVerRelationalStorageExecutor::CreateDistributedDeviceTable(const std::string &device,
    const TableInfo &baseTbl, const StoreInfo &info)
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }

    if (device.empty() || !baseTbl.IsValid()) {
        return -E_INVALID_ARGS;
    }

    std::string deviceTableName = DBCommon::GetDistributedTableName(device, baseTbl.GetTableName(), info);
    int errCode = SQLiteUtils::CreateSameStuTable(dbHandle_, baseTbl, deviceTableName);
    if (errCode != E_OK) {
        LOGE("Create device table failed. %d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::CloneIndexes(dbHandle_, baseTbl.GetTableName(), deviceTableName);
    if (errCode != E_OK) {
        LOGE("Copy index to device table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CheckQueryObjectLegal(const TableInfo &table, QueryObject &query,
    const std::string &schemaVersion)
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }

    TableInfo newTable;
    int errCode = SQLiteUtils::AnalysisSchema(dbHandle_, table.GetTableName(), newTable);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("Check new schema failed. %d", errCode);
        return errCode;
    } else {
        errCode = table.CompareWithTable(newTable, schemaVersion);
        if (errCode != -E_RELATIONAL_TABLE_EQUAL && errCode != -E_RELATIONAL_TABLE_COMPATIBLE) {
            LOGE("Check schema failed, schema was changed. %d", errCode);
            return -E_DISTRIBUTED_SCHEMA_CHANGED;
        } else {
            errCode = E_OK;
        }
    }

    SqliteQueryHelper helper = query.GetQueryHelper(errCode);
    if (errCode != E_OK) {
        LOGE("Get query helper for check query failed. %d", errCode);
        return errCode;
    }

    if (!query.IsQueryForRelationalDB()) {
        LOGE("Not support for this query type.");
        return -E_NOT_SUPPORT;
    }

    SyncTimeRange defaultTimeRange;
    sqlite3_stmt *stmt = nullptr;
    errCode = helper.GetRelationalQueryStatement(dbHandle_, defaultTimeRange.beginTime, defaultTimeRange.endTime, {},
        stmt);
    if (errCode != E_OK) {
        LOGE("Get query statement for check query failed. %d", errCode);
    }

    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CheckQueryObjectLegal(const QuerySyncObject &query)
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }
    TableInfo newTable;
    int errCode = SQLiteUtils::AnalysisSchema(dbHandle_, query.GetTableName(), newTable);
    if (errCode != E_OK) {
        LOGE("Check new schema failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetMaxTimestamp(const std::vector<std::string> &tableNames,
    Timestamp &maxTimestamp) const
{
    maxTimestamp = 0;
    for (const auto &tableName : tableNames) {
        const std::string sql = "SELECT MAX(timestamp) FROM " + DBConstant::RELATIONAL_PREFIX + tableName + "_log;";
        sqlite3_stmt *stmt = nullptr;
        int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb_);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            maxTimestamp = std::max(maxTimestamp, static_cast<Timestamp>(sqlite3_column_int64(stmt, 0))); // 0 is index
            errCode = E_OK;
        }
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        if (errCode != E_OK) {
            maxTimestamp = 0;
            return errCode;
        }
    }
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::SetLogTriggerStatus(bool status)
{
    const std::string key = "log_trigger_switch";
    std::string val = status ? "true" : "false";
    std::string sql = "INSERT OR REPLACE INTO " + DBConstant::RELATIONAL_PREFIX + "metadata" +
        " VALUES ('" + key + "', '" + val + "')";
    int errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, sql);
    if (errCode != E_OK) {
        LOGE("Set log trigger to %s failed. errCode=%d", val.c_str(), errCode);
    }
    return errCode;
}

namespace {
int GetRowDatas(sqlite3_stmt *stmt, bool isMemDb, std::vector<std::string> &colNames,
    std::vector<RelationalRowData *> &data)
{
    size_t totalLength = 0;
    do {
        int errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            return E_OK;
        } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            LOGE("Get data by bind sql failed:%d", errCode);
            return errCode;
        }

        if (colNames.empty()) {
            SQLiteUtils::GetSelectCols(stmt, colNames);  // Get column names.
        }
        auto relaRowData = new (std::nothrow) RelationalRowDataImpl(SQLiteRelationalUtils::GetSelectValues(stmt));
        if (relaRowData == nullptr) {
            LOGE("ExecuteQueryBySqlStmt OOM");
            return -E_OUT_OF_MEMORY;
        }

        auto dataSz = relaRowData->CalcLength();
        if (dataSz == 0) {  // invalid data
            delete relaRowData;
            relaRowData = nullptr;
            continue;
        }

        totalLength += static_cast<size_t>(dataSz);
        if (totalLength > static_cast<uint32_t>(DBConstant::MAX_REMOTEDATA_SIZE)) {  // the set has been full
            delete relaRowData;
            relaRowData = nullptr;
            LOGE("ExecuteQueryBySqlStmt OVERSIZE");
            return -E_REMOTE_OVER_SIZE;
        }
        data.push_back(relaRowData);
    } while (true);
    return E_OK;
}
}

// sql must not be empty, colNames and data must be empty
int SQLiteSingleVerRelationalStorageExecutor::ExecuteQueryBySqlStmt(const std::string &sql,
    const std::vector<std::string> &bindArgs, int packetSize, std::vector<std::string> &colNames,
    std::vector<RelationalRowData *> &data)
{
    int errCode = SQLiteUtils::SetAuthorizer(dbHandle_, &PermitSelect);
    if (errCode != E_OK) {
        return errCode;
    }

    sqlite3_stmt *stmt = nullptr;
    errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        (void)SQLiteUtils::SetAuthorizer(dbHandle_, nullptr);
        return errCode;
    }
    ResFinalizer finalizer([this, &stmt, &errCode] {
        (void)SQLiteUtils::SetAuthorizer(this->dbHandle_, nullptr);
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    });
    for (size_t i = 0; i < bindArgs.size(); ++i) {
        errCode = SQLiteUtils::BindTextToStatement(stmt, i + 1, bindArgs.at(i));
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return GetRowDatas(stmt, isMemDb_, colNames, data);
}

int SQLiteSingleVerRelationalStorageExecutor::CheckEncryptedOrCorrupted() const
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }

    int errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, "SELECT count(*) FROM sqlite_master;");
    if (errCode != E_OK) {
        LOGE("[SingVerRelaExec] CheckEncryptedOrCorrupted failed:%d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetExistsDeviceList(std::set<std::string> &devices) const
{
    return SqliteMetaExecutor::GetExistsDevicesFromMeta(dbHandle_, SqliteMetaExecutor::MetaMode::RDB,
        isMemDb_, devices);
}

int SQLiteSingleVerRelationalStorageExecutor::GetUploadCountInner(const Timestamp &timestamp,
    SqliteQueryHelper &helper, std::string &sql, int64_t &count)
{
    int errCode = E_OK;
    sqlite3_stmt *stmt = nullptr;
    errCode = helper.GetCloudQueryStatement(false, dbHandle_, timestamp, sql, stmt);
    if (errCode != E_OK) {
        LOGE("failed to get count statement %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        count = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
        errCode = E_OK;
    } else {
        LOGE("Failed to get the count to be uploaded. %d", errCode);
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetUploadCount(const Timestamp &timestamp, bool isCloudForcePush,
    bool isCompensatedTask, QuerySyncObject &query, int64_t &count)
{
    int errCode;
    SqliteQueryHelper helper = query.GetQueryHelper(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    std::string tableName = query.GetRelationTableName();
    std::string sql = helper.GetCountRelationalCloudQuerySql(isCloudForcePush, isCompensatedTask,
        CloudWaterType::DELETE);
    return GetUploadCountInner(timestamp, helper, sql, count);
}

int SQLiteSingleVerRelationalStorageExecutor::GetAllUploadCount(const std::vector<Timestamp> &timestampVec,
    bool isCloudForcePush, bool isCompensatedTask, QuerySyncObject &query, int64_t &count)
{
    if (timestampVec.size() != 3) { // 3 is the number of three mode.
        return -E_INVALID_ARGS;
    }
    int errCode;
    SqliteQueryHelper helper = query.GetQueryHelper(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    std::string tableName = query.GetRelationTableName();
    count = 0;
    std::vector<CloudWaterType> typeVec = {CloudWaterType::DELETE, CloudWaterType::UPDATE, CloudWaterType::INSERT};
    for (size_t i = 0; i < typeVec.size(); i++) {
        std::string sql = helper.GetCountRelationalCloudQuerySql(isCloudForcePush, isCompensatedTask, typeVec[i]);
        int64_t tempCount = 0;
        helper.AppendCloudQueryToGetDiffData(sql, typeVec[i]);
        errCode = GetUploadCountInner(timestampVec[i], helper, sql, tempCount);
        if (errCode != E_OK) {
            return errCode;
        }
        count += tempCount;
    }
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::UpdateCloudLogGid(const CloudSyncData &cloudDataResult,
    bool ignoreEmptyGid)
{
    if (cloudDataResult.insData.extend.empty() || cloudDataResult.insData.rowid.empty() ||
        cloudDataResult.insData.extend.size() != cloudDataResult.insData.rowid.size()) {
        return -E_INVALID_ARGS;
    }
    std::string sql = "UPDATE '" + DBCommon::GetLogTableName(cloudDataResult.tableName)
        + "' SET cloud_gid = ? WHERE data_key = ? ";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = BindStmtWithCloudGid(cloudDataResult, ignoreEmptyGid, stmt);
    int resetCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, resetCode);
    return errCode == E_OK ? resetCode : errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetSyncCloudData(CloudSyncData &cloudDataResult,
    const uint32_t &maxSize, SQLiteSingleVerRelationalContinueToken &token)
{
    token.GetCloudTableSchema(tableSchema_);
    sqlite3_stmt *queryStmt = nullptr;
    bool isStepNext = false;
    int errCode = token.GetCloudStatement(dbHandle_, cloudDataResult, queryStmt, isStepNext);
    if (errCode != E_OK) {
        (void)token.ReleaseCloudStatement();
        return errCode;
    }
    uint32_t totalSize = 0;
    uint32_t stepNum = -1;
    do {
        if (isStepNext) {
            errCode = SQLiteUtils::StepNext(queryStmt, isMemDb_);
            if (errCode != E_OK) {
                errCode = (errCode == -E_FINISHED ? E_OK : errCode);
                break;
            }
        }
        isStepNext = true;
        errCode = GetCloudDataForSync(queryStmt, cloudDataResult, ++stepNum, totalSize, maxSize);
    } while (errCode == E_OK);
    if (errCode != -E_UNFINISHED) {
        (void)token.ReleaseCloudStatement();
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetSyncCloudGid(QuerySyncObject &query,
    const SyncTimeRange &syncTimeRange, bool isCloudForcePushStrategy,
    bool isCompensatedTask, std::vector<std::string> &cloudGid)
{
    sqlite3_stmt *queryStmt = nullptr;
    int errCode = E_OK;
    SqliteQueryHelper helper = query.GetQueryHelper(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    std::string sql = helper.GetGidRelationalCloudQuerySql(tableSchema_.fields, isCloudForcePushStrategy,
        isCompensatedTask);
    errCode = helper.GetCloudQueryStatement(false, dbHandle_, syncTimeRange.beginTime, sql, queryStmt);
    if (errCode != E_OK) {
        return errCode;
    }
    do {
        errCode = SQLiteUtils::StepNext(queryStmt, isMemDb_);
        if (errCode != E_OK) {
            errCode = (errCode == -E_FINISHED ? E_OK : errCode);
            break;
        }
        GetCloudGid(queryStmt, cloudGid);
    } while (errCode == E_OK);
    int resetStatementErrCode = E_OK;
    SQLiteUtils::ResetStatement(queryStmt, true, resetStatementErrCode);
    queryStmt = nullptr;
    return (errCode == E_OK ? resetStatementErrCode : errCode);
}

int SQLiteSingleVerRelationalStorageExecutor::GetCloudDataForSync(sqlite3_stmt *statement,
    CloudSyncData &cloudDataResult, uint32_t &stepNum, uint32_t &totalSize, const uint32_t &maxSize)
{
    VBucket log;
    VBucket extraLog;
    uint32_t preSize = totalSize;
    GetCloudLog(statement, log, totalSize, cloudDataResult.isShared);
    GetCloudExtraLog(statement, extraLog);

    VBucket data;
    int64_t flag = 0;
    int errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::FLAG, extraLog, flag);
    if (errCode != E_OK) {
        return errCode;
    }
    if ((static_cast<uint64_t>(flag) & DataItem::DELETE_FLAG) == 0) {
        for (size_t cid = 0; cid < tableSchema_.fields.size(); ++cid) {
            Type cloudValue;
            errCode = SQLiteRelationalUtils::GetCloudValueByType(statement,
                tableSchema_.fields[cid].type, cid + STATUS_INDEX + 1, cloudValue);
            if (errCode != E_OK) {
                return errCode;
            }
            SQLiteRelationalUtils::CalCloudValueLen(cloudValue, totalSize);
            errCode = PutVBucketByType(data, tableSchema_.fields[cid], cloudValue);
            if (errCode != E_OK) {
                return errCode;
            }
        }
    }

    if (CloudStorageUtils::IsGetCloudDataContinue(stepNum, totalSize, maxSize)) {
        errCode = CloudStorageUtils::IdentifyCloudType(cloudDataResult, data, log, extraLog);
    } else {
        errCode = -E_UNFINISHED;
    }
    if (errCode == -E_IGNORE_DATA) {
        errCode = E_OK;
        totalSize = preSize;
        stepNum--;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::PutVBucketByType(VBucket &vBucket, const Field &field, Type &cloudValue)
{
    if (field.type == TYPE_INDEX<Asset> && cloudValue.index() == TYPE_INDEX<Bytes>) {
        Asset asset;
        int errCode = RuntimeContext::GetInstance()->BlobToAsset(std::get<Bytes>(cloudValue), asset);
        if (errCode != E_OK) {
            return errCode;
        }
        if (!CloudStorageUtils::CheckAssetStatus({asset})) {
            return -E_CLOUD_INVALID_ASSET;
        }
        vBucket.insert_or_assign(field.colName, asset);
    } else if (field.type == TYPE_INDEX<Assets> && cloudValue.index() == TYPE_INDEX<Bytes>) {
        Assets assets;
        int errCode = RuntimeContext::GetInstance()->BlobToAssets(std::get<Bytes>(cloudValue), assets);
        if (errCode != E_OK) {
            return errCode;
        }
        if (CloudStorageUtils::IsAssetsContainDuplicateAsset(assets)) {
            return -E_CLOUD_ERROR;
        }
        if (!CloudStorageUtils::CheckAssetStatus(assets)) {
            return -E_CLOUD_INVALID_ASSET;
        }
        vBucket.insert_or_assign(field.colName, assets);
    } else {
        vBucket.insert_or_assign(field.colName, cloudValue);
    }
    return E_OK;
}

void SQLiteSingleVerRelationalStorageExecutor::SetLocalSchema(const RelationalSchemaObject &localSchema)
{
    localSchema_ = localSchema;
}

int SQLiteSingleVerRelationalStorageExecutor::CleanCloudDataOnLogTable(const std::string &logTableName)
{
    std::string cleanLogSql = "UPDATE " + logTableName + " SET " + CloudDbConstant::FLAG + " = " +
        SET_FLAG_LOCAL_AND_CLEAN_WAIT_COMPENSATED_SYNC + ", " +
        DEVICE_FIELD + " = '', " + CLOUD_GID_FIELD + " = '', " + SHARING_RESOURCE + " = '' " +
        "WHERE (" + FLAG_IS_LOGIC_DELETE + ") OR " +
        CLOUD_GID_FIELD + " IS NOT NULL AND " + CLOUD_GID_FIELD + " != '';";
    int errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, cleanLogSql);
    if (errCode != E_OK) {
        LOGE("clean cloud log failed, %d", errCode);
        return errCode;
    }
    cleanLogSql = "DELETE FROM " + logTableName + " WHERE " + FLAG_IS_CLOUD + " AND " + DATA_IS_DELETE + ";";
    return SQLiteUtils::ExecuteRawSQL(dbHandle_, cleanLogSql);
}

int SQLiteSingleVerRelationalStorageExecutor::CleanCloudDataAndLogOnUserTable(const std::string &tableName,
    const std::string &logTableName, const RelationalSchemaObject &localSchema)
{
    std::string sql = "DELETE FROM '" + tableName + "' WHERE " + std::string(DBConstant::SQLITE_INNER_ROWID) +
        " IN (SELECT " + DATAKEY + " FROM '" + logTableName + "' WHERE (" + FLAG_IS_LOGIC_DELETE +
        ") OR CLOUD_GID IS NOT NULL AND CLOUD_GID != '' AND " + FLAG_IS_CLOUD + ");";
    int errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, sql);
    if (errCode != E_OK) {
        LOGE("Failed to delete cloud data on usertable, %d.", errCode);
        return errCode;
    }
    std::string cleanLogSql = "DELETE FROM '" + logTableName + "' WHERE " + FLAG_IS_CLOUD + ";";
    errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, cleanLogSql);
    if (errCode != E_OK) {
        LOGE("Failed to delete cloud data on log table, %d.", errCode);
        return errCode;
    }
    errCode = DoCleanAssetId(tableName, localSchema);
    if (errCode != E_OK) {
        LOGE("[Storage Executor] failed to clean asset id when clean cloud data, %d", errCode);
        return errCode;
    }
    errCode = CleanCloudDataOnLogTable(logTableName);
    if (errCode != E_OK) {
        LOGE("Failed to clean gid on log table, %d.", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetCleanCloudDataKeys(const std::string &logTableName,
    std::vector<int64_t> &dataKeys, bool distinguishCloudFlag)
{
    sqlite3_stmt *selectStmt = nullptr;
    std::string sql = "SELECT DATA_KEY FROM '" + logTableName + "' WHERE " + CLOUD_GID_FIELD +
        " IS NOT NULL AND " + CLOUD_GID_FIELD + " != '' AND data_key != '-1'";
    if (distinguishCloudFlag) {
        sql += " AND ";
        sql += FLAG_IS_CLOUD;
    }
    sql += ";";
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, selectStmt);
    if (errCode != E_OK) {
        LOGE("Get select data_key statement failed, %d", errCode);
        return errCode;
    }
    do {
        errCode = SQLiteUtils::StepWithRetry(selectStmt);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            dataKeys.push_back(sqlite3_column_int64(selectStmt, 0));
        } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            LOGE("SQLite step failed when query log's data_key : %d", errCode);
            break;
        }
    } while (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    SQLiteUtils::ResetStatement(selectStmt, true, errCode);
    return (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) ? E_OK : errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetUpdateLogRecordStatement(const TableSchema &tableSchema,
    const VBucket &vBucket, OpType opType, std::vector<std::string> &updateColName, sqlite3_stmt *&updateLogStmt)
{
    std::string updateLogSql = "update " + DBCommon::GetLogTableName(tableSchema.name) + " set ";
    if (opType == OpType::ONLY_UPDATE_GID) {
        updateLogSql += "cloud_gid = ?";
        updateColName.push_back(CloudDbConstant::GID_FIELD);
        CloudStorageUtils::AddUpdateColForShare(tableSchema, updateLogSql, updateColName);
    } else if (opType == OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO) {
        updateLogSql += "flag = flag & " + std::to_string(SET_FLAG_ZERO_MASK); // clear 2th bit of flag
        CloudStorageUtils::AddUpdateColForShare(tableSchema, updateLogSql, updateColName);
    } else if (opType == OpType::SET_CLOUD_FORCE_PUSH_FLAG_ONE) {
        updateLogSql += "flag = flag | " + std::to_string(SET_FLAG_ONE_MASK); // set 2th bit of flag
        CloudStorageUtils::AddUpdateColForShare(tableSchema, updateLogSql, updateColName);
    }  else if (opType == OpType::UPDATE_TIMESTAMP) {
        updateLogSql += "device = 'cloud', flag = flag & " + std::to_string(SET_CLOUD_FLAG) +
            ", timestamp = ?, cloud_gid = '', version = '', sharing_resource = ''";
        updateColName.push_back(CloudDbConstant::MODIFY_FIELD);
    } else if (opType == OpType::CLEAR_GID) {
        updateLogSql += "cloud_gid = '', version = '', sharing_resource = '', flag = flag & " +
            std::to_string(SET_FLAG_ZERO_MASK);
    } else if (opType == OpType::LOCKED_NOT_HANDLE) {
        updateLogSql += std::string(CloudDbConstant::TO_LOCAL_CHANGE) + ", cloud_gid = ?";
        updateColName.push_back(CloudDbConstant::GID_FIELD);
    } else {
        if (opType == OpType::DELETE) {
            updateLogSql += GetCloudDeleteSql(DBCommon::GetLogTableName(tableSchema.name));
        } else {
            updateLogSql += GetUpdateDataFlagSql() + ", cloud_gid = ?";
            updateColName.push_back(CloudDbConstant::GID_FIELD);
            CloudStorageUtils::AddUpdateColForShare(tableSchema, updateLogSql, updateColName);
        }
        updateLogSql += ", device = 'cloud', timestamp = ?";
        updateColName.push_back(CloudDbConstant::MODIFY_FIELD);
    }

    int errCode = AppendUpdateLogRecordWhereSqlCondition(tableSchema, vBucket, updateLogSql);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SQLiteUtils::GetStatement(dbHandle_, updateLogSql, updateLogStmt);
    if (errCode != E_OK) {
        LOGE("Get update log statement failed when update cloud data, %d", errCode);
    }
    return errCode;
}
} // namespace DistributedDB
#endif

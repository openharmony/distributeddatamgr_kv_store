/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "relational_store_client_utils.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "res_finalizer.h"
#include "sqlite_relational_utils.h"

namespace DistributedDB {
constexpr size_t MAX_SLOT_NUM = 100;   // Max tables of each matrix file
constexpr size_t MATRIX_FILE_SLOT_SIZE = sizeof(uint64_t);
constexpr size_t MATRIX_FILE_SIZE = MAX_SLOT_NUM * MATRIX_FILE_SLOT_SIZE;

int RelationalStoreClientUtils::UpdateDataLog(sqlite3 *db, const DistributedDB::UpdateOption &option)
{
    auto errCode = CheckUpdateOption(db, option);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isCreate = false;
    errCode = SQLiteUtils::CheckTableExists(db, DBCommon::GetMetaTableName(), isCreate);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCreate) {
        LOGE("[RDBClientUtils] UpdateDataLog meta not found",
            DBCommon::StringMiddleMaskingWithLen(option.tableName).c_str());
        return -E_DISTRIBUTED_SCHEMA_NOT_FOUND;
    }
    auto [ret, rdbSchema] = GetRDBSchema(db, false);
    if (ret != E_OK) {
        return ret;
    }
    isCreate = false;
    errCode = SQLiteUtils::CheckTableExists(db, option.tableName, isCreate);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCreate) {
        LOGE("[RDBClientUtils] UpdateDataLog table[%s] not found",
            DBCommon::StringMiddleMaskingWithLen(option.tableName).c_str());
        return -E_TABLE_NOT_FOUND;
    }
    auto tableInfo = rdbSchema.GetTable(option.tableName);
    if (tableInfo.GetTableName().empty()) {
        return -E_DISTRIBUTED_SCHEMA_NOT_FOUND;
    }
    auto tableMode = rdbSchema.GetTableMode();
    if (tableMode != DistributedTableMode::COLLABORATION) {
        LOGE("[RDBClientUtils] UpdateDataLog table[%s] mode[%d] not collaboration",
            DBCommon::StringMiddleMaskingWithLen(option.tableName).c_str(), static_cast<int>(tableMode));
        return -E_NOT_SUPPORT;
    }
    return UpdateDataLogInner(db, option);
}

std::pair<int, RelationalSchemaObject> RelationalStoreClientUtils::GetRDBSchema(sqlite3 *db, bool isTracker)
{
    std::pair<int, RelationalSchemaObject> res;
    auto &[errCode, rdbSchema] = res;
    std::string schemaKey = isTracker ? DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY
                                      : DBConstant::RELATIONAL_SCHEMA_KEY;
    const Key schema(schemaKey.begin(), schemaKey.end());
    Value schemaVal;
    errCode = SQLiteRelationalUtils::GetKvData(db, false, schema, schemaVal); // save schema to meta_data
    if (errCode == -E_NOT_FOUND) {
        LOGD("[RDBClientUtils] Not found rdb schema in db");
        errCode = E_OK;
        return res;
    }
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Get rdb schema from meta table failed. %d", errCode);
        return res;
    }
    std::string schemaJson(schemaVal.begin(), schemaVal.end());
    if (isTracker) {
        errCode = rdbSchema.ParseFromTrackerSchemaString(schemaJson);
    } else {
        errCode = rdbSchema.ParseFromSchemaString(schemaJson);
    }
    return res;
}

int RelationalStoreClientUtils::CheckUpdateOption(sqlite3 *db, const UpdateOption &option)
{
    if (db == nullptr) {
        LOGE("[RDBClientUtils] CheckUpdateOption db is nullptr");
        return -E_INVALID_ARGS;
    }
    if (option.tableName.empty()) {
        LOGE("[RDBClientUtils] CheckUpdateOption tableName is empty");
        return -E_INVALID_ARGS;
    }
    if (option.condition.logCondition.has_value() && option.condition.dataCondition.has_value()) {
        LOGE("[RDBClientUtils] CheckUpdateOption both condition exists");
        return -E_INVALID_ARGS;
    }
    if (!option.condition.logCondition.has_value() && !option.condition.dataCondition.has_value()) {
        LOGE("[RDBClientUtils] CheckUpdateOption both condition not exists");
        return -E_INVALID_ARGS;
    }
    int errCode = CheckSelectCondition(option.condition.logCondition, "log");
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = CheckSelectCondition(option.condition.dataCondition, "data");
    if (errCode != E_OK) {
        return errCode;
    }
    return CheckUpdateContent(option.content);
}

int RelationalStoreClientUtils::CheckSelectCondition(const std::optional<SelectCondition> &condition,
    const std::string &dfxLog)
{
    if (!condition.has_value()) {
        return E_OK;
    }
    auto &cd = condition.value();
    if (cd.sql.empty()) {
        LOGE("[RDBClientUtils] CheckSelectCondition %s sql is empty", dfxLog.c_str());
        return -E_INVALID_ARGS;
    }
    int count = std::count(cd.sql.begin(), cd.sql.end(), '?');
    if (static_cast<size_t>(count) != cd.args.size()) {
        LOGE("[RDBClientUtils] CheckSelectCondition %s args[%d] not match sql[%zu]", dfxLog.c_str(), count,
            cd.args.size());
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int RelationalStoreClientUtils::CheckUpdateContent(const UpdateContent &content)
{
    if (!content.flag.has_value() && !content.oriDevice.has_value()) {
        LOGE("[RDBClientUtils] CheckUpdateContent update content not exists");
        return -E_INVALID_ARGS;
    }
    if (!content.flag.has_value()) {
        return E_OK;
    }
    if (static_cast<uint32_t>(content.flag.value()) >= static_cast<uint32_t>(LogFlag::BUTT)) {
        LOGE("[RDBClientUtils] CheckUpdateContent invalid flag[%" PRIu32 "]",
            static_cast<uint32_t>(content.flag.value()));
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int RelationalStoreClientUtils::UpdateDataLogInner(sqlite3 *db, const UpdateOption &option)
{
    if (sqlite3_get_autocommit(db) == 0) {
        return UpdateDataLogInTransaction(db, option);
    }
    auto errCode = SQLiteUtils::BeginTransaction(db, TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Update data log begin transaction failed[%d]", errCode);
        return errCode;
    }
    errCode = UpdateDataLogInTransaction(db, option);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Update data log in transaction failed[%d]", errCode);
        int ret = SQLiteUtils::RollbackTransaction(db);
        if (ret != E_OK) {
            LOGE("[RDBClientUtils] Update data log rollback transaction failed[%d]", errCode);
        }
    } else {
        errCode = SQLiteUtils::CommitTransaction(db);
        if (errCode != E_OK) {
            LOGE("[RDBClientUtils] Update data log commit transaction failed[%d]", errCode);
            return errCode;
        }
    }
    return errCode;
}

int RelationalStoreClientUtils::UpdateDataLogInTransaction(sqlite3 *db, const UpdateOption &option)
{
    auto sql = GetUpdateSQL(option);
    sqlite3_stmt *stmt = nullptr;
    auto errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Get statement failed[%d]", errCode);
        return errCode;
    }
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *releaseStmt = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(releaseStmt, true, ret);
        if (ret != E_OK) {
            LOGE("[RDBClientUtils] Reset statement failed[%d]", ret);
        }
    });
    errCode = BindDataLogValue(stmt, option);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Bind data log value failed[%d]", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepNext(stmt);
    if (errCode == -E_FINISHED) {
        errCode = E_OK;
    }
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Step statement failed[%d]", errCode);
        return errCode;
    }
    LOGI("[RDBClientUtils] Update data log[%s] count[%d] success",
        DBCommon::StringMiddleMaskingWithLen(option.tableName).c_str(), sqlite3_changes(db));
    return E_OK;
}

std::string RelationalStoreClientUtils::GetUpdateSQL(const UpdateOption &option)
{
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(option.tableName) + " SET " + GetUpdateLogSQL(option);
    sql += " WHERE ";
    if (option.condition.logCondition.has_value()) {
        sql += option.condition.logCondition.value().sql;
    } else {
        sql += " data_key IN (SELECT _rowid_ FROM " + option.tableName + " WHERE ";
        sql += option.condition.dataCondition.value().sql + ")";
    }
    return sql;
}

std::string RelationalStoreClientUtils::GetUpdateLogSQL(const UpdateOption &option)
{
    std::string sql;
    if (option.content.oriDevice.has_value()) {
        sql += "ori_device = ?";
    }
    if (!option.content.flag.has_value()) {
        return sql;
    }
    if (!sql.empty()) {
        sql += ", ";
    }
    sql += "flag = flag";
    if (option.content.flag.value() == LogFlag::REMOTE) {
        sql.append("&~").append(std::to_string(static_cast<int64_t>(LogFlag::LOCAL)));
    } else if (option.content.flag.value() == LogFlag::LOCAL) {
        sql.append("|").append(std::to_string(static_cast<int64_t>(LogFlag::LOCAL)));
    }
    return sql;
}

int RelationalStoreClientUtils::BindDataLogValue(sqlite3_stmt *stmt, const UpdateOption &option)
{
    int index = 1;
    if (option.content.oriDevice.has_value()) {
        auto hashDev = DBCommon::TransferHashString(option.content.oriDevice.value());
        int errCode;
        if (!hashDev.empty()) {
            errCode = SQLiteUtils::BindBlobToStatement(stmt, index++, Bytes(hashDev.begin(), hashDev.end()));
        } else {
            errCode = SQLiteUtils::BindTextToStatement(stmt, index++, "");
        }
        if (errCode != E_OK) {
            LOGE("[RDBClientUtils] Bind ori_device to statement failed[%d]", errCode);
            return errCode;
        }
    }
    auto errCode = BindDataLogCondition(stmt, option.condition.logCondition, true, index);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Bind log condition to statement failed[%d]", errCode);
        return errCode;
    }
    errCode = BindDataLogCondition(stmt, option.condition.dataCondition, false, index);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Bind data condition to statement failed[%d]", errCode);
    }
    return errCode;
}

int RelationalStoreClientUtils::BindDataLogCondition(sqlite3_stmt *stmt,
    const std::optional<SelectCondition> &condition, bool isLog, int &index)
{
    if (!condition.has_value()) {
        return E_OK;
    }
    auto &cd = condition.value();
    for (const auto &arg : cd.args) {
        auto type = arg;
        if (isLog && std::holds_alternative<std::string>(type)) {
            type = DBCommon::TransferHashString(std::get<std::string>(type));
            auto str = std::get<std::string>(type);
            if (!str.empty()) {
                type = Bytes(str.begin(), str.end());
            }
        }
        auto errCode = SQLiteUtils::BindType(stmt, type, index++);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

uint64_t *RelationalStoreClientUtils::MmapMatrixFile(const std::string &path, size_t mmapSize, int32_t &fileFd)
{
    if (mmapSize > MATRIX_FILE_SIZE) {
        LOGE("[RDBClientUtils] mapped size exceed limit, size: %zu", mmapSize);
        return nullptr;
    }

    char *canonicalPath = realpath(path.c_str(), nullptr);
    if (canonicalPath == nullptr) {
        LOGE("[RDBClientUtils] File path is wrong");
        return nullptr;
    }

    std::string realFilePath(canonicalPath);
    free(canonicalPath);
    canonicalPath = nullptr;

    int fd = open(realFilePath.c_str(), O_RDWR);
    if (fd < 0) {
        LOGE("[RDBClientUtils] Open matrix file err: %d", errno);
        return nullptr;
    }
    void *statusData = mmap(nullptr, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (statusData == MAP_FAILED) {
        LOGE("[RDBClientUtils] mmap err: %d, size: %zu", errno, mmapSize);
        close(fd);
        return nullptr;
    }
    int errCode = madvise(statusData, mmapSize, MADV_RANDOM);
    if (errCode != 0) {
        LOGE("[RDBClientUtils] madvise err: %d", errno);
        munmap(statusData, mmapSize);
        close(fd);
        return nullptr;
    }
    fileFd = fd;
    return static_cast<uint64_t *>(statusData);
}

std::vector<uint64_t> RelationalStoreClientUtils::GetMatrixTableIndexs(const MatrixFileInfo &matrixFileInfo,
    const std::vector<std::string> &changedData)
{
    std::vector<uint64_t> indexList;
    for (const auto &tableName : changedData) {
        auto it = matrixFileInfo.matrixTables.find(tableName);
        if (it == matrixFileInfo.matrixTables.end()) {
            LOGW("[RDBClientUtils] Table not registered, %s", DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
            continue;
        }
        uint64_t index = it->second;
        if (index >= MAX_SLOT_NUM) {
            LOGW("[RDBClientUtils] Table index out of range %zu, limit: %zu, table: %s", index, MAX_SLOT_NUM,
                DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
            continue;
        }
        indexList.push_back(index);
    }
    return indexList;
}

int RelationalStoreClientUtils::UpdateMatrixFile(const MatrixFileInfo &fileInfo,
    const std::vector<std::string> &changedData, const MatrixFileUpdateConfig &config)
{
    std::vector<uint64_t> indexList = RelationalStoreClientUtils::GetMatrixTableIndexs(fileInfo, changedData);
    if (indexList.empty() && !config.isFullSync) {
        LOGI("[RDBClientUtils] No change, Index list empty:%d, isFull:%d", indexList.empty(), config.isFullSync);
        return E_OK;
    }
    int fd = -1;
    uint64_t *matrixMapPtr = MmapMatrixFile(fileInfo.matrixFilePath, MATRIX_FILE_SIZE, fd);
    if (matrixMapPtr == nullptr) {
        LOGE("[RDBClientUtils] Matrix map ptr is null");
        return -E_SYSTEM_API_FAIL;
    }

    ResFinalizer finalizer([matrixMapPtr, fd]() {
        close(fd);  // close file to trigger event
        if (munmap(matrixMapPtr, MATRIX_FILE_SIZE) != 0) {
            LOGW("[RDBClientUtils] munmap err: %d", errno);
        }
    });

    // update matrix file content
    for (const auto index : indexList) {
        matrixMapPtr[index] += 1;
    }
    if (config.isFullSync) {
        if (fileInfo.fullSyncOffset >= MAX_SLOT_NUM) {
            LOGW("[RDBClientUtils] isFull offset: %zu out of range %zu", fileInfo.fullSyncOffset, MAX_SLOT_NUM);
        } else {
            matrixMapPtr[fileInfo.fullSyncOffset] += 1;
        }
    }
    int errCode = E_OK;
    if (msync(matrixMapPtr, MATRIX_FILE_SIZE, MS_SYNC) != 0) {
        LOGE("[RDBClientUtils] msync err: %d", errno);
        errCode = -E_SYSTEM_API_FAIL;
    }
    return errCode;
}
} // namespace DistributedDB
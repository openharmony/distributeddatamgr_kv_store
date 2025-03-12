/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "rdb_general_ut.h"
#include "rdb_data_generator.h"

using namespace DistributedDBUnitTest;

namespace DistributedDB {
const Field intField = {"id", TYPE_INDEX<int64_t>, true, false};
const Field stringField = {"name", TYPE_INDEX<std::string>, false, false};
const Field boolField = {"gender", TYPE_INDEX<bool>, false, true};
const Field doubleField = {"height", TYPE_INDEX<double>, false, true};
const Field bytesField = {"photo", TYPE_INDEX<Bytes>, false, true};
const Field assetsField = {"assert", TYPE_INDEX<Asset>, false, true};

const std::vector<UtFieldInfo> g_defaultFiledInfo = {
    {intField, true}, {stringField, false}, {boolField, false},
    {doubleField, false}, {bytesField, false}, {assetsField, false},
};

UtDateBaseSchemaInfo g_defaultSchemaInfo = {
    .tablesInfo = {
        {.name = g_defaultTable1, .sharedTableName = g_defaultTable1 + "_shared", .fieldInfo = g_defaultFiledInfo},
        {.name = g_defaultTable2, .fieldInfo = g_defaultFiledInfo}
    }
};

int RDBGeneralUt::InitDelegate(const StoreInfo &info)
{
    std::string storePath = GetTestDir() + "/" + info.storeId + ".db";
    sqlite3 *db = RelationalTestUtils::CreateDataBase(storePath);
    if (db == nullptr) {
        LOGE("[RDBGeneralUt] Create database failed %s", storePath.c_str());
        return -E_INVALID_DB;
    }
    UtDateBaseSchemaInfo schemaInfo = GetTableSchemaInfo(info);
    int errCode = RDBDataGenerator::InitDatabaseWithSchemaInfo(schemaInfo, *db);
    if (errCode != SQLITE_OK) {
        LOGE("[RDBGeneralUt] Init database failed %d", errCode);
        return errCode;
    }
    RelationalStoreManager mgr(info.appId, info.userId);
    RelationalStoreDelegate *delegate = nullptr;
    errCode = mgr.OpenStore(storePath, info.storeId, option_, delegate);
    if ((errCode != E_OK )|| (delegate == nullptr)) {
        LOGE("[RDBGeneralUt] Open store failed %d", errCode);
        return errCode;
    }
    {
        std::lock_guard<std::mutex> lock(storeMutex_);
        stores_[info] = delegate;
        sqliteDb_[info] = db;
    }
    return E_OK;
}

void RDBGeneralUt::SetOption(const RelationalStoreDelegate::Option& option)
{
    option_ = option;
}

void RDBGeneralUt::AddSchemaInfo(const StoreInfo &info, const DistributedDBUnitTest::UtDateBaseSchemaInfo &schemaInfo)
{
    std::lock_guard<std::mutex> lock(storeMutex_);
    schemaInfoMap_[info] = schemaInfo;
}

UtDateBaseSchemaInfo RDBGeneralUt::GetTableSchemaInfo(const StoreInfo &info)
{
    std::lock_guard<std::mutex> lock(storeMutex_);
    auto iter = schemaInfoMap_.find(info);
    if (iter != schemaInfoMap_.end()) {
        return iter->second;
    }
    return g_defaultSchemaInfo;
}

DataBaseSchema RDBGeneralUt::GetSchema(const StoreInfo &info)
{
    UtDateBaseSchemaInfo schemaInfo = GetTableSchemaInfo(info);
    DataBaseSchema schema;
    for (auto &item : schemaInfo.tablesInfo) {
        TableSchema tableSchema;
        tableSchema.name = item.name;
        tableSchema.sharedTableName = item.sharedTableName;
        for (auto &fieldInfo : item.fieldInfo) {
            tableSchema.fields.push_back(fieldInfo.field);
        }
        schema.tables.push_back(tableSchema);
    }
    return schema;
}

TableSchema RDBGeneralUt::GetTableSchema(const StoreInfo &info, const std::string &tableName)
{
    UtDateBaseSchemaInfo schemaInfo = GetTableSchemaInfo(info);
    for (auto &item : schemaInfo.tablesInfo) {
        if (item.name != tableName) {
            continue;
        }
        TableSchema tableSchema;
        tableSchema.name = item.name;
        tableSchema.sharedTableName = item.sharedTableName;
        for (auto &fieldInfo : item.fieldInfo) {
            tableSchema.fields.push_back(fieldInfo.field);
        }
        return tableSchema;
    }
    LOGD("[RDBGeneralUt] Table %s not found", tableName.c_str());
    return {"", "", {}};
}

std::vector<TrackerSchema> RDBGeneralUt::GetAllTrackerSchema(const StoreInfo &info,
    const std::vector<std::string> &tables)
{
    UtDateBaseSchemaInfo schemaInfo = GetTableSchemaInfo(info);
    std::vector<TrackerSchema> res;
    std::set<std::string> trackerTables(tables.begin(), tables.end());
    for (const auto &item : schemaInfo.tablesInfo) {
        if (trackerTables.find(item.name) == trackerTables.end()) {
            continue;
        }
        TrackerSchema trackerSchema;
        trackerSchema.tableName = item.name;
        for (const auto &field : item.fieldInfo) {
            trackerSchema.trackerColNames.insert(field.field.colName);
            trackerSchema.extendColNames.insert(field.field.colName);
        }
        res.push_back(std::move(trackerSchema));
    }
    return res;
}

int RDBGeneralUt::CloseDelegate(const StoreInfo &info)
{
    std::lock_guard<std::mutex> lock(storeMutex_);
    auto storeIter = stores_.find(info);
    if (storeIter != stores_.end()) {
        RelationalStoreManager mgr(info.appId, info.userId);
        EXPECT_EQ(mgr.CloseStore(storeIter->second), OK);
        storeIter->second = nullptr;
        stores_.erase(storeIter);
    }
    auto dbIter = sqliteDb_.find(info);
    if (dbIter != sqliteDb_.end()) {
        sqlite3_close_v2(dbIter->second);
        dbIter->second = nullptr;
        sqliteDb_.erase(dbIter);
    }
    auto schemaIter = schemaInfoMap_.find(info);
    if (schemaIter != schemaInfoMap_.end()) {
        schemaInfoMap_.erase(schemaIter);
    }
    return E_OK;
}

void RDBGeneralUt::CloseAllDelegate()
{
    std::lock_guard<std::mutex> lock(storeMutex_);
    for (auto &iter : sqliteDb_) {
        sqlite3_close_v2(iter.second);
        iter.second = nullptr;
    }
    sqliteDb_.clear();
    for (auto &iter : stores_) {
        RelationalStoreManager mgr(iter.first.appId, iter.first.userId);
        EXPECT_EQ(mgr.CloseStore(iter.second), OK);
        iter.second = nullptr;
    }
    stores_.clear();
    schemaInfoMap_.clear();
}

void RDBGeneralUt::SetUp()
{
    BasicUnitTest::SetUp();
}

void RDBGeneralUt::TearDown()
{
    CloseAllDelegate();
    BasicUnitTest::TearDown();
}

int RDBGeneralUt::InitDatabase(const StoreInfo &info)
{
    auto schema = GetSchema(info);
    auto db = GetSqliteHandle(info);
    if (db == nullptr) {
        LOGE("[RDBGeneralUt] Get null sqlite when init database");
        return -E_INVALID_DB;
    }
    auto errCode = RDBDataGenerator::InitDatabase(schema, *db);
    if (errCode != E_OK) {
        LOGE("[RDBGeneralUt] Init db failed %d app %s store %s user %s", errCode, info.appId.c_str(),
            info.storeId.c_str(), info.userId.c_str());
    }
    return errCode;
}

sqlite3 *RDBGeneralUt::GetSqliteHandle(const StoreInfo &info)
{
    std::lock_guard<std::mutex> autoLock(storeMutex_);
    auto db = sqliteDb_.find(info);
    if (db == sqliteDb_.end()) {
        LOGE("[RDBGeneralUt] Not exist sqlite db app %s store %s user %s", info.appId.c_str(),
            info.storeId.c_str(), info.userId.c_str());
        return nullptr;
    }
    return db->second;
}

int RDBGeneralUt::InsertLocalDBData(int64_t begin, int64_t count, const StoreInfo &info)
{
    auto schema = GetSchema(info);
    auto db = GetSqliteHandle(info);
    if (db == nullptr) {
        LOGE("[RDBGeneralUt] Get null sqlite when insert data");
        return -E_INVALID_DB;
    }
    auto errCode = RDBDataGenerator::InsertLocalDBData(begin, begin + count, db, schema);
    if (errCode != E_OK) {
        LOGE("[RDBGeneralUt] Insert data failed %d app %s store %s user %s", errCode, info.appId.c_str(),
            info.storeId.c_str(), info.userId.c_str());
    } else {
        LOGI("[RDBGeneralUt] Insert data success begin %" PRId64 " count %" PRId64 " app %s store %s user %s",
            begin, count, info.appId.c_str(), info.storeId.c_str(), info.userId.c_str());
    }
    return errCode;
}

int RDBGeneralUt::CreateDistributedTable(const StoreInfo &info, const std::string &table, TableSyncType type)
{
    auto store = GetDelegate(info);
    if (store == nullptr) {
        LOGE("[RDBGeneralUt] Get null delegate when create distributed table %s", table.c_str());
        return -E_INVALID_DB;
    }
    auto errCode = store->CreateDistributedTable(table, type);
    if (errCode != E_OK) {
        LOGE("[RDBGeneralUt] Create distributed table failed %d app %s store %s user %s", errCode, info.appId.c_str(),
            info.storeId.c_str(), info.userId.c_str());
    }
    return errCode;
}

int RDBGeneralUt::SetDistributedTables(const StoreInfo &info, const std::vector<std::string> &tables,
    TableSyncType type)
{
    int errCode = E_OK;
    for (const auto &table : tables) {
        errCode = CreateDistributedTable(info, table, type);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    if (option_.tableMode != DistributedTableMode::COLLABORATION) {
        return E_OK;
    }
    auto store = GetDelegate(info);
    if (store == nullptr) {
        LOGE("[RDBGeneralUt] Get null delegate when set distributed tables");
        return -E_INVALID_DB;
    }
    auto schema = GetSchema(info);
    errCode = store->SetDistributedSchema(RDBDataGenerator::ParseSchema(schema));
    if (errCode != E_OK) {
        LOGE("[RDBGeneralUt] Set distributed schema failed %d app %s store %s user %s", errCode, info.appId.c_str(),
            info.storeId.c_str(), info.userId.c_str());
    }
    return errCode;
}

RelationalStoreDelegate *RDBGeneralUt::GetDelegate(const StoreInfo &info)
{
    std::lock_guard<std::mutex> autoLock(storeMutex_);
    auto db = stores_.find(info);
    if (db == stores_.end()) {
        LOGE("[RDBGeneralUt] Not exist delegate app %s store %s user %s", info.appId.c_str(),
             info.storeId.c_str(), info.userId.c_str());
        return nullptr;
    }
    return db->second;
}

void RDBGeneralUt::BlockPush(const StoreInfo &from, const StoreInfo &to, const std::string &table, DBStatus expectRet)
{
    auto store = GetDelegate(from);
    ASSERT_NE(store, nullptr);
    auto toDevice  = GetDevice(to);
    ASSERT_FALSE(toDevice.empty());
    Query query = Query::Select(table);
    DistributedDBToolsUnitTest::BlockSync(*store, query, SYNC_MODE_PUSH_ONLY, expectRet, {toDevice});
}

int RDBGeneralUt::CountTableData(const StoreInfo &info, const std::string &table)
{
    return CountTableDataByDev(info, table, "");
}

int RDBGeneralUt::CountTableDataByDev(const StoreInfo &info, const std::string &table, const std::string &dev)
{
    auto db = GetSqliteHandle(info);
    if (db == nullptr) {
        LOGE("[RDBGeneralUt] Get null sqlite when count table %s", table.c_str());
        return 0;
    }
    bool isCreated = false;
    int errCode = SQLiteUtils::CheckTableExists(db, table, isCreated);
    if (errCode != E_OK) {
        LOGE("[RDBGeneralUt] Check table exist failed %d when count table %s", errCode, table.c_str());
        return 0;
    }
    if (!isCreated) {
        LOGW("[RDBGeneralUt] Check table %s not exist", table.c_str());
        return 0;
    }

    std::string cntSql = "SELECT count(*) FROM '" + table + "'";
    if (!dev.empty()) {
        auto hex = DBCommon::TransferStringToHex(DBCommon::TransferHashString(dev));
        cntSql.append(" WHERE device='").append(hex).append("'");
    }
    sqlite3_stmt *stmt = nullptr;
    errCode = SQLiteUtils::GetStatement(db, cntSql, stmt);
    if (errCode != E_OK) {
        LOGE("[RDBGeneralUt] Check table %s failed by get stmt %d", table.c_str(), errCode);
        return 0;
    }
    int count = 0;
    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        count = sqlite3_column_int(stmt, 0);
        LOGI("[RDBGeneralUt] Count table %s success exist %d data dev %s", table.c_str(), count, dev.c_str());
    } else {
        LOGE("[RDBGeneralUt] Count table %s failed by %d dev %s", table.c_str(), errCode, dev.c_str());
    }

    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return count;
}

int RDBGeneralUt::SetTrackerTables(const StoreInfo &info, const std::vector<std::string> &tables)
{
    auto store = GetDelegate(info);
    if (store == nullptr) {
        LOGE("[RDBGeneralUt] Get null delegate when set tracker tables");
        return -E_INVALID_DB;
    }
    auto trackerSchema = GetAllTrackerSchema(info, tables);
    for (const auto &trackerTable : trackerSchema) {
        auto errCode = store->SetTrackerTable(trackerTable);
        if (errCode != E_OK) {
            LOGE("[RDBGeneralUt] Set tracker table %s failed %d app %s store %s user %s",
                trackerTable.tableName.c_str(), errCode,
                info.appId.c_str(), info.storeId.c_str(), info.userId.c_str());
            return errCode;
        }
    }
    LOGI("[RDBGeneralUt] Set tracker %zu table success app %s store %s user %s",
        trackerSchema.size(), info.appId.c_str(), info.storeId.c_str(), info.userId.c_str());
    return E_OK;
}
}
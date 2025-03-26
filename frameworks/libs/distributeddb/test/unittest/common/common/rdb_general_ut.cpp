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
#include "runtime_config.h"
#include "virtual_cloud_data_translate.h"

using namespace DistributedDBUnitTest;

namespace DistributedDB {
const std::string CIPHER_CONFIG_SQL = "PRAGMA codec_cipher='aes-256-gcm';";
const std::string KDF_ITER_CONFIG_SQL = "PRAGMA codec_kdf_iter=5000;";
const std::string SHA256_ALGO_SQL = "PRAGMA codec_hmac_algo=SHA256;";

const Field intField = {"id", TYPE_INDEX<int64_t>, true, false};
const Field stringField = {"name", TYPE_INDEX<std::string>, false, false};
const Field boolField = {"gender", TYPE_INDEX<bool>, false, true};
const Field doubleField = {"height", TYPE_INDEX<double>, false, true};
const Field bytesField = {"photo", TYPE_INDEX<Bytes>, false, true};
const Field assetField = {"assert", TYPE_INDEX<Asset>, false, true};
const Field assetsField = {"asserts", TYPE_INDEX<Assets>, false, true};

const std::vector<UtFieldInfo> g_defaultFiledInfo = {
    {intField, true}, {stringField, false}, {boolField, false},
    {doubleField, false}, {bytesField, false}, {assetField, false}, {assetsField, false},
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
    int errCode = E_OK;
    if (GetIsDbEncrypted()) {
        errCode = EncryptedDb(db);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    UtDateBaseSchemaInfo schemaInfo = GetTableSchemaInfo(info);
    errCode = RDBDataGenerator::InitDatabaseWithSchemaInfo(schemaInfo, *db);
    if (errCode != SQLITE_OK) {
        LOGE("[RDBGeneralUt] Init database failed %d", errCode);
        return errCode;
    }
    RelationalStoreManager mgr(info.appId, info.userId);
    RelationalStoreDelegate *delegate = nullptr;
    RelationalStoreDelegate::Option option = GetOption();
    errCode = mgr.OpenStore(storePath, info.storeId, option, delegate);
    if ((errCode != E_OK )|| (delegate == nullptr)) {
        LOGE("[RDBGeneralUt] Open store failed %d", errCode);
        return errCode;
    }
    {
        std::lock_guard<std::mutex> autoLock(storeMutex_);
        stores_[info] = delegate;
        sqliteDb_[info] = db;
    }
    LOGI("[RDBGeneralUt] Init delegate app %s store %s user %s success", info.appId.c_str(),
        info.storeId.c_str(), info.userId.c_str());
    return E_OK;
}

void RDBGeneralUt::SetOption(const RelationalStoreDelegate::Option& option)
{
    std::lock_guard<std::mutex> autoLock(storeMutex_);
    option_ = option;
}

RelationalStoreDelegate::Option RDBGeneralUt::GetOption() const
{
    std::lock_guard<std::mutex> autoLock(storeMutex_);
    return option_;
}

void RDBGeneralUt::SetSchemaInfo(const StoreInfo &info, const DistributedDBUnitTest::UtDateBaseSchemaInfo &schemaInfo)
{
    std::lock_guard<std::mutex> autoLock(storeMutex_);
    schemaInfoMap_[info] = schemaInfo;
}

UtDateBaseSchemaInfo RDBGeneralUt::GetTableSchemaInfo(const StoreInfo &info) const
{
    std::lock_guard<std::mutex> autoLock(storeMutex_);
    auto iter = schemaInfoMap_.find(info);
    if (iter != schemaInfoMap_.end()) {
        return iter->second;
    }
    return g_defaultSchemaInfo;
}

DataBaseSchema RDBGeneralUt::GetSchema(const StoreInfo &info) const
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

TableSchema RDBGeneralUt::GetTableSchema(const StoreInfo &info, const std::string &tableName) const
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
    const std::vector<std::string> &tables) const
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
    std::lock_guard<std::mutex> autoLock(storeMutex_);
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
    LOGI("[RDBGeneralUt] Close delegate app %s store %s user %s success", info.appId.c_str(),
        info.storeId.c_str(), info.userId.c_str());
    return E_OK;
}

void RDBGeneralUt::CloseAllDelegate()
{
    std::lock_guard<std::mutex> autoLock(storeMutex_);
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
    isDbEncrypted_ = false;
    LOGI("[RDBGeneralUt] Close all delegate success");
}

void RDBGeneralUt::SetUp()
{
    BasicUnitTest::SetUp();
    RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());
    {
        std::lock_guard<std::mutex> autoLock(storeMutex_);
        virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
        virtualAssetLoader_ = std::make_shared<VirtualAssetLoader>();
    }
}

void RDBGeneralUt::TearDown()
{
    CloseAllDelegate();
    {
        std::lock_guard<std::mutex> autoLock(storeMutex_);
        virtualCloudDb_ = nullptr;
        virtualAssetLoader_ = nullptr;
    }
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

sqlite3 *RDBGeneralUt::GetSqliteHandle(const StoreInfo &info) const
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
    LOGI("[RDBGeneralUt] Create distributed table success, app %s store %s user %s", info.appId.c_str(),
        info.storeId.c_str(), info.userId.c_str());
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
    RelationalStoreDelegate::Option option = GetOption();
    if (option.tableMode != DistributedTableMode::COLLABORATION) {
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
    LOGI("[RDBGeneralUt] Set distributed table success, app %s store %s user %s", info.appId.c_str(),
        info.storeId.c_str(), info.userId.c_str());
    return errCode;
}

RelationalStoreDelegate *RDBGeneralUt::GetDelegate(const StoreInfo &info) const
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

std::shared_ptr<VirtualCloudDb> RDBGeneralUt::GetVirtualCloudDb() const
{
    std::lock_guard<std::mutex> autoLock(storeMutex_);
    return virtualCloudDb_;
}

std::shared_ptr<VirtualAssetLoader> RDBGeneralUt::GetVirtualAssetLoader() const
{
    std::lock_guard<std::mutex> autoLock(storeMutex_);
    return virtualAssetLoader_;
}

void RDBGeneralUt::SetCloudDbConfig(const StoreInfo &info) const
{
    auto delegate = GetDelegate(info);
    ASSERT_NE(delegate, nullptr);
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = GetVirtualCloudDb();
    std::shared_ptr<VirtualAssetLoader> virtualAssetLoader = GetVirtualAssetLoader();
    ASSERT_NE(virtualCloudDb, nullptr);
    ASSERT_NE(virtualAssetLoader, nullptr);
    ASSERT_EQ(delegate->SetCloudDB(virtualCloudDb), DBStatus::OK);
    ASSERT_EQ(delegate->SetIAssetLoader(virtualAssetLoader), DBStatus::OK);
    ASSERT_EQ(delegate->SetCloudDbSchema(GetSchema(info)), DBStatus::OK);
    LOGI("[RDBGeneralUt] Set cloud db config success, app %s store %s user %s", info.appId.c_str(),
        info.storeId.c_str(), info.userId.c_str());
}

void RDBGeneralUt::CloudBlockSync(const StoreInfo &info, const Query &query, DBStatus exceptStatus,
    DBStatus callbackExpect)
{
    LOGI("[RDBGeneralUt] Begin cloud sync, app %s store %s user %s", info.appId.c_str(),
        info.storeId.c_str(), info.userId.c_str());
    auto delegate = GetDelegate(info);
    ASSERT_NE(delegate, nullptr);
    RelationalTestUtils::CloudBlockSync(query, delegate, exceptStatus, callbackExpect);
}

int RDBGeneralUt::GetCloudDataCount(const std::string &tableName) const
{
    VBucket extend;
    extend[CloudDbConstant::CURSOR_FIELD] = std::to_string(0);
    int realCount = 0;
    std::vector<VBucket> data;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = GetVirtualCloudDb();
    if (virtualCloudDb == nullptr) {
        LOGE("[RDBGeneralUt] virtual cloud db is nullptr");
        return -1;
    }
    virtualCloudDb->Query(tableName, extend, data);
    for (size_t j = 0; j < data.size(); ++j) {
        auto entry = data[j].find(CloudDbConstant::DELETE_FIELD);
        if (entry != data[j].end() && std::get<bool>(entry->second)) {
            continue;
        }
        realCount++;
    }
    LOGI("[RDBGeneralUt] Count cloud table %s success, count %d", tableName.c_str(), realCount);
    return realCount;
}

void RDBGeneralUt::SetIsDbEncrypted(bool isdbEncrypted)
{
    isDbEncrypted_ = isdbEncrypted;
}

bool RDBGeneralUt::GetIsDbEncrypted() const
{
    return isDbEncrypted_;
}

int RDBGeneralUt::EncryptedDb(sqlite3 *db)
{
    std::string passwd(PASSWD_VECTOR.begin(), PASSWD_VECTOR.end());
    int rc = sqlite3_key(db, passwd.c_str(), passwd.size());
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        LOGE("sqlite3_key failed, %d, passwd is %s.", rc, passwd.c_str());
        return -E_INVALID_DB;
    }
    char *errMsg = nullptr;
    int errCode = sqlite3_exec(db, CIPHER_CONFIG_SQL.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK || errMsg != nullptr) {
        LOGE("set cipher failed: %d, cipher is %s.", errCode, CIPHER_CONFIG_SQL.c_str());
        sqlite3_close(db);
        sqlite3_free(errMsg);
        return -E_INVALID_DB;
    }
    errMsg = nullptr;
    errCode = sqlite3_exec(db, KDF_ITER_CONFIG_SQL.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK || errMsg != nullptr) {
        LOGE("set iterTimes failed: %d, iterTimes is %s.", errCode, KDF_ITER_CONFIG_SQL.c_str());
        sqlite3_close(db);
        sqlite3_free(errMsg);
        return -E_INVALID_DB;
    }
    errCode = sqlite3_exec(db, SHA256_ALGO_SQL.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK || errMsg != nullptr) {
        LOGE("set codec_hmac_algo failed: %d, codec_hmac_algo is %s.", errCode, SHA256_ALGO_SQL.c_str());
        sqlite3_close(db);
        sqlite3_free(errMsg);
        return -E_INVALID_DB;
    }
    return E_OK;
}
}
/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>
#include "cloud/cloud_storage_utils.h"
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_types.h"
#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "log_print.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "sqlite_relational_utils.h"
#include "time_helper.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"

namespace {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
class DistributedDBCloudReferenceSyncTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
protected:
    void InitTestDir();
    DataBaseSchema GetSchema();
    void CloseDb();
    void SetReference();
    void InsertUserTableRecord(const std::string &tableName, int64_t recordCounts, bool isShared = false,
        int64_t begin = 0, std::string owner = "");
    void UpdateUserTableRecord(const std::string &tableName, int64_t begin, int64_t count);
    void DeleteUserTableRecord(const std::string &tableName, int64_t begin, int64_t count);
    void InsertCloudSharedTableRecord(int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull);
    void UpdateCloudSharedTableRecord(int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull);
    void DeleteCloudSharedTableRecordByGid(int64_t begin, int64_t count);
    void CheckCloudData(const std::string &tableName, bool hasRef, const std::vector<Entries> &refData);
    void CheckDistributedSharedData(const std::vector<std::string> &expect);
    void CheckSharedDataAfterUpdated(const std::vector<double> &expect);
    DataBaseSchema GetSchema(const std::vector<std::string> &tableNames);
    std::vector<std::string> InitMultiTable(int count);
    static void InitWalModeAndTable(sqlite3 *db, const std::vector<std::string> &tableName);
    std::string testDir_;
    std::string storePath_;
    sqlite3 *db_ = nullptr;
    RelationalStoreDelegate *delegate_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    std::shared_ptr<RelationalStoreManager> mgr_ = nullptr;
    const std::string parentTableName_ = "parent";
    const std::string sharedParentTableName_ = "parent_shared";
    const std::string childTableName_ = "child";
    const std::string sharedChildTableName_ = "child_shared";
    const std::vector<std::string> sharedTables_ = { sharedParentTableName_, sharedChildTableName_ };
    const Asset cloudAsset1_ = {
        .version = 2, .name = "Phone", .assetId = "0", .subpath = "/local/sync", .uri = "/cloud/sync",
        .modifyTime = "123456", .createTime = "0", .size = "1024", .hash = "DEC"
    };
    const Asset cloudAsset2_ = {
        .version = 2, .name = "Phone", .assetId = "0", .subpath = "/local/sync", .uri = "/cloud/sync",
        .modifyTime = "123456", .createTime = "0", .size = "1024", .hash = "UPDATE"
    };
};

void DistributedDBCloudReferenceSyncTest::SetUpTestCase()
{
    RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());
}

void DistributedDBCloudReferenceSyncTest::TearDownTestCase()
{}

void DistributedDBCloudReferenceSyncTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    InitTestDir();
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(testDir_) != 0) {
        LOGE("rm test db files error.");
    }
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    LOGD("Test dir is %s", testDir_.c_str());
    db_ = RelationalTestUtils::CreateDataBase(storePath_);
    ASSERT_NE(db_, nullptr);
    InitWalModeAndTable(db_, { parentTableName_, childTableName_ });
    mgr_ = std::make_shared<RelationalStoreManager>(APP_ID, USER_ID);
    RelationalStoreDelegate::Option option;
    ASSERT_EQ(mgr_->OpenStore(storePath_, STORE_ID_1, option, delegate_), DBStatus::OK);
    ASSERT_NE(delegate_, nullptr);
    virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
    ASSERT_EQ(delegate_->SetCloudDB(virtualCloudDb_), DBStatus::OK);
    ASSERT_EQ(delegate_->SetIAssetLoader(std::make_shared<VirtualAssetLoader>()), DBStatus::OK);
    ASSERT_EQ(delegate_->CreateDistributedTable(parentTableName_, CLOUD_COOPERATION), DBStatus::OK);
    ASSERT_EQ(delegate_->CreateDistributedTable(childTableName_, CLOUD_COOPERATION), DBStatus::OK);
    SetReference();
    DataBaseSchema dataBaseSchema = GetSchema();
    ASSERT_EQ(delegate_->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
}

void DistributedDBCloudReferenceSyncTest::TearDown()
{
    virtualCloudDb_->ForkQuery(nullptr);
    CloseDb();
    EXPECT_EQ(sqlite3_close_v2(db_), SQLITE_OK);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(testDir_) != E_OK) {
        LOGE("rm test db files error.");
    }
}

void DistributedDBCloudReferenceSyncTest::InitTestDir()
{
    if (!testDir_.empty()) {
        return;
    }
    DistributedDBToolsUnitTest::TestDirInit(testDir_);
    storePath_ = testDir_ + "/" + STORE_ID_1 + ".db";
    LOGI("The test db is:%s", testDir_.c_str());
}

DataBaseSchema DistributedDBCloudReferenceSyncTest::GetSchema()
{
    DataBaseSchema schema;
    TableSchema tableSchema;
    tableSchema.name = parentTableName_;
    tableSchema.sharedTableName = sharedParentTableName_;
    tableSchema.fields = {
        {"id", TYPE_INDEX<std::string>, true}, {"name", TYPE_INDEX<std::string>}, {"height", TYPE_INDEX<double>},
        {"photo", TYPE_INDEX<Bytes>}, {"age", TYPE_INDEX<int64_t>}
    };
    TableSchema childSchema;
    childSchema.name = childTableName_;
    childSchema.sharedTableName = sharedChildTableName_;
    childSchema.fields = {
        {"id", TYPE_INDEX<std::string>, true}, {"name", TYPE_INDEX<std::string>}, {"height", TYPE_INDEX<double>},
        {"photo", TYPE_INDEX<Bytes>}, {"age", TYPE_INDEX<int64_t>}
    };
    schema.tables.push_back(tableSchema);
    schema.tables.push_back(childSchema);
    return schema;
}

DataBaseSchema DistributedDBCloudReferenceSyncTest::GetSchema(const std::vector<std::string> &tableNames)
{
    DataBaseSchema schema;
    for (const auto &table : tableNames) {
        TableSchema tableSchema;
        tableSchema.name = table;
        tableSchema.sharedTableName = table + "_shared";
        tableSchema.fields = {
            {"id", TYPE_INDEX<std::string>, true}, {"name", TYPE_INDEX<std::string>}, {"height", TYPE_INDEX<double>},
            {"photo", TYPE_INDEX<Bytes>}, {"age", TYPE_INDEX<int64_t>}
        };
        schema.tables.push_back(tableSchema);
    }
    return schema;
}

void DistributedDBCloudReferenceSyncTest::CloseDb()
{
    virtualCloudDb_ = nullptr;
    EXPECT_EQ(mgr_->CloseStore(delegate_), DBStatus::OK);
    delegate_ = nullptr;
    mgr_ = nullptr;
}

void DistributedDBCloudReferenceSyncTest::SetReference()
{
    std::vector<TableReferenceProperty> tableReferenceProperty;
    TableReferenceProperty property;
    property.sourceTableName = childTableName_;
    property.targetTableName = parentTableName_;
    property.columns["name"] = "name";
    tableReferenceProperty.push_back(property);
    delegate_->SetReference(tableReferenceProperty);
}

void DistributedDBCloudReferenceSyncTest::InitWalModeAndTable(sqlite3 *db, const std::vector<std::string> &tableName)
{
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    static constexpr const char *createSQLBegin =  "CREATE TABLE IF NOT EXISTS ";
    static constexpr const char *createSQLEnd =  " (" \
        "id TEXT PRIMARY KEY," \
        "name TEXT," \
        "height REAL ," \
        "photo BLOB," \
        "age INT);";
    for (const auto &table : tableName) {
        std::string createSQL = createSQLBegin + table + createSQLEnd;
        LOGD("[DistributedDBCloudReferenceSyncTest] create sql is %s", createSQL.c_str());
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, createSQL), SQLITE_OK);
    }
}

void DistributedDBCloudReferenceSyncTest::InsertUserTableRecord(const std::string &tableName,
    int64_t recordCounts, bool isShared, int64_t begin, std::string owner)
{
    ASSERT_NE(db_, nullptr);
    for (int64_t i = begin; i < recordCounts; ++i) {
        string sql = "INSERT OR REPLACE INTO " + tableName + " (";
        if (isShared) {
            sql += " cloud_owner, cloud_privilege,";
        }
        sql += " id, name, height, photo, age) VALUES (";
        if (isShared) {
            if (owner.empty()) {
                sql += "'mock_owner', 'true', ";
            } else {
                sql += "'" + owner + "', 'true', ";
            }
        }
        sql += "'" + std::to_string(i) + "', 'Local";
        sql += std::to_string(i) + "', '155.10',  'text', '21');";
        ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    }
}

void DistributedDBCloudReferenceSyncTest::UpdateUserTableRecord(const std::string &tableName, int64_t begin,
    int64_t count)
{
    string updateAge = "UPDATE " + tableName + " SET age = '99' where id in (";
    for (int64_t j = begin; j < begin + count; ++j) {
        updateAge += "'" + std::to_string(j) + "',";
    }
    updateAge.pop_back();
    updateAge += ");";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db_, updateAge), SQLITE_OK);
}

void DistributedDBCloudReferenceSyncTest::DeleteUserTableRecord(const std::string &tableName, int64_t begin,
    int64_t count)
{
    for (int64_t i = begin; i < begin + count; i++) {
        string sql = "Delete from " + tableName + " where id = " + std::to_string(i) + ";";
        ASSERT_EQ(RelationalTestUtils::ExecSql(db_, sql), SQLITE_OK);
    }
}

void DistributedDBCloudReferenceSyncTest::InsertCloudSharedTableRecord(int64_t begin, int64_t count, int64_t photoSize,
    bool assetIsNull)
{
    std::vector<uint8_t> photo(photoSize, 'v');
    std::vector<VBucket> record1;
    std::vector<VBucket> record2;
    std::vector<VBucket> extend1;
    std::vector<VBucket> extend2;
    Timestamp now = TimeHelper::GetSysCurrentTime();
    for (int64_t i = begin; i < begin + count; ++i) {
        VBucket data;
        data.insert_or_assign(std::string("id"), std::to_string(i));
        data.insert_or_assign(std::string("name"), std::string("Cloud") + std::to_string(i));
        data.insert_or_assign(std::string("height"), 166.0); // 166.0 is random double value
        data.insert_or_assign(std::string("photo"), photo);
        data.insert_or_assign(std::string("age"), 13L); // 13 is random int64_t value
        data.insert_or_assign(std::string("cloud_owner"), std::string("a_owner"));
        data.insert_or_assign(std::string("cloud_privilege"), std::string("true"));
        Asset asset = cloudAsset1_;
        asset.name = asset.name + std::to_string(i);
        assetIsNull ? data.insert_or_assign(std::string("asset"), Nil()) :
            data.insert_or_assign(std::string("asset"), asset);
        record1.push_back(data);
        record2.push_back(data);
        VBucket log;
        log.insert_or_assign(CloudDbConstant::CREATE_FIELD,
            static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND + i));
        log.insert_or_assign(CloudDbConstant::MODIFY_FIELD,
            static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND + i));
        log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
        extend1.push_back(log);
        extend2.push_back(log);
    }
    ASSERT_EQ(virtualCloudDb_->BatchInsert(sharedParentTableName_, std::move(record1), extend1), DBStatus::OK);
    ASSERT_EQ(virtualCloudDb_->BatchInsert(sharedChildTableName_, std::move(record2), extend2), DBStatus::OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(count));
}

void DistributedDBCloudReferenceSyncTest::UpdateCloudSharedTableRecord(int64_t begin, int64_t count, int64_t photoSize,
    bool assetIsNull)
{
    std::vector<uint8_t> photo(photoSize, 'v');
    std::vector<VBucket> record1;
    std::vector<VBucket> record2;
    std::vector<VBucket> extend1;
    std::vector<VBucket> extend2;
    Timestamp now = TimeHelper::GetSysCurrentTime();
    for (int64_t i = begin; i < begin + 2 * count; ++i) { // 2 is two tables shared only one gid
        VBucket data;
        data.insert_or_assign("id", std::to_string(i));
        data.insert_or_assign("name", std::string("Cloud") + std::to_string(i));
        data.insert_or_assign("height", 188.0); // 188.0 is random double value
        data.insert_or_assign("photo", photo);
        data.insert_or_assign("age", 13L); // 13 is random int64_t value
        data.insert_or_assign("cloud_owner", std::string("b_owner"));
        data.insert_or_assign("cloud_privilege", std::string("true"));
        Asset asset = cloudAsset2_;
        asset.name = asset.name + std::to_string(i);
        assetIsNull ? data.insert_or_assign("asset", Nil()) : data.insert_or_assign("asset", asset);
        record1.push_back(data);
        record2.push_back(data);
        VBucket log;
        log.insert_or_assign(CloudDbConstant::CREATE_FIELD,
            static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND + i));
        log.insert_or_assign(CloudDbConstant::MODIFY_FIELD,
            static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND + i));
        log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
        log.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(i));
        extend1.push_back(log);
        log.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(++i));
        extend2.push_back(log);
    }
    ASSERT_EQ(virtualCloudDb_->BatchUpdate(sharedParentTableName_, std::move(record1), extend1), DBStatus::OK);
    ASSERT_EQ(virtualCloudDb_->BatchUpdate(sharedChildTableName_, std::move(record2), extend2), DBStatus::OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(count));
}

void DistributedDBCloudReferenceSyncTest::DeleteCloudSharedTableRecordByGid(int64_t begin, int64_t count)
{
    for (size_t i = 0; i < sharedTables_.size(); i++) {
        for (int64_t j = begin; j < begin + count; ++j) {
            VBucket data;
            data.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(j));
            ASSERT_EQ(virtualCloudDb_->DeleteByGid(sharedTables_[i], data), DBStatus::OK);
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(count));
}

void DistributedDBCloudReferenceSyncTest::CheckCloudData(const std::string &tableName, bool hasRef,
    const std::vector<Entries> &refData)
{
    VBucket extend;
    extend[CloudDbConstant::CURSOR_FIELD] = std::string();
    std::vector<VBucket> queryRes;
    (void) virtualCloudDb_->Query(tableName, extend, queryRes);
    if (hasRef) {
        ASSERT_EQ(refData.size(), queryRes.size());
    }
    int index = 0;
    for (const auto &data : queryRes) {
        if (!hasRef) {
            EXPECT_EQ(data.find(CloudDbConstant::REFERENCE_FIELD), data.end());
            continue;
        }
        ASSERT_NE(data.find(CloudDbConstant::REFERENCE_FIELD), data.end());
        Entries entries = std::get<Entries>(data.at(CloudDbConstant::REFERENCE_FIELD));
        EXPECT_EQ(refData[index], entries);
        index++;
    }
}

void DistributedDBCloudReferenceSyncTest::CheckDistributedSharedData(const std::vector<std::string> &expect)
{
    for (size_t i = 0; i < sharedTables_.size(); i++) {
        std::string sql = "SELECT version FROM " + DBCommon::GetLogTableName(sharedTables_[i]) + " WHERE rowid = 1;";
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
        while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_TEXT);
            Type cloudValue;
            ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<std::string>, 0, cloudValue), E_OK);
            std::string versionValue;
            ASSERT_EQ(CloudStorageUtils::GetValueFromOneField(cloudValue, versionValue), E_OK);
            ASSERT_EQ(versionValue, expect[i]);
        }
        int errCode;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
}

void DistributedDBCloudReferenceSyncTest::CheckSharedDataAfterUpdated(const std::vector<double> &expect)
{
    for (size_t i = 0; i < sharedTables_.size(); i++) {
        std::string sql = "SELECT height FROM " + sharedTables_[i] + " WHERE _rowid_ = 1;";
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
        while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_FLOAT);
            Type cloudValue;
            ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<double>, 0, cloudValue), E_OK);
            double heightValue;
            ASSERT_EQ(CloudStorageUtils::GetValueFromOneField(cloudValue, heightValue), E_OK);
            EXPECT_EQ(heightValue, expect[i]);
        }
        int errCode;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
}

std::vector<std::string> DistributedDBCloudReferenceSyncTest::InitMultiTable(int count)
{
    std::vector<std::string> tableName;
    for (int i = 0; i < count; ++i) {
        std::string table = "table_";
        table += static_cast<char>(static_cast<int>('a') + i);
        tableName.push_back(table);
    }
    InitWalModeAndTable(db_, tableName);
    for (const auto &table : tableName) {
        EXPECT_EQ(delegate_->CreateDistributedTable(table, CLOUD_COOPERATION), DBStatus::OK);
        LOGW("table %s", table.c_str());
    }
    return tableName;
}

/**
 * @tc.name: CloudSyncTest001
 * @tc.desc: sync table with reference
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, CloudSyncTest001, TestSize.Level0)
{
    std::vector<std::string> tableNames = { parentTableName_, childTableName_ };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);

    InsertUserTableRecord(parentTableName_, 1);
    InsertUserTableRecord(childTableName_, 1);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    LOGD("check parent table");
    CheckCloudData(parentTableName_, false, {});
    LOGD("check child table");
    std::vector<Entries> expectEntries;
    Entries entries;
    entries[parentTableName_] = "0";
    expectEntries.push_back(entries);
    CheckCloudData(childTableName_, true, expectEntries);
}

/**
 * @tc.name: CloudSyncTest002
 * @tc.desc: sync shared table with reference
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, CloudSyncTest002, TestSize.Level0)
{
    std::vector<std::string> tableNames = { sharedParentTableName_, sharedChildTableName_ };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);

    InsertUserTableRecord(sharedParentTableName_, 1, true);
    InsertUserTableRecord(sharedChildTableName_, 1, true);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    LOGD("check parent table");
    CheckCloudData(sharedParentTableName_, false, {});
    LOGD("check child table");
    std::vector<Entries> expectEntries;
    Entries entries;
    entries[sharedParentTableName_] = "0";
    expectEntries.push_back(entries);
    CheckCloudData(sharedChildTableName_, true, expectEntries);
}

/**
 * @tc.name: CloudSyncTest003
 * @tc.desc: sync with invalid table reference
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, CloudSyncTest003, TestSize.Level0)
{
    std::vector<std::string> tableNames = { childTableName_, parentTableName_ };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_, DistributedDB::INVALID_ARGS);

    tableNames = { sharedChildTableName_, sharedParentTableName_ };
    query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_, DistributedDB::INVALID_ARGS);

    tableNames = { sharedChildTableName_, parentTableName_ };
    query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_, DistributedDB::OK);

    tableNames = { childTableName_, sharedParentTableName_ };
    query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_, DistributedDB::OK);
}

/**
 * @tc.name: CloudSyncTest004
 * @tc.desc: sync shared table and check version, cloud insert, local update, local delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, CloudSyncTest004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. cloud insert records then sync, check distributed shared table
     * @tc.expected: OK.
     */
    int64_t num = 200;
    std::vector<std::string> tableNames = { sharedParentTableName_, sharedChildTableName_ };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    InsertCloudSharedTableRecord(0, num, 10240, true);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    CheckDistributedSharedData({"0", "200"});

    /**
     * @tc.steps: step2. user update records then sync, check distributed shared table
     * @tc.expected: OK.
     */
    UpdateUserTableRecord(sharedParentTableName_, 0, num);
    UpdateUserTableRecord(sharedChildTableName_, 0, num);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    CheckDistributedSharedData({"400", "600"});
    CheckCloudData(sharedParentTableName_, false, {});
    std::vector<Entries> expectEntries;
    Entries entries;
    for (size_t i = 0; i < 100; i++) { // 100 is max cloud query num
        entries[sharedParentTableName_] = std::to_string(i);
        expectEntries.push_back(entries);
    }
    CheckCloudData(sharedChildTableName_, true, expectEntries);

    /**
     * @tc.steps: step3. user delete records then sync, check distributed shared table
     * @tc.expected: OK.
     */
    DeleteUserTableRecord(sharedParentTableName_, 0, 1);
    DeleteUserTableRecord(sharedChildTableName_, 0, 1);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    CheckDistributedSharedData({"400", "600"});
}

/**
 * @tc.name: CloudSyncTest005
 * @tc.desc: sync shared table and check version, local insert, cloud delete, local update
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, CloudSyncTest005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. user insert records then sync, check distributed shared table
     * @tc.expected: OK.
     */
    std::vector<std::string> tableNames = { sharedParentTableName_, sharedChildTableName_ };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    InsertUserTableRecord(sharedParentTableName_, 1, true, 0);
    InsertUserTableRecord(sharedChildTableName_, 1, true, 0);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    CheckDistributedSharedData({"0", "1"});

    /**
     * @tc.steps: step2. user update records then sync, check distributed shared table
     * @tc.expected: OK.
     */
    DeleteCloudSharedTableRecordByGid(0, 2);
    UpdateUserTableRecord(sharedParentTableName_, 0, 1);
    UpdateUserTableRecord(sharedChildTableName_, 0, 1);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    CheckDistributedSharedData({"2", "3"});
    CheckCloudData(sharedParentTableName_, false, {});
    std::vector<Entries> expectEntries;
    Entries entries;
    entries[sharedParentTableName_] = "0";
    expectEntries.push_back(entries);
    entries = {};
    entries[sharedParentTableName_] = "2";
    expectEntries.push_back(entries);
    CheckCloudData(sharedChildTableName_, true, expectEntries);
}

/**
 * @tc.name: CloudSyncTest006
 * @tc.desc: sync shared table and check version, cloud insert, cloud update
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, CloudSyncTest006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. user insert records then sync, check distributed shared table
     * @tc.expected: OK.
     */
    std::vector<std::string> tableNames = { sharedParentTableName_, sharedChildTableName_ };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    InsertCloudSharedTableRecord(0, 1, 1, true);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    CheckDistributedSharedData({"0", "1"});

    /**
     * @tc.steps: step2. cloud update records then sync, check distributed shared table
     * @tc.expected: OK.
     */
    UpdateCloudSharedTableRecord(0, 1, 1, true);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    CheckDistributedSharedData({"2", "3"});
}

/**
 * @tc.name: CloudSyncTest007
 * @tc.desc: there is no gid locally, shared table sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, CloudSyncTest007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. user insert records then sync, check distributed shared table
     * @tc.expected: OK.
     */
    std::vector<std::string> tableNames = { sharedParentTableName_, sharedChildTableName_ };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    InsertUserTableRecord(sharedParentTableName_, 1, true, 0, "a_owner");
    InsertUserTableRecord(sharedChildTableName_, 1, true, 0, "a_owner");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    InsertCloudSharedTableRecord(0, 1, 1, true);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    CheckSharedDataAfterUpdated({166.0, 166.0}); // 166.0 is the height col val on the cloud
}

void ComplexReferenceCheck001SetReference(RelationalStoreDelegate *delegate)
{
    // the reference like this
    // h <-  a
    //    b     c
    //  d   e  f  g
    std::vector<TableReferenceProperty> tableReferenceProperty;
    TableReferenceProperty property;
    property.sourceTableName = "table_d";
    property.targetTableName = "table_b";
    property.columns["name"] = "name";
    tableReferenceProperty.push_back(property);

    property.sourceTableName = "table_e";
    property.targetTableName = "table_b";
    tableReferenceProperty.push_back(property);

    property.sourceTableName = "table_f";
    property.targetTableName = "table_c";
    tableReferenceProperty.push_back(property);

    property.sourceTableName = "table_g";
    property.targetTableName = "table_c";
    tableReferenceProperty.push_back(property);

    property.sourceTableName = "table_b";
    property.targetTableName = "table_a";
    tableReferenceProperty.push_back(property);

    property.sourceTableName = "table_b";
    property.targetTableName = "table_h";
    tableReferenceProperty.push_back(property);

    property.sourceTableName = "table_c";
    property.targetTableName = "table_a";
    tableReferenceProperty.push_back(property);

    property.sourceTableName = "table_a";
    property.targetTableName = "table_h";
    tableReferenceProperty.push_back(property);
    delegate->SetReference(tableReferenceProperty);
}

/**
 * @tc.name: ComplexReferenceCheck001
 * @tc.desc: sync with complex table reference
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, ComplexReferenceCheck001, TestSize.Level0)
{
    auto tableName = InitMultiTable(8); // 8 table
    ComplexReferenceCheck001SetReference(delegate_);
    ASSERT_EQ(delegate_->SetCloudDbSchema(GetSchema(tableName)), DBStatus::OK);

    std::vector<std::string> tableNames = { "table_a", "table_b", "table_d" };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);

    tableNames = { "table_a", "table_b", "table_c", "table_h" };
    query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_, INVALID_ARGS);

    tableNames = { "table_h", "table_e" };
    query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);

    tableNames = { "table_e" };
    query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
}

void ComplexReferenceCheck002SetReference(RelationalStoreDelegate *delegate)
{
    // the reference like this
    // a -> b -> c -> d -> e
    std::vector<TableReferenceProperty> tableReferenceProperty;
    TableReferenceProperty property;
    property.sourceTableName = "table_a";
    property.targetTableName = "table_b";
    property.columns["name"] = "name";
    tableReferenceProperty.push_back(property);

    property.sourceTableName = "table_c";
    property.targetTableName = "table_d";
    tableReferenceProperty.push_back(property);

    property.sourceTableName = "table_d";
    property.targetTableName = "table_e";
    tableReferenceProperty.push_back(property);

    property.sourceTableName = "table_b";
    property.targetTableName = "table_c";
    tableReferenceProperty.push_back(property);

    delegate->SetReference(tableReferenceProperty);
}

/**
 * @tc.name: ComplexReferenceCheck002
 * @tc.desc: sync with complex table reference
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, ComplexReferenceCheck002, TestSize.Level0)
{
    auto tableName = InitMultiTable(5); // 5 table
    ASSERT_EQ(delegate_->SetCloudDbSchema(GetSchema(tableName)), DBStatus::OK);
    ComplexReferenceCheck002SetReference(delegate_);

    std::vector<std::string> tableNames = { "table_a", "table_e" };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_, DistributedDB::INVALID_ARGS);

    tableNames = { "table_e", "table_a" };
    query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);

    tableNames = { "table_a", "table_a_shared" };
    query = Query::Select().FromTable(tableNames);
    std::vector<std::string> actualTables;
    std::set<std::string> addTables;
    virtualCloudDb_->ForkQuery([&addTables, &actualTables](const std::string &table, VBucket &) {
        if (addTables.find(table) != addTables.end()) {
            return ;
        }
        actualTables.push_back(table);
        addTables.insert(table);
    });
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    virtualCloudDb_->ForkQuery(nullptr);
    for (const auto &item : actualTables) {
        LOGW("table is %s", item.c_str());
    }
    ASSERT_EQ(actualTables.size(), tableName.size() * 2); // 2 is table size + shared table size
    // expect res table is table_e table_d ... table_e_shared table_a_shared
    for (size_t i = 0; i < tableName.size(); ++i) {
        size_t expectIndex = tableName.size() - 1 - i;
        size_t expectSharedIndex = expectIndex + tableName.size();
        const std::string actualTable = actualTables[expectIndex];
        const std::string actualSharedTable = actualTables[expectSharedIndex];
        bool equal = (actualTable == tableName[i]);
        equal = equal && (actualSharedTable == (tableName[i] + "_shared"));
        LOGW("index %zu expectIndex %zu expectSharedIndex %zu actualTable %s actualSharedTable %s",
            i, expectIndex, expectSharedIndex, actualTable.c_str(), actualSharedTable.c_str());
        EXPECT_TRUE(equal);
    }
}

/**
 * @tc.name: ComplexReferenceCheck003
 * @tc.desc: sync with complex table reference
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, ComplexReferenceCheck003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. init reference table, and reopen db
     * @tc.expected: OK.
     */
    auto tableName = InitMultiTable(8); // 8 table
    ComplexReferenceCheck001SetReference(delegate_);
    ASSERT_EQ(delegate_->SetCloudDbSchema(GetSchema(tableName)), DBStatus::OK);
    virtualCloudDb_ = nullptr;
    EXPECT_EQ(mgr_->CloseStore(delegate_), DBStatus::OK);
    delegate_ = nullptr;
    RelationalStoreDelegate::Option option;
    ASSERT_EQ(mgr_->OpenStore(storePath_, STORE_ID_1, option, delegate_), DBStatus::OK);
    ASSERT_NE(delegate_, nullptr);
    virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
    ASSERT_EQ(delegate_->SetCloudDB(virtualCloudDb_), DBStatus::OK);
    ASSERT_EQ(delegate_->SetIAssetLoader(std::make_shared<VirtualAssetLoader>()), DBStatus::OK);
    ASSERT_EQ(delegate_->SetCloudDbSchema(GetSchema(tableName)), DBStatus::OK);
    tableName = InitMultiTable(8);

    /**
     * @tc.steps: step2. init local data, sync
     * @tc.expected: OK.
     */
    int64_t num = 10;
    InsertUserTableRecord("table_a", num);
    InsertUserTableRecord("table_b", num);
    InsertUserTableRecord("table_c", num);
    std::vector<std::string> tableNames = { "table_a", "table_b", "table_c" };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);

    /**
     * @tc.steps: step3. operator target table, check source table timeStamp
     * @tc.expected: OK.
     */
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db_, "SELECT max(timestamp) FROM " + DBCommon::GetLogTableName("table_c"),
        stmt), E_OK);
    ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt, false), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int64_t timeStamp = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    std::string updateSql = "UPDATE table_a SET name = '99' where id = 1";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db_, updateSql), SQLITE_OK);
    std::string deleteSql = "DELETE FROM table_a where id = 2";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db_, deleteSql), SQLITE_OK);
    updateSql = "UPDATE table_a SET age = '99' where id = 3";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db_, updateSql), SQLITE_OK);
    stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db_, "SELECT count(*) FROM " + DBCommon::GetLogTableName("table_c") +
        " WHERE timestamp > '" + std::to_string(timeStamp) + "';", stmt), E_OK);
    ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt, false), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int64_t count = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    EXPECT_EQ(count, 2L); // 2 is changed log num
    RelationalTestUtils::CloudBlockSync(query, delegate_);
}

/**
 * @tc.name: ComplexReferenceCheck004
 * @tc.desc: sync with complex table reference
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, ComplexReferenceCheck004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. init table and set reference twice
     * @tc.expected: OK.
     */
    auto tableName = InitMultiTable(8); // 8 table
    ASSERT_EQ(delegate_->SetCloudDbSchema(GetSchema(tableName)), DBStatus::OK);
    ComplexReferenceCheck001SetReference(delegate_);
    ComplexReferenceCheck001SetReference(delegate_);

    /**
     * @tc.steps: step2. init local data, sync
     * @tc.expected: OK.
     */
    InsertUserTableRecord("table_b", 10);
    InsertUserTableRecord("table_d", 10);
    InsertUserTableRecord("table_e", 10);
    std::vector<std::string> tableNames = { "table_b", "table_d", "table_e" };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);

    /**
     * @tc.steps: step3. operator target table, check source table timeStamp
     * @tc.expected: OK.
     */
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db_, "SELECT max(timestamp) FROM " + DBCommon::GetLogTableName("table_d"),
        stmt), E_OK);
    ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt, false), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int64_t timeStamp = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    std::string updateSql = "UPDATE table_b SET name = '99' where id = 4";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db_, updateSql), SQLITE_OK);
    std::string deleteSql = "DELETE FROM table_b where id = 6";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db_, deleteSql), SQLITE_OK);
    updateSql = "UPDATE table_b SET age = '99' where id = 7";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db_, updateSql), SQLITE_OK);
    stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db_, "SELECT count(*) FROM " + DBCommon::GetLogTableName("table_d") +
        " WHERE timestamp > '" + std::to_string(timeStamp) + "';", stmt), E_OK);
    ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt, false), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int64_t count = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    EXPECT_EQ(count, 2L);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
}

/**
 * @tc.name: ComplexReferenceCheck005
 * @tc.desc: sync with complex table reference
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, ComplexReferenceCheck005, TestSize.Level1)
{
    auto tableName = InitMultiTable(26); // 26 table is alphabet count
    ASSERT_EQ(delegate_->SetCloudDbSchema(GetSchema(tableName)), DBStatus::OK);
    std::vector<TableReferenceProperty> tableReferenceProperty;
    TableReferenceProperty property;
    property.columns["name"] = "name";
    for (size_t i = 0; i < tableName.size() - 1; ++i) {
        property.sourceTableName = tableName[i];
        property.targetTableName = tableName[i + 1];
        tableReferenceProperty.push_back(property);
    }
    delegate_->SetReference(tableReferenceProperty);

    std::vector<std::string> tableNames = { "table_e" };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
}

/**
 * @tc.name: ComplexReferenceCheck006
 * @tc.desc: sync with upper table reference
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, ComplexReferenceCheck006, TestSize.Level1)
{
    auto tableName = InitMultiTable(2); // 2 table is alphabet count
    ASSERT_EQ(delegate_->SetCloudDbSchema(GetSchema(tableName)), DBStatus::OK);
    std::vector<TableReferenceProperty> tableReferenceProperty;
    TableReferenceProperty property;
    property.columns["name"] = "name";
    for (size_t i = 0; i < tableName.size() - 1; ++i) {
        property.sourceTableName = tableName[i];
        property.targetTableName = DBCommon::ToUpperCase(tableName[i + 1]);
        tableReferenceProperty.push_back(property);
    }
    delegate_->SetReference(tableReferenceProperty);

    std::vector<std::string> tableNames = { "table_a" };
    Query query = Query::Select().FromTable(tableNames);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
}

/**
 * @tc.name: SetSharedReference001
 * @tc.desc: test set shared table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudReferenceSyncTest, SetSharedReference001, TestSize.Level0)
{
    std::vector<TableReferenceProperty> tableReferenceProperty;
    TableReferenceProperty property;
    property.columns["name"] = "name";
    property.sourceTableName = childTableName_;
    property.targetTableName = sharedParentTableName_;
    tableReferenceProperty.push_back(property);
    EXPECT_EQ(delegate_->SetReference(tableReferenceProperty), NOT_SUPPORT);

    property.sourceTableName = sharedChildTableName_;
    property.targetTableName = parentTableName_;
    tableReferenceProperty.clear();
    tableReferenceProperty.push_back(property);
    EXPECT_EQ(delegate_->SetReference(tableReferenceProperty), NOT_SUPPORT);

    property.sourceTableName = sharedChildTableName_;
    property.targetTableName = sharedParentTableName_;
    tableReferenceProperty.clear();
    tableReferenceProperty.push_back(property);
    EXPECT_EQ(delegate_->SetReference(tableReferenceProperty), NOT_SUPPORT);
}
}
#endif // RELATIONAL_STORE
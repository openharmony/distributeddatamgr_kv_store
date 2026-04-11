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

#include "rdb_general_ut.h"

#ifdef USE_DISTRIBUTEDDB_DEVICE
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBRDBAbnormalTableTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    void InitDefaultTable(const StoreInfo &info);
    void InitTableWithDiffFieldType(const StoreInfo &info);
    void InitTableWithPK1(const StoreInfo &info);
    void InitTableWithPK2(const StoreInfo &info);
    void AbnormalTable001(bool isLocalError);
    void AbnormalTable005(bool isLocalError);
    void AbnormalTable008(bool isLocalError);
    void AbnormalTable010(bool isLocalError);
    void AbnormalTable013(bool isLocalError);
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtDateBaseSchemaInfo GetMultiTableSchema();
    static UtDateBaseSchemaInfo GetSchemaWithTable2();
    static UtDateBaseSchemaInfo GetSchemaWithPK1();
    static UtDateBaseSchemaInfo GetSchemaWithPK2();
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
    static constexpr const char *TEST_TABLE = "TEST_TABLE";
    static constexpr const char *TEST_TABLE2 = "TEST_TABLE2";
    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    StoreInfo info2_ = {USER_ID, APP_ID, STORE_ID_2};
};

void DistributedDBRDBAbnormalTableTest::SetUp()
{
    RDBGeneralUt::SetUp();
}

void DistributedDBRDBAbnormalTableTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

void DistributedDBRDBAbnormalTableTest::InitDefaultTable(const StoreInfo &info)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + std::string(TEST_TABLE) +
        "(C1 INTEGER PRIMARY KEY, C2 INTEGER, C3 INTEGER);";
    EXPECT_EQ(ExecuteSQL(sql, info), E_OK);
}

void DistributedDBRDBAbnormalTableTest::InitTableWithDiffFieldType(const StoreInfo &info)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + std::string(TEST_TABLE) +
        "(C1 INTEGER PRIMARY KEY, C2 INTEGER, C3 TEXT);";
    EXPECT_EQ(ExecuteSQL(sql, info), E_OK);
}

void DistributedDBRDBAbnormalTableTest::InitTableWithPK1(const StoreInfo &info)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + std::string(TEST_TABLE) +
        "(C1 INTEGER PRIMARY KEY AUTOINCREMENT, C2 INTEGER UNIQUE, C3 INTEGER);";
    EXPECT_EQ(ExecuteSQL(sql, info), E_OK);
}

void DistributedDBRDBAbnormalTableTest::InitTableWithPK2(const StoreInfo &info)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + std::string(TEST_TABLE) +
        "(C1 INTEGER PRIMARY KEY AUTOINCREMENT, C2 INTEGER, C3 INTEGER UNIQUE);";
    EXPECT_EQ(ExecuteSQL(sql, info), E_OK);
}

void DistributedDBRDBAbnormalTableTest::AbnormalTable001(bool isLocalError)
{
    /**
     * @tc.steps: step1. Init delegate with collaboration mode and create tables.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetDefaultSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);

    /**
     * @tc.steps: step2. A creates distributed table, B sets empty distributed schema.
     * @tc.expected: step2. Ok
     */
    if (isLocalError) {
        ASSERT_EQ(SetDistributedTables(info2_, {TEST_TABLE}), E_OK);
    } else {
        ASSERT_EQ(SetDistributedTables(info1_, {TEST_TABLE}), E_OK);
    }

    /**
     * @tc.steps: step3. Insert data on A.
     * @tc.expected: step3. Ok
     */
    std::string sql = "INSERT INTO " + std::string(TEST_TABLE) + " VALUES(1, 100, 200)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);

    /**
     * @tc.steps: step4. A initiates PUSH sync to B.
     * @tc.expected: step4. PUSH failed with DISTRIBUTED_SCHEMA_NOT_FOUND
     */
    if (isLocalError) {
        DistributedDBToolsUnitTest::SetExpectSyncStatus(DISTRIBUTED_SCHEMA_NOT_FOUND);
        BlockPush(info1_, info2_, TEST_TABLE, DISTRIBUTED_SCHEMA_NOT_FOUND);
        DistributedDBToolsUnitTest::SetExpectSyncStatus(OK);
    } else {
        BlockPush(info1_, info2_, TEST_TABLE, DISTRIBUTED_SCHEMA_NOT_FOUND);
    }
}

void DistributedDBRDBAbnormalTableTest::AbnormalTable005(bool isLocalError)
{
    /**
     * @tc.steps: step1. Init delegate with collaboration mode and create tables.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    SetSchemaInfo(info1_, GetMultiTableSchema());
    SetSchemaInfo(info2_, GetMultiTableSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);

    /**
     * @tc.steps: step2. A creates distributed tables {TEST_TABLE, TEST_TABLE2}, B only creates {TEST_TABLE}.
     * @tc.expected: step2. Ok
     */
    if (isLocalError) {
        ASSERT_EQ(SetDistributedTables(info1_, {TEST_TABLE2}), E_OK);
        ASSERT_EQ(SetDistributedTables(info2_, {TEST_TABLE, TEST_TABLE2}), E_OK);
    } else {
        ASSERT_EQ(SetDistributedTables(info1_, {TEST_TABLE, TEST_TABLE2}), E_OK);
        ASSERT_EQ(SetDistributedTables(info2_, {TEST_TABLE}), E_OK);
    }

    /**
     * @tc.steps: step3. Insert data on A.
     * @tc.expected: step3. Ok
     */
    std::string sql = "INSERT INTO " + std::string(TEST_TABLE) + " VALUES(1)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    sql = "INSERT INTO " + std::string(TEST_TABLE2) + " VALUES(1)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);

    /**
     * @tc.steps: step4. A initiates PUSH sync to B.
     * @tc.expected: step4. PUSH failed with DISTRIBUTED_SCHEMA_NOT_FOUND
     */
    auto store = GetDelegate(info1_);
    ASSERT_NE(store, nullptr);
    auto toDevice  = GetDevice(info2_);
    ASSERT_FALSE(toDevice.empty());
    Query query = Query::Select().FromTable({ TEST_TABLE, TEST_TABLE2 });
    if (isLocalError) {
        DistributedDBToolsUnitTest::SetExpectSyncStatus(DISTRIBUTED_SCHEMA_NOT_FOUND);
        ASSERT_NO_FATAL_FAILURE(DistributedDBToolsUnitTest::BlockSync(*store, query, SyncMode::SYNC_MODE_PUSH_ONLY,
            DISTRIBUTED_SCHEMA_NOT_FOUND, {toDevice}));
        DistributedDBToolsUnitTest::SetExpectSyncStatus(OK);
    } else {
        ASSERT_NO_FATAL_FAILURE(DistributedDBToolsUnitTest::BlockSync(*store, query, SyncMode::SYNC_MODE_PUSH_ONLY,
            DISTRIBUTED_SCHEMA_NOT_FOUND, {toDevice}));
    }
}

void DistributedDBRDBAbnormalTableTest::AbnormalTable008(bool isLocalError)
{
    /**
     * @tc.steps: step1. Init delegate with collaboration mode and create tables.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetDefaultSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);

    /**
     * @tc.steps: step2. A creates distributed table, B only sets distributed schema without SetDistributedTables.
     * @tc.expected: step2. Ok
     */
    if (isLocalError) {
        ASSERT_EQ(SetDistributedTables(info2_, {TEST_TABLE}), E_OK);
        auto store = GetDelegate(info1_);
        auto schema = GetSchema(info1_);
        EXPECT_EQ(store->SetDistributedSchema(RDBDataGenerator::ParseSchema(schema)), OK);
    } else {
        ASSERT_EQ(SetDistributedTables(info1_, {TEST_TABLE}), E_OK);
        auto store = GetDelegate(info2_);
        auto schema = GetSchema(info2_);
        EXPECT_EQ(store->SetDistributedSchema(RDBDataGenerator::ParseSchema(schema)), OK);
    }

    /**
     * @tc.steps: step3. Insert data on A.
     * @tc.expected: step3. Ok
     */
    std::string sql = "INSERT INTO " + std::string(TEST_TABLE) + " VALUES(1, 100, 200)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);

    /**
     * @tc.steps: step4. A initiates PUSH sync to B.
     * @tc.expected: step4. PUSH failed with DISTRIBUTED_SCHEMA_NOT_FOUND
     */
    if (isLocalError) {
        DistributedDBToolsUnitTest::SetExpectSyncStatus(DISTRIBUTED_SCHEMA_NOT_FOUND);
        BlockPush(info1_, info2_, TEST_TABLE, DISTRIBUTED_SCHEMA_NOT_FOUND);
        DistributedDBToolsUnitTest::SetExpectSyncStatus(OK);
    } else {
        BlockPush(info1_, info2_, TEST_TABLE, DISTRIBUTED_SCHEMA_NOT_FOUND);
    }
}

void DistributedDBRDBAbnormalTableTest::AbnormalTable010(bool isLocalError)
{
    /**
     * @tc.steps: step1. Init delegate with collaboration mode and create tables.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetDefaultSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);

    /**
     * @tc.steps: step2. One side sets empty distributed schema.
     * @tc.expected: step2. Ok
     */
    if (isLocalError) {
        SetSchemaInfo(info1_, {});
        ASSERT_EQ(SetDistributedTables(info2_, {TEST_TABLE}), E_OK);
        SetDistributedTables(info1_, {TEST_TABLE});
    } else {
        SetSchemaInfo(info2_, {});
        ASSERT_EQ(SetDistributedTables(info1_, {TEST_TABLE}), E_OK);
        SetDistributedTables(info2_, {TEST_TABLE});
    }

    /**
     * @tc.steps: step3. Insert data on A.
     * @tc.expected: step3. Ok
     */
    std::string sql = "INSERT INTO " + std::string(TEST_TABLE) + " VALUES(1, 100, 200)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);

    /**
     * @tc.steps: step4. A initiates PUSH sync to B.
     * @tc.expected: step4. PUSH failed with DISTRIBUTED_SCHEMA_NOT_FOUND
     */
    if (isLocalError) {
        DistributedDBToolsUnitTest::SetExpectSyncStatus(DISTRIBUTED_SCHEMA_MISMATCH);
        BlockPush(info1_, info2_, TEST_TABLE, DISTRIBUTED_SCHEMA_MISMATCH);
        DistributedDBToolsUnitTest::SetExpectSyncStatus(OK);
    } else {
        BlockPush(info1_, info2_, TEST_TABLE, DISTRIBUTED_SCHEMA_MISMATCH);
    }
}

void DistributedDBRDBAbnormalTableTest::AbnormalTable013(bool isLocalError)
{
    /**
     * @tc.steps: step1. Init delegate with collaboration mode and create tables.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetDefaultSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);

    /**
     * @tc.steps: step2. A and B create distributed tables.
     * @tc.expected: step2. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {TEST_TABLE}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, {TEST_TABLE}), E_OK);

    /**
     * @tc.steps: step3. One side sets abnormal trigger that causes constraint violation on insert.
     * @tc.expected: step3. Ok
     */
    sqlite3 *db = GetSqliteHandle(isLocalError ? info1_ : info2_);
    ASSERT_NE(db, nullptr);
    std::string triggerSql = "CREATE TRIGGER constraint_trigger BEFORE INSERT ON " + std::string(TEST_TABLE) +
        " BEGIN SELECT RAISE(ABORT, 'constraint violation') WHERE NEW.C2 < 0; END;";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, triggerSql), E_OK);

    /**
     * @tc.steps: step4. Insert data that will violate the constraint.
     * @tc.expected: step4. Ok
     */
    std::string sql = "INSERT INTO " + std::string(TEST_TABLE) + " VALUES(1, -100, 200)";
    EXPECT_EQ(ExecuteSQL(sql, isLocalError ? info2_ : info1_), E_OK);

    /**
     * @tc.steps: step5. Sync between A and B.
     * @tc.expected: step5. Sync failed with CONSTRAINT
     */
    if (isLocalError) {
        BlockPull(info1_, info2_, TEST_TABLE, CONSTRAINT);
    } else {
        BlockPush(info1_, info2_, TEST_TABLE, CONSTRAINT);
    }
}

UtDateBaseSchemaInfo DistributedDBRDBAbnormalTableTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    UtTableSchemaInfo tableSchema;
    tableSchema.name = TEST_TABLE;
    UtFieldInfo field1;
    field1.field.colName = "C1";
    field1.field.type = TYPE_INDEX<int64_t>;
    field1.field.primary = true;
    tableSchema.fieldInfo.push_back(field1);
    UtFieldInfo field2;
    field2.field.colName = "C2";
    field2.field.type = TYPE_INDEX<int64_t>;
    field2.field.primary = false;
    tableSchema.fieldInfo.push_back(field2);
    UtFieldInfo field3;
    field3.field.colName = "C3";
    field3.field.type = TYPE_INDEX<int64_t>;
    field3.field.primary = false;
    tableSchema.fieldInfo.push_back(field3);
    info.tablesInfo.push_back(tableSchema);
    return info;
}

UtDateBaseSchemaInfo DistributedDBRDBAbnormalTableTest::GetMultiTableSchema()
{
    UtDateBaseSchemaInfo info;
    UtTableSchemaInfo tableSchema;
    tableSchema.name = TEST_TABLE;
    UtFieldInfo field1;
    field1.field.colName = "C1";
    field1.field.type = TYPE_INDEX<int64_t>;
    field1.field.primary = true;
    tableSchema.fieldInfo.push_back(field1);
    info.tablesInfo.push_back(tableSchema);

    UtTableSchemaInfo tableSchema2;
    tableSchema2.name = TEST_TABLE2;
    tableSchema2.fieldInfo.push_back(field1);
    info.tablesInfo.push_back(tableSchema2);
    return info;
}

UtDateBaseSchemaInfo DistributedDBRDBAbnormalTableTest::GetSchemaWithTable2()
{
    UtDateBaseSchemaInfo info;
    UtTableSchemaInfo tableSchema;
    tableSchema.name = TEST_TABLE2;
    UtFieldInfo field1;
    field1.field.colName = "C1";
    field1.field.type = TYPE_INDEX<int64_t>;
    field1.field.primary = true;
    tableSchema.fieldInfo.push_back(field1);
    info.tablesInfo.push_back(tableSchema);
    return info;
}

UtDateBaseSchemaInfo DistributedDBRDBAbnormalTableTest::GetSchemaWithPK1()
{
    UtDateBaseSchemaInfo info;
    UtTableSchemaInfo tableSchema;
    tableSchema.name = TEST_TABLE;
    UtFieldInfo field2;
    field2.field.colName = "C2";
    field2.field.type = TYPE_INDEX<int64_t>;
    field2.field.primary = true;
    tableSchema.fieldInfo.push_back(field2);
    info.tablesInfo.push_back(tableSchema);
    return info;
}

UtDateBaseSchemaInfo DistributedDBRDBAbnormalTableTest::GetSchemaWithPK2()
{
    UtDateBaseSchemaInfo info;
    UtTableSchemaInfo tableSchema;
    tableSchema.name = TEST_TABLE;
    UtFieldInfo field3;
    field3.field.colName = "C3";
    field3.field.type = TYPE_INDEX<int64_t>;
    field3.field.primary = true;
    tableSchema.fieldInfo.push_back(field3);
    info.tablesInfo.push_back(tableSchema);
    return info;
}

/**
 * @tc.name: AbnormalTable001
 * @tc.desc: Test PUSH sync failed when B does not create distributed table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable001, TestSize.Level0)
{
    AbnormalTable001(false);
}

/**
 * @tc.name: AbnormalTable002
 * @tc.desc: Test PUSH sync failed when A does not create distributed table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable002, TestSize.Level0)
{
    AbnormalTable001(true);
}

/**
 * @tc.name: AbnormalTable003
 * @tc.desc: Test PUSH sync failed when table field type mismatch between A and B.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate with collaboration mode.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetDefaultSchema());

    /**
     * @tc.steps: step2. A creates table T(C1 int, C2 int), B creates table T(C1 int, C2 TEXT).
     * @tc.expected: step2. Ok
     */
    InitDefaultTable(info1_);
    InitTableWithDiffFieldType(info2_);

    /**
     * @tc.steps: step3. A and B create distributed tables.
     * @tc.expected: step3. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {TEST_TABLE}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, {TEST_TABLE}), E_OK);

    /**
     * @tc.steps: step4. Insert data on A.
     * @tc.expected: step4. Ok
     */
    std::string sql = "INSERT INTO " + std::string(TEST_TABLE) + " VALUES(1, 100, 200)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);

    /**
     * @tc.steps: step5. A initiates PUSH sync to B.
     * @tc.expected: step5. PUSH failed with TABLE_FIELD_MISMATCH
     */
    BlockPush(info1_, info2_, TEST_TABLE, TABLE_FIELD_MISMATCH);
}

/**
 * @tc.name: AbnormalTable004
 * @tc.desc: Test PUSH sync failed when distributed schema PK mismatch between A and B.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate with collaboration mode.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);
    SetSchemaInfo(info1_, GetSchemaWithPK1());
    SetSchemaInfo(info2_, GetSchemaWithPK2());

    /**
     * @tc.steps: step2. A creates table with PK(C1, C2), B creates table with PK(C1, C3).
     * @tc.expected: step2. Ok
     */
    InitTableWithPK1(info1_);
    InitTableWithPK2(info2_);

    /**
     * @tc.steps: step3. A and B create distributed tables.
     * @tc.expected: step3. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {TEST_TABLE}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, {TEST_TABLE}), E_OK);

    /**
     * @tc.steps: step4. Insert data on A.
     * @tc.expected: step4. Ok
     */
    std::string sql = "INSERT INTO " + std::string(TEST_TABLE) + " VALUES(1, 100, 200)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);

    /**
     * @tc.steps: step5. A initiates PUSH sync to B.
     * @tc.expected: step5. PUSH failed with DISTRIBUTED_SCHEMA_MISMATCH
     */
    BlockPush(info1_, info2_, TEST_TABLE, DISTRIBUTED_SCHEMA_MISMATCH);
}

/**
 * @tc.name: AbnormalTable005
 * @tc.desc: Test PUSH sync failed when B has trigger that causes constraint violation.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable005, TestSize.Level0)
{
    AbnormalTable013(false);
}

/**
 * @tc.name: AbnormalTable006
 * @tc.desc: Test PUSH sync failed when B creates partial distributed tables.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable006, TestSize.Level0)
{
    AbnormalTable005(false);
}

/**
 * @tc.name: AbnormalTable007
 * @tc.desc: Test PUSH sync failed when A creates partial distributed tables.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable007, TestSize.Level0)
{
    AbnormalTable005(true);
}

/**
 * @tc.name: AbnormalTable008
 * @tc.desc: Test PUSH sync failed when B sets distributed schema without creating distributed table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable008, TestSize.Level0)
{
    AbnormalTable008(false);
}

/**
 * @tc.name: AbnormalTable009
 * @tc.desc: Test PUSH sync failed when A sets distributed schema without creating distributed table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable009, TestSize.Level0)
{
    AbnormalTable008(true);
}

/**
 * @tc.name: AbnormalTable010
 * @tc.desc: Test PUSH sync failed when B has empty distributed schema while A has valid schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable010, TestSize.Level0)
{
    AbnormalTable010(false);
}

/**
 * @tc.name: AbnormalTable011
 * @tc.desc: Test PUSH sync failed when A has empty distributed schema while B has valid schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable011, TestSize.Level0)
{
    AbnormalTable010(true);
}

/**
 * @tc.name: AbnormalTable012
 * @tc.desc: Test PUSH sync failed when A syncs table1 to B but B only has table2 distributed schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable012, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate with collaboration mode and create tables.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetSchemaWithTable2());
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);

    /**
     * @tc.steps: step2. A creates distributed table TEST_TABLE, B creates distributed table TEST_TABLE2.
     * @tc.expected: step2. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {TEST_TABLE}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, {TEST_TABLE2}), E_OK);

    /**
     * @tc.steps: step3. Insert data on A.
     * @tc.expected: step3. Ok
     */
    std::string sql = "INSERT INTO " + std::string(TEST_TABLE) + " VALUES(1, 100, 200)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);

    /**
     * @tc.steps: step4. A initiates PUSH sync to B.
     * @tc.expected: step4. PUSH failed with DISTRIBUTED_SCHEMA_NOT_FOUND
     */
    BlockPush(info1_, info2_, TEST_TABLE, DISTRIBUTED_SCHEMA_NOT_FOUND);
}

/**
 * @tc.name: AbnormalTable013
 * @tc.desc: Test PUSH sync failed when A has trigger that causes constraint violation.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBRDBAbnormalTableTest, AbnormalTable013, TestSize.Level0)
{
    AbnormalTable013(true);
}
} // namespace
#endif // USE_DISTRIBUTEDDB_DEVICE

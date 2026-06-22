/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OMIT_JSON
#include <gtest/gtest.h>
#include <cmath>

#include "db_errno.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "relational_schema_object.h"
#include "schema_utils.h"
#include "schema_constant.h"
#include "schema_negotiate.h"

using namespace std;
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
    constexpr const char* DB_SUFFIX = ".db";
    constexpr const char* STORE_ID = "Relational_Store_ID";
    string g_testDir;
    string g_dbDir;

    const std::string NORMAL_SCHEMA = R""({
            "SCHEMA_VERSION": "2.0",
            "SCHEMA_TYPE": "RELATIVE",
            "TABLES": [{
                "NAME": "FIRST",
                "DEFINE": {
                    "field_name1": {
                        "COLUMN_ID":1,
                        "TYPE": "STRING",
                        "NOT_NULL": true,
                        "DEFAULT": "abcd"
                    },
                    "field_name2": {
                        "COLUMN_ID":2,
                        "TYPE": "MYINT(21)",
                        "NOT_NULL": false,
                        "DEFAULT": "222"
                    },
                    "field_name3": {
                        "COLUMN_ID":3,
                        "TYPE": "INTGER",
                        "NOT_NULL": false,
                        "DEFAULT": "1"
                    }
                },
                "AUTOINCREMENT": true,
                "UNIQUE": [["field_name1"], ["field_name2", "field_name3"]],
                "PRIMARY_KEY": "field_name1",
                "INDEX": {
                    "index_name1": ["field_name1", "field_name2"],
                    "index_name2": ["field_name3"]
                }
            }, {
                "NAME": "SECOND",
                "DEFINE": {
                    "key": {
                        "COLUMN_ID":1,
                        "TYPE": "BLOB",
                        "NOT_NULL": true
                    },
                    "value": {
                        "COLUMN_ID":2,
                        "TYPE": "BLOB",
                        "NOT_NULL": false
                    }
                },
                "PRIMARY_KEY": "field_name1"
            }]
        })"";

    const std::string NORMAL_SCHEMA_V2_1 = R""({
            "SCHEMA_VERSION": "2.1",
            "SCHEMA_TYPE": "RELATIVE",
            "TABLE_MODE": "SPLIT_BY_DEVICE",
            "TABLES": [{
                "NAME": "FIRST",
                "DEFINE": {
                    "field_name1": {
                        "COLUMN_ID":1,
                        "TYPE": "STRING",
                        "NOT_NULL": true,
                        "DEFAULT": "abcd"
                    },
                    "field_name2": {
                        "COLUMN_ID":2,
                        "TYPE": "MYINT(21)",
                        "NOT_NULL": false,
                        "DEFAULT": "222"
                    },
                    "field_name3": {
                        "COLUMN_ID":3,
                        "TYPE": "INTGER",
                        "NOT_NULL": false,
                        "DEFAULT": "1"
                    }
                },
                "AUTOINCREMENT": true,
                "UNIQUE": [["field_name1"], ["field_name2", "field_name3"]],
                "PRIMARY_KEY": ["field_name1", "field_name3"],
                "INDEX": {
                    "index_name1": ["field_name1", "field_name2"],
                    "index_name2": ["field_name3"]
                }
            }, {
                "NAME": "SECOND",
                "DEFINE": {
                    "key": {
                        "COLUMN_ID":1,
                        "TYPE": "BLOB",
                        "NOT_NULL": true
                    },
                    "value": {
                        "COLUMN_ID":2,
                        "TYPE": "BLOB",
                        "NOT_NULL": false
                    }
                },
                "PRIMARY_KEY": ["field_name1"]
            }]
        })"";

    const std::string INVALID_SCHEMA = R""({
            "SCHEMA_VERSION": "2.0",
            "SCHEMA_TYPE": "RELATIVE",
            "TABLES": [{
                "NAME": "FIRST",
                "DEFINE": {
                    "field_name1": {
                        "COLUMN_ID":1,
                        "TYPE": "STRING",
                        "NOT_NULL": true,
                        "DEFAULT": "abcd"
                    },"field_name2": {
                        "COLUMN_ID":2,
                        "TYPE": "MYINT(21)",
                        "NOT_NULL": false,
                        "DEFAULT": "222"
                    }
                },
                "PRIMARY_KEY": "field_name1"
            }]
        })"";

    const std::string INVALID_JSON_STRING = R""({
            "SCHEMA_VERSION": "2.0",
            "SCHEMA_TYPE": "RELATIVE",
            "TABLES": [{
                "NAME": "FIRST",
                "DEFINE": {
                    "field_name1": {)"";

    const std::string SCHEMA_VERSION_STR_1 = R"("SCHEMA_VERSION": "1.0",)";
    const std::string SCHEMA_VERSION_STR_2 = R"("SCHEMA_VERSION": "2.0",)";
    const std::string SCHEMA_VERSION_STR_2_1 = R"("SCHEMA_VERSION": "2.1",)";
    const std::string SCHEMA_VERSION_STR_INVALID = R"("SCHEMA_VERSION": "awd3",)";
    const std::string SCHEMA_TYPE_STR_NONE = R"("SCHEMA_TYPE": "NONE",)";
    const std::string SCHEMA_TYPE_STR_JSON = R"("SCHEMA_TYPE": "JSON",)";
    const std::string SCHEMA_TYPE_STR_FLATBUFFER = R"("SCHEMA_TYPE": "FLATBUFFER",)";
    const std::string SCHEMA_TYPE_STR_RELATIVE = R"("SCHEMA_TYPE": "RELATIVE",)";
    const std::string SCHEMA_TYPE_STR_INVALID = R"("SCHEMA_TYPE": "adewaaSAD",)";
    const std::string SCHEMA_TABLE_MODE_COLLABORATION = R"("TABLE_MODE": "COLLABORATION",)";
    const std::string SCHEMA_TABLE_MODE_SPLIT_BY_DEVICE = R"("TABLE_MODE": "SPLIT_BY_DEVICE",)";
    const std::string SCHEMA_TABLE_MODE_INVALID = R"("TABLE_MODE": "SPLIT_BY_USER",)";
    const std::string DISTRIBUTED_VALID_VERSION = R"("DISTRIBUTED_SCHEMA": { "VERSION": 1 })";
    const std::string DISTRIBUTED_INVALID_SMALL_VERSION = R"("DISTRIBUTED_SCHEMA": { "VERSION": -1 })";
    const std::string DISTRIBUTED_INVALID_LARGE_VERSION = R"("DISTRIBUTED_SCHEMA": { "VERSION": 5000000000 })";
    const std::string DISTRIBUTED_INVALID_NAN_VERSION = R"("DISTRIBUTED_SCHEMA": { "VERSION": "not a number" })";

    const std::string SCHEMA_TABLE_STR = R""("TABLES": [{
            "NAME": "FIRST",
            "DEFINE": {
                "field_name1": {
                    "COLUMN_ID":1,
                    "TYPE": "STRING",
                    "NOT_NULL": true,
                    "DEFAULT": "abcd"
                },"field_name2": {
                    "COLUMN_ID":2,
                    "TYPE": "MYINT(21)",
                    "NOT_NULL": false,
                    "DEFAULT": "222"
                }
            },
            "PRIMARY_KEY": "field_name1"
        }])"";

    const std::string TABLE_DEFINE_STR = R""({
        "NAME": "FIRST",
        "DEFINE": {
            "field_name1": {
                "COLUMN_ID":1,
                "TYPE": "STRING",
                "NOT_NULL": true,
                "DEFAULT": "abcd"
            },"field_name2": {
                "COLUMN_ID":2,
                "TYPE": "MYINT(21)",
                "NOT_NULL": false,
                "DEFAULT": "222"
            }
        },
        "PRIMARY_KEY": "field_name1"
    })"";

    const std::string TABLE_DEFINE_STR_NAME = R""("NAME": "FIRST",)"";
    const std::string TABLE_DEFINE_STR_NAME_INVALID = R"("NAME": 123,)";
    const std::string TABLE_DEFINE_STR_NAME_INVALID_CHARACTER = R"("NAME": "t1; --",)";
    const std::string TABLE_DEFINE_STR_FIELDS = R""("DEFINE": {
            "field_name1": {
                "COLUMN_ID":1,
                "TYPE": "STRING",
                "NOT_NULL": true,
                "DEFAULT": "abcd"
            },"field_name2": {
                "COLUMN_ID":2,
                "TYPE": "MYINT(21)",
                "NOT_NULL": false,
                "DEFAULT": "222"
            }
        },)"";
    const std::string TABLE_DEFINE_STR_FIELDS_EMPTY = R""("DEFINE": {},)"";
    const std::string TABLE_DEFINE_STR_FIELDS_NOTYPE = R""("DEFINE": {
            "field_name1": {
                "COLUMN_ID":1,
                "NOT_NULL": true,
                "DEFAULT": "abcd"
            }},)"";
    const std::string TABLE_DEFINE_STR_FIELDS_INVALID_CHARACTER = R""("DEFINE": {
            "1 = 1; --": {
                "COLUMN_ID":1,
                "NOT_NULL": true,
                "DEFAULT": "abcd"
            }},)"";
    const std::string TABLE_DEFINE_STR_KEY = R""("PRIMARY_KEY": "field_name1")"";
    const std::string TABLE_DEFINE_BOOL_KEY_INVALID = R""("PRIMARY_KEY": false)"";
    const std::string TABLE_DEFINE_BOOL_ARRAY_KEY_INVALID = R""("PRIMARY_KEY": [false, true, true])"";
}

class DistributedDBRelationalSchemaObjectTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
};

void DistributedDBRelationalSchemaObjectTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    LOGI("The test db is:%s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
}

void DistributedDBRelationalSchemaObjectTest::TearDownTestCase(void)
{
}

void DistributedDBRelationalSchemaObjectTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBRelationalSchemaObjectTest::TearDown()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    return;
}

/**
 * @tc.name: RelationalSchemaParseTest001
 * @tc.desc: Test relational schema parse from json string
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaParseTest001, TestSize.Level1)
{
    const std::string schemaStr = NORMAL_SCHEMA;
    RelationalSchemaObject schemaObj;
    int errCode = schemaObj.ParseFromSchemaString(schemaStr);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(schemaObj.GetTable("FIRST").GetUniqueDefine().size(), 2u);

    RelationalSchemaObject schemaObj2;
    schemaObj2.ParseFromSchemaString(schemaObj.ToSchemaString());
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(schemaObj2.GetTable("FIRST").GetUniqueDefine().size(), 2u);
    errCode = E_OK;
    RelationalSyncOpinion op = SchemaNegotiate::MakeLocalSyncOpinion(schemaObj, schemaObj2.ToSchemaString(),
        static_cast<uint8_t>(SchemaType::RELATIVE), SOFTWARE_VERSION_CURRENT, errCode);

    EXPECT_EQ(op.size(), 2u);
    EXPECT_EQ(op.at("FIRST").permitSync, true);
    EXPECT_EQ(op.at("SECOND").permitSync, true);
}

/**
 * @tc.name: RelationalSchemaParseTest002
 * @tc.desc: Test relational schema parse from invalid json string
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaParseTest002, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;

    std::string schemaStr01(SchemaConstant::SCHEMA_STRING_SIZE_LIMIT + 1, 's');
    int errCode = schemaObj.ParseFromSchemaString(schemaStr01);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);

    errCode = schemaObj.ParseFromSchemaString(INVALID_JSON_STRING);
    EXPECT_EQ(errCode, -E_JSON_PARSE_FAIL);

    std::string noVersion = "{" + SCHEMA_TYPE_STR_RELATIVE + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(noVersion);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidVersion1 = "{" + SCHEMA_VERSION_STR_1  + SCHEMA_TYPE_STR_RELATIVE + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidVersion1);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidVersion2 = "{" + SCHEMA_VERSION_STR_INVALID  + SCHEMA_TYPE_STR_RELATIVE + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidVersion2);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string noType = "{" + SCHEMA_VERSION_STR_2 + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(noType);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidType1 = "{" + SCHEMA_VERSION_STR_2 + SCHEMA_TYPE_STR_NONE + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidType1);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidType2 = "{" + SCHEMA_VERSION_STR_2 + SCHEMA_TYPE_STR_JSON + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidType2);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidType3 = "{" + SCHEMA_VERSION_STR_2 + SCHEMA_TYPE_STR_FLATBUFFER + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidType3);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidType4 = "{" + SCHEMA_VERSION_STR_2 + SCHEMA_TYPE_STR_INVALID + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidType4);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string noTable = "{" + SCHEMA_VERSION_STR_2 +
        SCHEMA_TYPE_STR_RELATIVE.substr(0, SCHEMA_TYPE_STR_RELATIVE.length() - 1) + "}";
    errCode = schemaObj.ParseFromSchemaString(noTable);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);
}

namespace {
std::string GenerateFromTableStr(const std::string &tableStr)
{
    return R""({
        "SCHEMA_VERSION": "2.0",
        "SCHEMA_TYPE": "RELATIVE",
        "TABLES": )"" + tableStr + "}";
}
}

/**
 * @tc.name: RelationalSchemaParseTest003
 * @tc.desc: Test relational schema parse from invalid json string
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaParseTest003, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = E_OK;

    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr(TABLE_DEFINE_STR));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr01 = "{" + TABLE_DEFINE_STR_FIELDS + TABLE_DEFINE_STR_KEY + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr01 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr02 = "{" + TABLE_DEFINE_STR_NAME_INVALID + TABLE_DEFINE_STR_FIELDS +
        TABLE_DEFINE_STR_KEY + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr02 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr04 = "{" + TABLE_DEFINE_STR_NAME + TABLE_DEFINE_STR_FIELDS_NOTYPE +
        TABLE_DEFINE_STR_KEY + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr04 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr05 = "{" + TABLE_DEFINE_STR_NAME + TABLE_DEFINE_STR_FIELDS +
        TABLE_DEFINE_BOOL_KEY_INVALID + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr05 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr06 = "{" + TABLE_DEFINE_STR_NAME + TABLE_DEFINE_STR_FIELDS +
        TABLE_DEFINE_BOOL_ARRAY_KEY_INVALID + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr06 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr07 = "{" + TABLE_DEFINE_STR_NAME_INVALID_CHARACTER + TABLE_DEFINE_STR_FIELDS +
        TABLE_DEFINE_STR_KEY + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr07 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr08 = "{" + TABLE_DEFINE_STR_NAME + TABLE_DEFINE_STR_FIELDS_INVALID_CHARACTER +
        TABLE_DEFINE_STR_KEY + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr08 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    errCode = schemaObj.ParseFromSchemaString("");
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: RelationalSchemaParseTest004
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaParseTest004, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = E_OK;

    std::string schema = "{" + SCHEMA_VERSION_STR_2_1 + SCHEMA_TABLE_MODE_COLLABORATION + SCHEMA_TYPE_STR_RELATIVE +
        SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(schema);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: RelationalSchemaParseTest005
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaParseTest005, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = E_OK;

    std::string schema = "{" + SCHEMA_VERSION_STR_2_1 + SCHEMA_TABLE_MODE_SPLIT_BY_DEVICE + SCHEMA_TYPE_STR_RELATIVE +
        SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(schema);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: RelationalSchemaParseTest006
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaParseTest006, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = E_OK;

    std::string schema = "{" + SCHEMA_VERSION_STR_2_1 + SCHEMA_TABLE_MODE_INVALID + SCHEMA_TYPE_STR_RELATIVE +
        SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(schema);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);
}

/**
 * @tc.name: RelationalSchemaParseTest007
 * @tc.desc: test parse for distributed version in schema
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuhongyang
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaParseTest007, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    std::string schemaOtherField = SCHEMA_VERSION_STR_2_1 + SCHEMA_TABLE_MODE_COLLABORATION + SCHEMA_TYPE_STR_RELATIVE +
        SCHEMA_TABLE_STR + ",";
    /**
     * @tc.steps: step1. call ParseFromSchemaString with a version less than the min of uint32_t
     * @tc.expected: step1. return -E_SCHEMA_PARSE_FAIL.
     */
    std::string schema = "{" + schemaOtherField + DISTRIBUTED_INVALID_SMALL_VERSION + "}";
    int errCode = schemaObj.ParseFromSchemaString(schema);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);
    /**
     * @tc.steps: step2. call ParseFromSchemaString with a version greater than the max of uint32_t
     * @tc.expected: step2. return -E_SCHEMA_PARSE_FAIL.
     */
    schema = "{" + schemaOtherField + DISTRIBUTED_INVALID_LARGE_VERSION + "}";
    errCode = schemaObj.ParseFromSchemaString(schema);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);
    /**
     * @tc.steps: step3. call ParseFromSchemaString with a version that is not a number
     * @tc.expected: step3. return -E_SCHEMA_PARSE_FAIL.
     */
    schema = "{" + schemaOtherField + DISTRIBUTED_INVALID_NAN_VERSION + "}";
    errCode = schemaObj.ParseFromSchemaString(schema);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);
    /**
     * @tc.steps: step4. call ParseFromSchemaString with a normal version
     * @tc.expected: step4. return E_OK.
     */
    schema = "{" + schemaOtherField + DISTRIBUTED_VALID_VERSION + "}";
    errCode = schemaObj.ParseFromSchemaString(schema);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: RelationalSchemaCompareTest001
 * @tc.desc: Test relational schema negotiate with same schema string
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaCompareTest001, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = schemaObj.ParseFromSchemaString(NORMAL_SCHEMA);
    EXPECT_EQ(errCode, E_OK);
    errCode = E_OK;
    RelationalSyncOpinion opinion = SchemaNegotiate::MakeLocalSyncOpinion(schemaObj, NORMAL_SCHEMA,
        static_cast<uint8_t>(SchemaType::RELATIVE), SOFTWARE_VERSION_CURRENT, errCode);
    EXPECT_EQ(opinion.at("FIRST").permitSync, true);
    EXPECT_EQ(opinion.at("FIRST").checkOnReceive, false);
    EXPECT_EQ(opinion.at("FIRST").requirePeerConvert, false);
}

/**
 * @tc.name: RelationalSchemaCompareTest002
 * @tc.desc: Test relational schema v2.1 negotiate with same schema string
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaCompareTest002, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = schemaObj.ParseFromSchemaString(NORMAL_SCHEMA_V2_1);
    EXPECT_EQ(errCode, E_OK);
    errCode = E_OK;
    RelationalSyncOpinion opinion = SchemaNegotiate::MakeLocalSyncOpinion(schemaObj, NORMAL_SCHEMA_V2_1,
        static_cast<uint8_t>(SchemaType::RELATIVE), SOFTWARE_VERSION_CURRENT, errCode);
    EXPECT_EQ(opinion.at("FIRST").permitSync, true);
    EXPECT_EQ(opinion.at("FIRST").checkOnReceive, false);
    EXPECT_EQ(opinion.at("FIRST").requirePeerConvert, false);
}

/**
 * @tc.name: RelationalSchemaCompareTest003
 * @tc.desc: Test relational schema v2.1 negotiate with schema v2.0
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaCompareTest003, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = schemaObj.ParseFromSchemaString(NORMAL_SCHEMA_V2_1);
    EXPECT_EQ(errCode, E_OK);
    errCode = E_OK;
    RelationalSyncOpinion opinion = SchemaNegotiate::MakeLocalSyncOpinion(schemaObj, NORMAL_SCHEMA,
        static_cast<uint8_t>(SchemaType::RELATIVE), SOFTWARE_VERSION_CURRENT, errCode);
    EXPECT_TRUE(opinion.empty());
}

/**
 * @tc.name: RelationalSchemaCompareTest004
 * @tc.desc: Test relational schema v2.0 negotiate with schema v2.1
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaCompareTest004, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = schemaObj.ParseFromSchemaString(NORMAL_SCHEMA);
    EXPECT_EQ(errCode, E_OK);
    errCode = E_OK;
    RelationalSyncOpinion opinion = SchemaNegotiate::MakeLocalSyncOpinion(schemaObj, NORMAL_SCHEMA_V2_1,
        static_cast<uint8_t>(SchemaType::RELATIVE), SOFTWARE_VERSION_CURRENT, errCode);
    EXPECT_TRUE(opinion.empty());
}

/**
 * @tc.name: RelationalSchemaCompareTest005
 * @tc.desc: Test collaboration relational schema negotiate with other table mode
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaCompareTest005, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = schemaObj.ParseFromSchemaString(NORMAL_SCHEMA_V2_1);
    EXPECT_EQ(errCode, E_OK);
    schemaObj.SetTableMode(DistributedTableMode::COLLABORATION);
    errCode = E_OK;
    RelationalSyncOpinion opinion = SchemaNegotiate::MakeLocalSyncOpinion(schemaObj, NORMAL_SCHEMA_V2_1,
        static_cast<uint8_t>(SchemaType::RELATIVE), SOFTWARE_VERSION_CURRENT, errCode);
    EXPECT_TRUE(opinion.empty());
}

/**
 * @tc.name: RelationalTableCompareTest001
 * @tc.desc: Test relational schema negotiate with same schema string
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalTableCompareTest001, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = schemaObj.ParseFromSchemaString(NORMAL_SCHEMA);
    EXPECT_EQ(errCode, E_OK);
    TableInfo table1 = schemaObj.GetTable("FIRST");
    TableInfo table2 = schemaObj.GetTable("FIRST");
    EXPECT_EQ(table1.CompareWithTable(table2), -E_RELATIONAL_TABLE_EQUAL);

    table2.AddIndexDefine("indexname", {"field_name2", "field_name1"});
    EXPECT_EQ(table1.CompareWithTable(table2), -E_RELATIONAL_TABLE_COMPATIBLE);

    TableInfo table3 = schemaObj.GetTable("SECOND");
    EXPECT_EQ(table1.CompareWithTable(table3), -E_RELATIONAL_TABLE_INCOMPATIBLE);

    TableInfo table4 = schemaObj.GetTable("FIRST");
    table4.AddField(table3.GetFields().at("value"));
    EXPECT_EQ(table1.CompareWithTable(table4), -E_RELATIONAL_TABLE_COMPATIBLE_UPGRADE);

    TableInfo table5 = schemaObj.GetTable("FIRST");
    table5.AddField(table3.GetFields().at("key"));
    EXPECT_EQ(table1.CompareWithTable(table5), -E_RELATIONAL_TABLE_INCOMPATIBLE);

    TableInfo table6 = schemaObj.GetTable("FIRST");
    table6.SetUniqueDefine({{"field_name1", "field_name1"}});
    EXPECT_EQ(table1.CompareWithTable(table6, SchemaConstant::SCHEMA_SUPPORT_VERSION_V2_1),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);

    TableInfo table7 = schemaObj.GetTable("FIRST");
    table7.SetAutoIncrement(false);
    EXPECT_EQ(table1.CompareWithTable(table7, SchemaConstant::SCHEMA_SUPPORT_VERSION_V2_1),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
}

/**
 * @tc.name: RelationalSchemaOpinionTest001
 * @tc.desc: Test relational schema sync opinion
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaOpinionTest001, TestSize.Level1)
{
    RelationalSyncOpinion opinion;
    opinion["table_1"] = SyncOpinion {true, false, false};
    opinion["table_2"] = SyncOpinion {false, true, false};
    opinion["table_3"] = SyncOpinion {false, false, true};

    uint32_t len = SchemaNegotiate::CalculateParcelLen(opinion);
    std::vector<uint8_t> buff(len, 0);
    Parcel writeParcel(buff.data(), len);
    int errCode = SchemaNegotiate::SerializeData(opinion, writeParcel);
    EXPECT_EQ(errCode, E_OK);

    Parcel readParcel(buff.data(), len);
    RelationalSyncOpinion opinionRecv;
    errCode = SchemaNegotiate::DeserializeData(readParcel, opinionRecv);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(opinion.size(), opinionRecv.size());
    for (const auto &it : opinion) {
        SyncOpinion tableOpinionRecv = opinionRecv.at(it.first);
        EXPECT_EQ(it.second.permitSync, tableOpinionRecv.permitSync);
        EXPECT_EQ(it.second.requirePeerConvert, tableOpinionRecv.requirePeerConvert);
    }
}

/**
 * @tc.name: RelationalSchemaNegotiateTest001
 * @tc.desc: Test relational schema negotiate
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaNegotiateTest001, TestSize.Level1)
{
    RelationalSyncOpinion localOpinion;
    localOpinion["table_1"] = SyncOpinion {true, false, false};
    localOpinion["table_2"] = SyncOpinion {false, true, false};
    localOpinion["table_3"] = SyncOpinion {false, false, true};

    RelationalSyncOpinion remoteOpinion;
    remoteOpinion["table_2"] = SyncOpinion {true, false, false};
    remoteOpinion["table_3"] = SyncOpinion {false, true, false};
    remoteOpinion["table_4"] = SyncOpinion {false, false, true};
    RelationalSyncStrategy strategy = SchemaNegotiate::ConcludeSyncStrategy(localOpinion, remoteOpinion);

    EXPECT_EQ(strategy.size(), 2u);
    EXPECT_EQ(strategy.at("table_2").permitSync, true);
    EXPECT_EQ(strategy.at("table_3").permitSync, false);
}

/**
 * @tc.name: TableCompareTest001
 * @tc.desc: Test table compare
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, TableCompareTest001, TestSize.Level1)
{
    FieldInfo field1;
    field1.SetFieldName("a");
    FieldInfo field2;
    field2.SetFieldName("b");
    FieldInfo field3;
    field3.SetFieldName("c");
    FieldInfo field4;
    field4.SetFieldName("d");

    TableInfo table;
    table.AddField(field2);
    table.AddField(field3);

    TableInfo inTable1;
    inTable1.AddField(field1);
    inTable1.AddField(field2);
    inTable1.AddField(field3);
    EXPECT_EQ(table.CompareWithTable(inTable1), -E_RELATIONAL_TABLE_COMPATIBLE_UPGRADE);

    TableInfo inTable2;
    inTable2.AddField(field1);
    inTable2.AddField(field2);
    inTable2.AddField(field4);
    EXPECT_EQ(table.CompareWithTable(inTable2), -E_RELATIONAL_TABLE_INCOMPATIBLE);

    TableInfo inTable3;
    inTable3.AddField(field3);
    inTable3.AddField(field2);
    EXPECT_EQ(table.CompareWithTable(inTable3), -E_RELATIONAL_TABLE_EQUAL);
}

HWTEST_F(DistributedDBRelationalSchemaObjectTest, TableCaseInsensitiveCompareTest001, TestSize.Level1)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    std::string createStudentSql = "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT, score INT, level INT)";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, createStudentSql), SQLITE_OK);

    TableInfo tableStudent;
    EXPECT_EQ(SQLiteUtils::AnalysisSchema(db, "STUDENT", tableStudent), E_OK);

    RelationalSchemaObject schema;
    schema.AddRelationalTable(tableStudent);

    EXPECT_FALSE(schema.GetTable("STUDENT").Empty());
    EXPECT_FALSE(schema.GetTable("StudENT").Empty());
    EXPECT_EQ(schema.GetTable("StudENT").CompareWithTable(schema.GetTable("STUDENT")), -E_RELATIONAL_TABLE_EQUAL);

    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

namespace {
TableInfo GetTableInfo(sqlite3 *db, const std::string &tableName, const std::string &sql)
{
    EXPECT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    TableInfo tableInfo;
    EXPECT_EQ(SQLiteUtils::AnalysisSchema(db, tableName, tableInfo), E_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "DROP TABLE IF EXISTS " + tableName), SQLITE_OK);
    return tableInfo;
}
}

HWTEST_F(DistributedDBRelationalSchemaObjectTest, TableCaseInsensitiveCompareTest002, TestSize.Level1)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);

    std::string createTableSql1 = "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT, score INT, level INT); " \
        "create index index_name on student (name);";
    TableInfo table1 = GetTableInfo(db, "student", createTableSql1);
    std::string createTableSql2 = "CREATE TABLE Student(ID INTEGER PRIMARY KEY, Name TEXT, Score INT, Level INT); " \
        "create index index_NAME on student (Name);";
    TableInfo table2 = GetTableInfo(db, "Student", createTableSql2);

    EXPECT_EQ(table1.CompareWithTable(table2), -E_RELATIONAL_TABLE_EQUAL);

    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
    db = nullptr;
}

namespace {
int TableCompareTest(sqlite3 *db, const std::string &sql1, const std::string &sql2)
{
    RelationalTestUtils::ExecSql(db, sql1);
    TableInfo table1;
    SQLiteUtils::AnalysisSchema(db, "student", table1);
    RelationalTestUtils::ExecSql(db, "DROP TABLE IF EXISTS student");
    RelationalTestUtils::ExecSql(db, sql2);
    TableInfo table2;
    SQLiteUtils::AnalysisSchema(db, "student", table2);
    RelationalTestUtils::ExecSql(db, "DROP TABLE IF EXISTS student");
    return table1.CompareWithTable(table2);
}

/**
 * @tc.name: TableCompareTest001
 * @tc.desc: Test table compare with default value
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, TableCompareTest002, TestSize.Level1)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT)",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT)"),
        -E_RELATIONAL_TABLE_EQUAL);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT)",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT 'xue')"),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT)",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT '')"),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT)",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT 'NULL')"),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT)",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT 'null')"),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT)",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT NULL)"),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT)",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT null)"),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT 'XUE')",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT 'xue')"),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT NULL)",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT null)"),
        -E_RELATIONAL_TABLE_EQUAL);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT 'NULL')",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT 'null')"),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT '')",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT NULL)"),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT '')",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT null)"),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);
    EXPECT_EQ(TableCompareTest(db, "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT '')",
        "CREATE TABLE student(id INTEGER PRIMARY KEY, name TEXT DEFAULT '')"),
        -E_RELATIONAL_TABLE_EQUAL);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    db = nullptr;
}
} // namespace

/**
 * @tc.name: FieldInfoCompareTest
 * @tc.desc: Test field info compare
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, FieldInfoCompareTest, TestSize.Level1)
{
    /**
     * @tc.steps: step1. call CompareWithField when storageType is different
     * @tc.expected: step1. return false.
     */
    FieldInfo field1;
    field1.SetStorageType(StorageType::STORAGE_TYPE_INTEGER);
    FieldInfo field2;
    EXPECT_EQ(field2.CompareWithField(field1, true), false);

    /**
     * @tc.steps: step2. call CompareWithField when fieldName is different
     * @tc.expected: step2. return false.
     */
    field1.SetFieldName("test1");
    field1.SetFieldName("test2");
    EXPECT_EQ(field2.CompareWithField(field1, true), false);
}

/**
 * @tc.name: TableInfoInterfacesTest
 * @tc.desc: Test TableInfo Interfaces
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, TableInfoInterfacesTest, TestSize.Level1)
{
    /**
     * @tc.steps: step1. GetFieldName with empty TableInfo class
     * @tc.expected: step1. return empty string.
     */
    TableInfo table1;
    std::string str1 = table1.GetFieldName(0);
    const std::string expectStr1 = "";
    EXPECT_TRUE(str1.compare(0, expectStr1.length(), expectStr1) == 0);
    table1.ToTableInfoString("");

    /**
     * @tc.steps: step2. Set and get tableId.
     * @tc.expected: step2. success.
     */
    int inputId = 1;
    table1.SetTableId(inputId);
    int outputId = table1.GetTableId();
    EXPECT_EQ(outputId, inputId);
}

/**
 * @tc.name: SchemaTableCompareTest
 * @tc.desc: Test LiteSchemaTable Compare
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, SchemaTableCompareTest, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set key index of SetPrimaryKey to an invalid value
     * @tc.expected: step1. fieldName vector is null.
     */
    TableInfo table1;
    int keyIndex = -1;
    table1.SetPrimaryKey("test", keyIndex);
    CompositeFields vec = table1.GetIdentifyKey();
    uint32_t expectedVal = 0;
    EXPECT_EQ(vec.size(), expectedVal);

    /**
     * @tc.steps: step2. Compare table when fieldName of SetPrimaryKey is set to'rowid'
     * @tc.expected: step2. compare return -E_RELATIONAL_TABLE_INCOMPATIBLE.
     */
    const std::vector<CompositeFields> uniqueDefine = {{"test0", "test1"}};
    table1.SetUniqueDefine(uniqueDefine);
    const std::map<int, FieldName> keyName1 = {{0, "rowid"}};
    table1.SetPrimaryKey(keyName1);

    vec = table1.GetIdentifyKey();
    EXPECT_EQ(vec.size(), uniqueDefine[0].size());
    int ret = table1.CompareWithLiteSchemaTable(table1);
    EXPECT_EQ(ret, -E_RELATIONAL_TABLE_INCOMPATIBLE);

    /**
     * @tc.steps: step3. Compare table when fieldName of SetPrimaryKey is not set to "rowid".
     * @tc.expected: step3. compare return E_OK.
     */
    FieldInfo field1;
    table1.AddField(field1);
    const std::map<int, FieldName> keyName2 = {{0, "test0"}, {1, "test1"}};
    table1.SetPrimaryKey(keyName2);

    vec = table1.GetIdentifyKey();
    EXPECT_EQ(vec.size(), keyName2.size());
    field1.SetFieldName("test1");
    ret = table1.CompareWithLiteSchemaTable(table1);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: EqualConstraintSerializeAndParseTest001
 * @tc.desc: Test EqualConstraint serialization and deserialization in tableSyncPolicies
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xfz
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, EqualConstraintSerializeAndParseTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set distributed schema with tableSyncPolicies and verify serialization.
     * @tc.expected: step1. Schema string contains FIELD_SYNC_POLICIES with EQUAL_CONSTRAINTS.
     */
    DistributedSchema schema{.version = 1};
    DistributedTable table{.tableName = "STUDENT"};
    DistributedField field{.colName = "name", .isP2pSync = false, .isSpecified = false};
    table.fields.push_back(field);
    schema.tables.push_back(table);

    TableSyncPolicy policyTable{.tableName = "STUDENT"};
    FieldSyncPolicy policyField{.colName = "name"};
    EqualConstraint ec1{.notNull = true, .hasDefault = true};
    EqualConstraint ec2{.notNull = false, .hasDefault = true};
    policyField.equalConstraints = {ec1, ec2};
    policyTable.fieldSyncPolicies.push_back(policyField);
    schema.tableSyncPolicies.push_back(policyTable);

    RelationalSchemaObject schemaObj;
    schemaObj.SetDistributedSchema(schema);
    std::string schemaStr = schemaObj.ToSchemaString();
    EXPECT_NE(schemaStr.find("FIELD_SYNC_POLICIES"), std::string::npos);
    EXPECT_NE(schemaStr.find("EQUAL_CONSTRAINTS"), std::string::npos);
    EXPECT_NE(schemaStr.find("NOT_NULL"), std::string::npos);
    EXPECT_NE(schemaStr.find("HAS_DEFAULT"), std::string::npos);

    /**
     * @tc.steps: step2. Parse schema string and verify tableSyncPolicies is preserved.
     * @tc.expected: step2. tableSyncPolicies is correctly parsed after round-trip.
     */
    RelationalSchemaObject parsedObj;
    int errCode = parsedObj.ParseFromSchemaString(schemaStr);
    EXPECT_EQ(errCode, E_OK);
    const auto &parsedSchema = parsedObj.GetDistributedSchema();
    EXPECT_EQ(parsedSchema.tableSyncPolicies.size(), 1u);
    EXPECT_EQ(parsedSchema.tableSyncPolicies[0].tableName, "STUDENT");
    EXPECT_EQ(parsedSchema.tableSyncPolicies[0].fieldSyncPolicies.size(), 1u);
    EXPECT_EQ(parsedSchema.tableSyncPolicies[0].fieldSyncPolicies[0].equalConstraints.size(), 2u);
    bool foundTrue = false;
    bool foundFalse = false;
    bool allHasDefault = true;
    for (const auto &ec : parsedSchema.tableSyncPolicies[0].fieldSyncPolicies[0].equalConstraints) {
        if (ec.notNull) foundTrue = true;
        if (!ec.notNull) foundFalse = true;
        if (!ec.hasDefault) allHasDefault = false;
    }
    EXPECT_TRUE(foundTrue);
    EXPECT_TRUE(foundFalse);
    EXPECT_TRUE(allHasDefault);
}

/**
 * @tc.name: EqualConstraintCompareWithTableTest001
 * @tc.desc: Test CompareWithTable with isNotNull mismatch under different equalConstraints
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xfz
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, EqualConstraintCompareWithTableTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create two tables with same fields but different isNotNull.
     * @tc.expected: step1. Without equalConstraints, table is incompatible;
     *               with equalConstraints, table is compatible.
     */
    FieldInfo field1;
    field1.SetFieldName("id");
    field1.SetNotNull(true);
    field1.SetDefaultValue("0");

    FieldInfo field2;
    field2.SetFieldName("id");
    field2.SetNotNull(false);
    field2.SetDefaultValue("0");

    TableInfo table1;
    table1.SetTableName("test_table");
    table1.AddField(field1);

    TableInfo table2;
    table2.SetTableName("test_table");
    table2.AddField(field2);

    // Without equalConstraints: isNotNull mismatch causes incompatibility
    EXPECT_EQ(table1.CompareWithTable(table2, SchemaConstant::SCHEMA_SUPPORT_VERSION_V2),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);

    // With equalConstraints: isNotNull mismatch is allowed
    std::map<std::string, std::vector<EqualConstraint>> equalConstraints;
    EqualConstraint ec1{.notNull = true, .hasDefault = true};
    EqualConstraint ec2{.notNull = false, .hasDefault = true};
    equalConstraints["id"] = {ec1, ec2};
    int result = table1.CompareWithTable(table2, SchemaConstant::SCHEMA_SUPPORT_VERSION_V2, equalConstraints);
    EXPECT_NE(result, -E_RELATIONAL_TABLE_INCOMPATIBLE);
}

/**
 * @tc.name: EqualConstraintCompareWithTableTest002
 * @tc.desc: Test CompareWithTable with upgrade field (IsNotNull && !HasDefaultValue)
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xfz
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, EqualConstraintCompareWithTableTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create local table with fewer fields, remote table has extra not-null field without default.
     * @tc.expected: step1. Table is incompatible regardless of equalConstraints.
     */
    FieldInfo field1;
    field1.SetFieldName("id");
    field1.SetNotNull(false);

    FieldInfo field2;
    field2.SetFieldName("name");
    field2.SetNotNull(true); // not null but no default value

    TableInfo table1;
    table1.SetTableName("test_table");
    table1.AddField(field1);

    TableInfo table2;
    table2.SetTableName("test_table");
    table2.AddField(field1);
    table2.AddField(field2);

    // Upgrade field with IsNotNull && !HasDefaultValue causes incompatibility
    EXPECT_EQ(table1.CompareWithTable(table2, SchemaConstant::SCHEMA_SUPPORT_VERSION_V2),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);

    std::map<std::string, std::vector<EqualConstraint>> equalConstraints;
    EqualConstraint ec1;
    ec1.notNull = true;
    ec1.hasDefault = true;
    EqualConstraint ec2;
    ec2.notNull = false;
    ec2.hasDefault = true;
    equalConstraints["name"] = {ec1, ec2};
    EXPECT_EQ(table1.CompareWithTable(table2, SchemaConstant::SCHEMA_SUPPORT_VERSION_V2, equalConstraints),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);

    /**
     * @tc.steps: step2. Remote table has extra not-null field with default value.
     * @tc.expected: step2. With equalConstraints, table is compatible upgrade.
     */
    FieldInfo field2WithDefault;
    field2WithDefault.SetFieldName("name");
    field2WithDefault.SetNotNull(true);
    field2WithDefault.SetDefaultValue("'unknown'");

    TableInfo table2WithDefault;
    table2WithDefault.SetTableName("test_table");
    table2WithDefault.AddField(field1);
    table2WithDefault.AddField(field2WithDefault);

    int result = table1.CompareWithTable(table2WithDefault,
        SchemaConstant::SCHEMA_SUPPORT_VERSION_V2, equalConstraints);
    EXPECT_EQ(result, -E_RELATIONAL_TABLE_COMPATIBLE_UPGRADE);
}

/**
 * @tc.name: EqualConstraintCheckDistributedSchemaChangeTest001
 * @tc.desc: Test CheckDistributedSchemaChange detects tableSyncPolicies change
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xfz
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, EqualConstraintCheckDistributedSchemaChangeTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set distributed schema without tableSyncPolicies, then check with tableSyncPolicies.
     * @tc.expected: step1. CheckDistributedSchemaChange returns true (changed).
     */
    DistributedSchema schema1{.version = 1};
    DistributedTable table{.tableName = "test_table"};
    DistributedField field{.colName = "name", .isP2pSync = false, .isSpecified = false};
    table.fields.push_back(field);
    schema1.tables.push_back(table);

    RelationalSchemaObject schemaObj;
    schemaObj.SetDistributedSchema(schema1);

    DistributedSchema schema2{.version = 1};
    DistributedTable table2{.tableName = "test_table"};
    DistributedField field2{.colName = "name", .isP2pSync = false, .isSpecified = false};
    table2.fields.push_back(field2);
    schema2.tables.push_back(table2);

    TableSyncPolicy policyTable{.tableName = "test_table"};
    FieldSyncPolicy policyField{.colName = "name"};
    EqualConstraint ec1{.notNull = true, .hasDefault = true};
    EqualConstraint ec2{.notNull = false, .hasDefault = true};
    policyField.equalConstraints = {ec1, ec2};
    policyTable.fieldSyncPolicies.push_back(policyField);
    schema2.tableSyncPolicies.push_back(policyTable);

    EXPECT_TRUE(schemaObj.CheckDistributedSchemaChange(schema2));

    /**
     * @tc.steps: step2. Check with same tableSyncPolicies.
     * @tc.expected: step2. CheckDistributedSchemaChange returns false (not changed).
     */
    schemaObj.SetDistributedSchema(schema2);
    EXPECT_FALSE(schemaObj.CheckDistributedSchemaChange(schema2));
}

/**
 * @tc.name: EqualConstraintCompareWithFieldTest001
 * @tc.desc: Test CompareWithField with isNotNull mismatch and equalConstraints
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xfz
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, EqualConstraintCompareWithFieldTest001, TestSize.Level1)
{
    FieldInfo field1;
    field1.SetFieldName("name");
    field1.SetNotNull(true);
    field1.SetDefaultValue("0");

    FieldInfo field2;
    field2.SetFieldName("name");
    field2.SetNotNull(false);
    field2.SetDefaultValue("0");

    /**
     * @tc.steps: step1. Compare fields with isNotNull mismatch without equal constraint.
     * @tc.expected: step1. Compare returns false (incompatible).
     */
    EXPECT_FALSE(field1.CompareWithField(field2, false));

    /**
     * @tc.steps: step2. Compare fields with isNotNull mismatch with equal constraint.
     * @tc.expected: step2. Compare returns true (equal constraint allows mismatch).
     */
    std::map<std::string, std::vector<EqualConstraint>> equalConstraints;
    EqualConstraint ec1{.notNull = true, .hasDefault = true};
    EqualConstraint ec2{.notNull = false, .hasDefault = true};
    equalConstraints["name"] = {ec1, ec2};

    EXPECT_TRUE(field1.CompareWithField(field2, false, equalConstraints));

    /**
     * @tc.steps: step3. Compare fields with equal constraint but different column name.
     * @tc.expected: step3. Compare returns false (constraint does not match column).
     */
    std::map<std::string, std::vector<EqualConstraint>> wrongColConstraints;
    EqualConstraint wec1{.notNull = true, .hasDefault = true};
    EqualConstraint wec2{.notNull = false, .hasDefault = true};
    wrongColConstraints["other_col"] = {wec1, wec2};
    EXPECT_FALSE(field1.CompareWithField(field2, false, wrongColConstraints));

    /**
     * @tc.steps: step4. Compare fields with same isNotNull under no constraint.
     * @tc.expected: step4. Compare returns true (compatible).
     */
    FieldInfo field3;
    field3.SetFieldName("name");
    field3.SetNotNull(true);
    field3.SetDefaultValue("0");
    EXPECT_TRUE(field1.CompareWithField(field3, false));
}

/**
 * @tc.name: EqualConstraintCompareWithFieldTest002
 * @tc.desc: Test CompareWithField with equal constraint but notNull side has no default value
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xfz
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, EqualConstraintCompareWithFieldTest002, TestSize.Level1)
{
    FieldInfo field1;
    field1.SetFieldName("name");
    field1.SetNotNull(true);
    // no default value

    FieldInfo field2;
    field2.SetFieldName("name");
    field2.SetNotNull(false);

    std::map<std::string, std::vector<EqualConstraint>> equalConstraints;
    EqualConstraint ec1{.notNull = true, .hasDefault = true};
    EqualConstraint ec2{.notNull = false, .hasDefault = true};
    equalConstraints["name"] = {ec1, ec2};

    /**
     * @tc.steps: step1. Compare fields with equal constraint but notNull side has no default value.
     * @tc.expected: step1. Compare returns false (notNull side must have default value).
     */
    EXPECT_FALSE(field1.CompareWithField(field2, false, equalConstraints));

    /**
     * @tc.steps: step2. Add default value to notNull side, compare again.
     * @tc.expected: step2. Compare returns true.
     */
    field1.SetDefaultValue("'unknown'");
    field2.SetDefaultValue("'unknown'");
    EXPECT_TRUE(field1.CompareWithField(field2, false, equalConstraints));
}

/**
 * @tc.name: EqualConstraintCompareWithFieldTest003
 * @tc.desc: Test CompareWithField with equal constraint but hasDefault is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xfz
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, EqualConstraintCompareWithFieldTest003, TestSize.Level1)
{
    FieldInfo field1;
    field1.SetFieldName("name");
    field1.SetNotNull(true);
    field1.SetDefaultValue("0");

    FieldInfo field2;
    field2.SetFieldName("name");
    field2.SetNotNull(false);
    field2.SetDefaultValue("0");

    /**
     * @tc.steps: step1. Compare fields with equal constraint but hasDefault is false.
     * @tc.expected: step1. Compare returns false (hasDefault must be true for all constraints).
     */
    std::map<std::string, std::vector<EqualConstraint>> equalConstraintsNoDefault;
    EqualConstraint ec1{.notNull = true, .hasDefault = false};
    EqualConstraint ec2{.notNull = false, .hasDefault = false};
    equalConstraintsNoDefault["name"] = {ec1, ec2};
    EXPECT_FALSE(field1.CompareWithField(field2, false, equalConstraintsNoDefault));

    /**
     * @tc.steps: step2. Compare fields with equal constraint but only one hasDefault is true.
     * @tc.expected: step2. Compare returns false (all hasDefault must be true).
     */
    std::map<std::string, std::vector<EqualConstraint>> equalConstraintsPartial;
    EqualConstraint ec3{.notNull = true, .hasDefault = true};
    EqualConstraint ec4{.notNull = false, .hasDefault = false};
    equalConstraintsPartial["name"] = {ec3, ec4};
    EXPECT_FALSE(field1.CompareWithField(field2, false, equalConstraintsPartial));

    /**
     * @tc.steps: step3. Compare fields with equal constraint and all hasDefault is true.
     * @tc.expected: step3. Compare returns true.
     */
    std::map<std::string, std::vector<EqualConstraint>> equalConstraintsAllDefault;
    EqualConstraint ec5{.notNull = true, .hasDefault = true};
    EqualConstraint ec6{.notNull = false, .hasDefault = true};
    equalConstraintsAllDefault["name"] = {ec5, ec6};
    EXPECT_TRUE(field1.CompareWithField(field2, false, equalConstraintsAllDefault));
}

/**
 * @tc.name: EqualConstraintCompareWithTableTest003
 * @tc.desc: Test CompareWithTable with EqualConstraint under STRONG-equivalent behavior
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xfz
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, EqualConstraintCompareWithTableTest003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create two tables with same fields but different isNotNull, with equal constraint.
     * @tc.expected: step1. With equal constraint, table is compatible.
     */
    FieldInfo field1;
    field1.SetFieldName("id");
    field1.SetNotNull(true);
    field1.SetDefaultValue("0");

    FieldInfo field2;
    field2.SetFieldName("id");
    field2.SetNotNull(false);
    field2.SetDefaultValue("0");

    TableInfo table1;
    table1.SetTableName("STUDENT");
    table1.AddField(field1);

    TableInfo table2;
    table2.SetTableName("STUDENT");
    table2.AddField(field2);

    // Without equal constraint: incompatible
    EXPECT_EQ(table1.CompareWithTable(table2, SchemaConstant::SCHEMA_SUPPORT_VERSION_V2),
        -E_RELATIONAL_TABLE_INCOMPATIBLE);

    // With equal constraint: compatible
    std::map<std::string, std::vector<EqualConstraint>> equalConstraints;
    EqualConstraint ec1{.notNull = true, .hasDefault = true};
    EqualConstraint ec2{.notNull = false, .hasDefault = true};
    equalConstraints["id"] = {ec1, ec2};
    int result = table1.CompareWithTable(table2, SchemaConstant::SCHEMA_SUPPORT_VERSION_V2, equalConstraints);
    EXPECT_NE(result, -E_RELATIONAL_TABLE_INCOMPATIBLE);
}

std::string GetLocalSchemaStr1()
{
    return R""({
        "SCHEMA_VERSION": "2.1",
        "SCHEMA_TYPE": "RELATIVE",
        "TABLE_MODE": "SPLIT_BY_DEVICE",
        "TABLES": [{
            "NAME": "STUDENT",
            "DEFINE": {
                "id": {"COLUMN_ID":1, "TYPE": "INTEGER", "NOT_NULL": true},
                "name": {"COLUMN_ID":2, "TYPE": "TEXT", "NOT_NULL": true, "DEFAULT": "'unknown'"}
            },
            "PRIMARY_KEY": "id"
        }],
        "DISTRIBUTED_SCHEMA": {
            "VERSION": 1,
            "DISTRIBUTED_TABLE": [{
                "TABLE_NAME": "STUDENT",
                "DISTRIBUTED_FIELD": [{"COL_NAME": "name", "IS_P2P_SYNC": false, "IS_SPECIFIED": false}]
            }],
            "FIELD_SYNC_POLICIES": [{
                "TABLE_NAME": "STUDENT",
                "DISTRIBUTED_FIELD": [{
                    "COL_NAME": "name",
                    "EQUAL_CONSTRAINTS": [
                        {"NOT_NULL": true, "HAS_DEFAULT": true}, {"NOT_NULL": false, "HAS_DEFAULT": true}
                    ]
                }]
            }]
        }
    })"";
}

std::string GetRemoteSchemaStr1()
{
    return R""({
        "SCHEMA_VERSION": "2.1",
        "SCHEMA_TYPE": "RELATIVE",
        "TABLE_MODE": "SPLIT_BY_DEVICE",
        "TABLES": [{
            "NAME": "STUDENT",
            "DEFINE": {
                "id": {"COLUMN_ID":1, "TYPE": "INTEGER", "NOT_NULL": true},
                "name": {"COLUMN_ID":2, "TYPE": "TEXT", "NOT_NULL": false, "DEFAULT": "'unknown'"}
            },
            "PRIMARY_KEY": "id"
        }],
        "DISTRIBUTED_SCHEMA": {
            "VERSION": 1,
            "DISTRIBUTED_TABLE": [{
                "TABLE_NAME": "STUDENT",
                "DISTRIBUTED_FIELD": [{"COL_NAME": "name", "IS_P2P_SYNC": false, "IS_SPECIFIED": false}]
            }],
            "FIELD_SYNC_POLICIES": [{
                "TABLE_NAME": "STUDENT",
                "DISTRIBUTED_FIELD": [{
                    "COL_NAME": "name",
                    "EQUAL_CONSTRAINTS": [
                        {"NOT_NULL": true, "HAS_DEFAULT": true}, {"NOT_NULL": false, "HAS_DEFAULT": true}
                    ]
                }]
            }]
        }
    })"";
}

/**
 * @tc.name: EqualConstraintNegotiateTest001
 * @tc.desc: Test schema negotiate with EqualConstraint in fieldSyncPolicies
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xfz
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, EqualConstraintNegotiateTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create local schema with equal constraint in fieldSyncPolicies.
     * Create remote schema with same table but name field is nullable.
     * @tc.expected: step1. With equal constraint, negotiate should permit sync.
     */
    const std::string localSchemaStr = GetLocalSchemaStr1();
    const std::string remoteSchemaStr = GetRemoteSchemaStr1();

    RelationalSchemaObject localSchema;
    int errCode = localSchema.ParseFromSchemaString(localSchemaStr);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(localSchema.GetDistributedSchema().tableSyncPolicies.size(), 1u);
    EXPECT_EQ(localSchema.GetDistributedSchema().tableSyncPolicies[0].tableName, "STUDENT");
    EXPECT_EQ(localSchema.GetDistributedSchema().tableSyncPolicies[0].fieldSyncPolicies.size(), 1u);
    EXPECT_EQ(localSchema.GetDistributedSchema().tableSyncPolicies[0].fieldSyncPolicies[0].equalConstraints.size(), 2u);

    int negotiateErrCode = E_OK;
    RelationalSyncOpinion opinion = SchemaNegotiate::MakeLocalSyncOpinion(localSchema, remoteSchemaStr,
        static_cast<uint8_t>(SchemaType::RELATIVE), SOFTWARE_VERSION_RELEASE_11_0, negotiateErrCode);
    // With equal constraint, isNotNull mismatch should not block sync
    EXPECT_EQ(negotiateErrCode, E_OK);
    EXPECT_FALSE(opinion.empty());
    if (opinion.find("STUDENT") != opinion.end()) {
        EXPECT_TRUE(opinion.at("STUDENT").permitSync);
    }
}

std::string GetLocalSchemaStr2()
{
    return R""({
        "SCHEMA_VERSION": "2.1",
        "SCHEMA_TYPE": "RELATIVE",
        "TABLE_MODE": "SPLIT_BY_DEVICE",
        "TABLES": [{
            "NAME": "STUDENT",
            "DEFINE": {
                "id": {"COLUMN_ID":1, "TYPE": "INTEGER", "NOT_NULL": true},
                "name": {"COLUMN_ID":2, "TYPE": "TEXT", "NOT_NULL": true}
            },
            "PRIMARY_KEY": "id"
        }],
        "DISTRIBUTED_SCHEMA": {
            "VERSION": 1,
            "DISTRIBUTED_TABLE": [{
                "TABLE_NAME": "STUDENT",
                "DISTRIBUTED_FIELD": [{"COL_NAME": "name", "IS_P2P_SYNC": true, "IS_SPECIFIED": false}]
            }],
            "FIELD_SYNC_POLICIES": []
        }
    })"";
}

std::string GetRemoteSchemaStr2()
{
    return R""({
        "SCHEMA_VERSION": "2.1",
        "SCHEMA_TYPE": "RELATIVE",
        "TABLE_MODE": "SPLIT_BY_DEVICE",
        "TABLES": [{
            "NAME": "STUDENT",
            "DEFINE": {
                "id": {"COLUMN_ID":1, "TYPE": "INTEGER", "NOT_NULL": true},
                "name": {"COLUMN_ID":2, "TYPE": "TEXT", "NOT_NULL": false}
            },
            "PRIMARY_KEY": "id"
        }],
        "DISTRIBUTED_SCHEMA": {
            "VERSION": 1,
            "DISTRIBUTED_TABLE": [{
                "TABLE_NAME": "STUDENT",
                "DISTRIBUTED_FIELD": [{"COL_NAME": "name", "IS_P2P_SYNC": true, "IS_SPECIFIED": false}]
            }],
            "FIELD_SYNC_POLICIES": []
        }
    })"";
}

/**
 * @tc.name: EqualConstraintNegotiateTest002
 * @tc.desc: Test schema negotiate with isNotNull mismatch but no equal constraint
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xfz
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, EqualConstraintNegotiateTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create local schema without equal constraint and a table with NOT_NULL field.
     * Create remote schema with same table but field is nullable.
     * @tc.expected: step1. Without equal constraint, negotiate should not permit sync.
     */
    const std::string localSchemaStr = GetLocalSchemaStr2();
    const std::string remoteSchemaStr = GetRemoteSchemaStr2();

    RelationalSchemaObject localSchema;
    int errCode = localSchema.ParseFromSchemaString(localSchemaStr);
    EXPECT_EQ(errCode, E_OK);

    int negotiateErrCode = E_OK;
    RelationalSyncOpinion opinion = SchemaNegotiate::MakeLocalSyncOpinion(localSchema, remoteSchemaStr,
        static_cast<uint8_t>(SchemaType::RELATIVE), SOFTWARE_VERSION_RELEASE_11_0, negotiateErrCode);
    // Without equal constraint, isNotNull mismatch should block sync
    EXPECT_EQ(negotiateErrCode, -E_TABLE_FIELD_MISMATCH);
}
#endif
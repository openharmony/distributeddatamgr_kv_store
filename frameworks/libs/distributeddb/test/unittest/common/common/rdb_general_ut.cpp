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

void RDBGeneralUt::AddSchemaInfo(const StoreInfo &info, const DistributedDBUnitTest:: UtDateBaseSchemaInfo& schemaInfo)
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

TableSchema RDBGeneralUt::GetTableSchema(const StoreInfo &info, const std::string tableName)
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
}
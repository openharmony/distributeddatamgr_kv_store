/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "rdb_data_generator.h"

#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "res_finalizer.h"
namespace DistributedDBUnitTest {
using namespace DistributedDB;
int RDBDataGenerator::InitDatabase(const DataBaseSchema &schema, sqlite3 &db)
{
    int errCode = RelationalTestUtils::ExecSql(&db, "PRAGMA journal_mode=WAL;");
    if (errCode != SQLITE_OK) {
        return errCode;
    }
    for (const auto &table : schema.tables) {
        std::string sql = "CREATE TABLE IF NOT EXISTS " + table.name + "(";
        for (const auto &field : table.fields) {
            sql += field.colName + " " + GetTypeText(field.type);
            if (field.primary) {
                sql += " PRIMARY KEY,";
            } else {
                sql += ",";
            }
        }
        sql.pop_back();
        sql += ");";
        errCode = RelationalTestUtils::ExecSql(&db, sql);
        if (errCode != SQLITE_OK) {
            LOGE("execute sql failed %d, sql is %s", errCode, sql.c_str());
            break;
        }
    }
    return errCode;
}

std::string RDBDataGenerator::GetTypeText(int type)
{
    switch (type) {
        case DistributedDB::TYPE_INDEX<int64_t>:
            return "INT";
        case DistributedDB::TYPE_INDEX<std::string>:
            return "TEXT";
        case DistributedDB::TYPE_INDEX<DistributedDB::Assets>:
            return "ASSETS";
        case DistributedDB::TYPE_INDEX<DistributedDB::Asset>:
            return "ASSET";
        default:
            return "";
    }
}

DistributedDB::DBStatus RDBDataGenerator::InsertCloudDBData(int64_t begin, int64_t count, int64_t gidStart,
    const DistributedDB::DataBaseSchema &schema,
    const std::shared_ptr<DistributedDB::VirtualCloudDb> &virtualCloudDb)
{
    for (const auto &table : schema.tables) {
        auto [record, extend] = GenerateDataRecords(begin, count, gidStart, table.fields);
        DBStatus res = virtualCloudDb->BatchInsertWithGid(table.name, std::move(record), extend);
        if (res != DBStatus::OK) {
            return res;
        }
    }
    return DBStatus::OK;
}

std::pair<std::vector<VBucket>, std::vector<VBucket>> RDBDataGenerator::GenerateDataRecords(int64_t begin,
    int64_t count, int64_t gidStart, const std::vector<Field> &fields)
{
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    Timestamp now = TimeHelper::GetSysCurrentTime();
    auto time = static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND);
    for (int64_t i = begin; i < begin + count; i++) {
        VBucket data;
        for (const auto &field : fields) {
            FillColValueByType(i, field, data);
        }
        record.push_back(data);

        VBucket log;
        log.insert_or_assign(CloudDbConstant::CREATE_FIELD, time);
        log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, time);
        log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
        log.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(i + gidStart));
        extend.push_back(log);
        time++;
    }
    return {record, extend};
}

void RDBDataGenerator::FillColValueByType(int64_t index, const Field &field, VBucket &vBucket)
{
    vBucket[field.colName] = GetColValueByType(index, field);
}

DistributedDB::Type RDBDataGenerator::GetColValueByType(int64_t index, const DistributedDB::Field &field)
{
    Type value = Nil();
    switch (field.type) {
        case DistributedDB::TYPE_INDEX<int64_t>:
            value = index;
            break;
        case DistributedDB::TYPE_INDEX<std::string>:
            value = field.colName + "_" + std::to_string(index);
            break;
        case DistributedDB::TYPE_INDEX<DistributedDB::Assets>:
            value = GenerateAssets(index, field);
            break;
        case DistributedDB::TYPE_INDEX<DistributedDB::Asset>:
            value = GenerateAsset(index, field);
            break;
    }
    return value;
}

Asset RDBDataGenerator::GenerateAsset(int64_t index, const DistributedDB::Field &field)
{
    Asset asset;
    asset.name = field.colName + "_" + std::to_string(index);
    asset.hash = "default_hash";
    return asset;
}

Assets RDBDataGenerator::GenerateAssets(int64_t index, const Field &field)
{
    Assets assets;
    assets.push_back(GenerateAsset(index, field));
    return assets;
}

int RDBDataGenerator::InsertLocalDBData(int64_t begin, int64_t count, sqlite3 *db,
    const DistributedDB::DataBaseSchema &schema)
{
    for (const auto &table : schema.tables) {
        int errCode = InsertLocalDBData(begin, count, db, table);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

int RDBDataGenerator::InsertLocalDBData(int64_t begin, int64_t count, sqlite3 *db,
    const DistributedDB::TableSchema &schema)
{
    if (schema.fields.empty()) {
        return -E_INTERNAL_ERROR;
    }
    std::string sql = "INSERT OR REPLACE INTO " + schema.name + " VALUES(";
    for (size_t i = 0; i < schema.fields.size(); ++i) {
        sql += "?,";
    }
    sql.pop_back();
    sql += ");";
    for (int64_t i = begin; i < begin + count; i++) {
        sqlite3_stmt *stmt = nullptr;
        int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
        if (errCode != E_OK) {
            return errCode;
        }
        ResFinalizer resFinalizer([stmt]() {
            sqlite3_stmt *sqlite3Stmt = stmt;
            int ret = E_OK;
            SQLiteUtils::ResetStatement(sqlite3Stmt, true, ret);
        });
        errCode = BindOneRowStmt(i, stmt, 0, false, schema.fields);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = SQLiteUtils::StepNext(stmt);
        if (errCode != -E_FINISHED) {
            return errCode;
        }
    }
    return E_OK;
}

int RDBDataGenerator::UpsertLocalDBData(int64_t begin, int64_t count, sqlite3 *db,
    const DistributedDB::TableSchema &schema)
{
    if (schema.fields.empty()) {
        return -E_INTERNAL_ERROR;
    }
    std::string sql = GetUpsertSQL(schema);
    for (int64_t i = begin; i < begin + count; i++) {
        sqlite3_stmt *stmt = nullptr;
        int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
        if (errCode != E_OK) {
            return errCode;
        }
        ResFinalizer resFinalizer([stmt]() {
            sqlite3_stmt *sqlite3Stmt = stmt;
            int ret = E_OK;
            SQLiteUtils::ResetStatement(sqlite3Stmt, true, ret);
        });
        errCode = BindOneRowStmt(i, stmt, 0, false, schema.fields);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = BindOneRowStmt(i, stmt, static_cast<int>(schema.fields.size()), true, schema.fields);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = SQLiteUtils::StepNext(stmt);
        if (errCode != -E_FINISHED) {
            return errCode;
        }
    }
    return E_OK;
}

int RDBDataGenerator::UpdateLocalDBData(int64_t begin, int64_t count, sqlite3 *db, const TableSchema &schema)
{
    if (schema.fields.empty()) {
        return -E_INTERNAL_ERROR;
    }
    std::string sql = GetUpdateSQL(schema);
    for (int64_t i = begin; i < begin + count; i++) {
        sqlite3_stmt *stmt = nullptr;
        int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
        if (errCode != E_OK) {
            return errCode;
        }
        ResFinalizer resFinalizer([stmt]() {
            sqlite3_stmt *sqlite3Stmt = stmt;
            int ret = E_OK;
            SQLiteUtils::ResetStatement(sqlite3Stmt, true, ret);
        });
        errCode = BindOneRowUpdateStmt(i, stmt, 0, schema.fields);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = SQLiteUtils::StepNext(stmt);
        if (errCode != -E_FINISHED) {
            return errCode;
        }
    }
    return E_OK;
}

int RDBDataGenerator::BindOneRowStmt(int64_t index, sqlite3_stmt *stmt, int cid, bool withoutPk,
    const std::vector<DistributedDB::Field> &fields)
{
    for (const auto &field : fields) {
        if (withoutPk && field.primary) {
            continue;
        }
        cid++;
        auto type = GetColValueByType(index, field);
        int errCode = BindOneColStmt(cid, stmt, type);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

int RDBDataGenerator::BindOneRowUpdateStmt(int64_t index, sqlite3_stmt *stmt, int cid,
    const std::vector<DistributedDB::Field> &fields)
{
    std::vector<Field> pkFields;
    for (const auto &field : fields) {
        if (field.primary) {
            pkFields.push_back(field);
            continue;
        }
        cid++;
        auto type = GetColValueByType(index, field);
        int errCode = BindOneColStmt(cid, stmt, type);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return BindOneRowStmt(index, stmt, cid, false, pkFields);
}

int RDBDataGenerator::BindOneColStmt(int cid, sqlite3_stmt *stmt, const DistributedDB::Type &type)
{
    switch (type.index()) {
        case DistributedDB::TYPE_INDEX<int64_t>:
            return SQLiteUtils::BindInt64ToStatement(stmt, cid, std::get<int64_t>(type));
        case DistributedDB::TYPE_INDEX<std::string>:
            return SQLiteUtils::BindTextToStatement(stmt, cid, std::get<std::string>(type));
        case DistributedDB::TYPE_INDEX<DistributedDB::Assets>: {
            auto assets = std::get<Assets>(type);
            std::vector<uint8_t> blob;
            int errCode = RuntimeContext::GetInstance()->AssetsToBlob(assets, blob);
            if (errCode != E_OK) {
                return errCode;
            }
            return SQLiteUtils::BindBlobToStatement(stmt, cid, blob);
        }
        case DistributedDB::TYPE_INDEX<DistributedDB::Asset>: {
            auto assets = std::get<Asset>(type);
            std::vector<uint8_t> blob;
            int errCode = RuntimeContext::GetInstance()->AssetToBlob(assets, blob);
            if (errCode != E_OK) {
                return errCode;
            }
            return SQLiteUtils::BindBlobToStatement(stmt, cid, blob);
        }
    }
    return E_OK;
}

std::string RDBDataGenerator::GetUpsertSQL(const DistributedDB::TableSchema &schema)
{
    std::string sql = "INSERT INTO " + schema.name + "(";
    for (const auto &field : schema.fields) {
        sql += field.colName + ",";
    }
    sql.pop_back();
    sql += ") VALUES(";
    std::string pkFields;
    std::string noPkFields;
    for (const auto &field : schema.fields) {
        sql += "?,";
        if (field.primary) {
            pkFields += field.colName + ",";
        } else {
            noPkFields += field.colName + "=?,";
        }
    }
    pkFields.pop_back();
    noPkFields.pop_back();
    sql.pop_back();
    sql += ") ON CONFLICT(" + pkFields + ") DO UPDATE SET " + noPkFields;
    LOGI("upsert sql is %s", sql.c_str());
    return sql;
}

std::string RDBDataGenerator::GetUpdateSQL(const TableSchema &schema)
{
    std::string pkFields;
    std::string noPkFields;
    for (const auto &field : schema.fields) {
        if (field.primary) {
            pkFields += field.colName + "=?,";
        } else {
            noPkFields += field.colName + "=?,";
        }
    }
    pkFields.pop_back();
    noPkFields.pop_back();
    std::string sql = "UPDATE " + schema.name + " SET ";
    sql += noPkFields + " WHERE " + pkFields;
    LOGI("upsert sql is %s", sql.c_str());
    return sql;
}

int RDBDataGenerator::InsertVirtualLocalDBData(int64_t begin, int64_t count,
    DistributedDB::RelationalVirtualDevice *device, const DistributedDB::TableSchema &schema)
{
    if (device == nullptr) {
        return -E_INVALID_ARGS;
    }
    std::vector<VirtualRowData> rows;
    for (int64_t index = begin; index < count; ++index) {
        VirtualRowData virtualRowData;
        for (const auto &field : schema.fields) {
            auto type = GetColValueByType(index, field);
            FillTypeIntoDataValue(field, type, virtualRowData);
        }
        virtualRowData.logInfo.timestamp = TimeHelper::GetSysCurrentTime() + TimeHelper::BASE_OFFSET;
        rows.push_back(virtualRowData);
    }
    return device->PutData(schema.name, rows);
}

DistributedDB::DistributedSchema RDBDataGenerator::ParseSchema(const DistributedDB::DataBaseSchema &schema,
    bool syncOnlyPk)
{
    DistributedDB::DistributedSchema res;
    for (const auto &item : schema.tables) {
        DistributedTable table;
        for (const auto &field : item.fields) {
            DistributedField distributedField;
            distributedField.isP2pSync = syncOnlyPk ? field.primary : true;
            distributedField.colName = field.colName;
            table.fields.push_back(distributedField);
        }
        table.tableName = item.name;
        res.tables.push_back(table);
    }
    return res;
}

void RDBDataGenerator::FillTypeIntoDataValue(const DistributedDB::Field &field, const DistributedDB::Type &type,
    DistributedDB::VirtualRowData &virtualRow)
{
    DataValue dataValue;
    std::string hash;
    switch (type.index()) {
        case DistributedDB::TYPE_INDEX<int64_t>:
            if (field.primary) {
                hash = std::to_string(std::get<int64_t>(type));
            }
            dataValue = std::get<int64_t>(type);
            break;
        case DistributedDB::TYPE_INDEX<std::string>:
            if (field.primary) {
                hash = std::get<std::string>(type);
            }
            dataValue = std::get<std::string>(type);
            break;
        default:
            return;
    }
    if (field.primary) {
        std::vector<uint8_t> blob;
        DBCommon::StringToVector(hash, blob);
        DBCommon::CalcValueHash(blob, virtualRow.logInfo.hashKey);
    }
    virtualRow.objectData.PutDataValue(field.colName, dataValue);
}

int RDBDataGenerator::PrepareVirtualDeviceEnv(const std::string &tableName, sqlite3 *db,
    DistributedDB::RelationalVirtualDevice *device)
{
    return PrepareVirtualDeviceEnv(tableName, tableName, db, device);
}

int RDBDataGenerator::PrepareVirtualDeviceEnv(const std::string &scanTable, const std::string &expectTable, sqlite3 *db,
    DistributedDB::RelationalVirtualDevice *device)
{
    TableInfo tableInfo;
    int errCode = SQLiteUtils::AnalysisSchema(db, scanTable, tableInfo);
    if (errCode != E_OK) {
        return errCode;
    }
    tableInfo.SetTableName(expectTable);
    device->SetLocalFieldInfo(tableInfo.GetFieldInfos());
    device->SetTableInfo(tableInfo);
    return E_OK;
}

DistributedDB::TableSchema RDBDataGenerator::FlipTableSchema(const DistributedDB::TableSchema &origin)
{
    DistributedDB::TableSchema res;
    res.name = origin.name;
    for (const auto &item : origin.fields) {
        res.fields.insert(res.fields.begin(), item);
    }
    return res;
}
}

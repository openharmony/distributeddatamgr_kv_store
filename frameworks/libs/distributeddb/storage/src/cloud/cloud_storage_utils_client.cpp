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

#include "cloud/cloud_storage_utils.h"

#include <sstream>
#include "db_common.h"
#include "runtime_context.h"

namespace DistributedDB {
int CloudStorageUtils::Int64ToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
    std::vector<uint8_t> &value)
{
    (void)collateType;
    int64_t val = 0;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    DBCommon::StringToVector(std::to_string(val), value);
    return E_OK;
}

int CloudStorageUtils::BoolToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
    std::vector<uint8_t> &value)
{
    (void)collateType;
    bool val = false;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    DBCommon::StringToVector(std::to_string(val ? 1 : 0), value);
    return E_OK;
}

int CloudStorageUtils::DoubleToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
    std::vector<uint8_t> &value)
{
    (void)collateType;
    double val = 0.0;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    std::ostringstream s;
    s << val;
    DBCommon::StringToVector(s.str(), value);
    return E_OK;
}

int CloudStorageUtils::TextToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
    std::vector<uint8_t> &value)
{
    std::string val;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    if (collateType == CollateType::COLLATE_NOCASE) {
        std::transform(val.begin(), val.end(), val.begin(), ::toupper);
    } else if (collateType == CollateType::COLLATE_RTRIM) {
        DBCommon::RTrim(val);
    }

    DBCommon::StringToVector(val, value);
    return E_OK;
}

int CloudStorageUtils::BlobToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
    std::vector<uint8_t> &value)
{
    (void)collateType;
    if (field.type == TYPE_INDEX<Bytes>) {
        return CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, value);
    } else if (field.type == TYPE_INDEX<Asset>) {
        Asset val;
        if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
            return -E_CLOUD_ERROR;
        }
#ifdef RDB_CLIENT
        return E_OK;
#else
        int errCode = RuntimeContext::GetInstance()->AssetToBlob(val, value);
        if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            LOGE("asset to blob fail, %d", errCode);
        }
        return errCode;
#endif
    } else {
        Assets val;
        if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) { // LCOV_EXCL_BR_LINE
            return -E_CLOUD_ERROR;
        }
#ifdef RDB_CLIENT
        return E_OK;
#else
        int errCode = RuntimeContext::GetInstance()->AssetsToBlob(val, value);
        if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            LOGE("assets to blob fail, %d", errCode);
        }
        return errCode;
#endif
    }
}

int CloudStorageUtils::CalculateHashKeyForOneField(const Field &field, const VBucket &vBucket, bool allowEmpty,
    CollateType collateType, std::vector<uint8_t> &hashValue)
{
    Type type;
    bool isExisted = GetTypeCaseInsensitive(field.colName, vBucket, type);
    if (allowEmpty && !isExisted) {
        return E_OK; // if vBucket from cloud doesn't contain primary key and allowEmpty, no need to calculate hash
    }
    static std::map<int32_t, std::function<int(const VBucket &, const Field &, CollateType,
        std::vector<uint8_t> &)>> toVecFunc = {
        { TYPE_INDEX<int64_t>, &CloudStorageUtils::Int64ToVector },
        { TYPE_INDEX<bool>, &CloudStorageUtils::BoolToVector },
        { TYPE_INDEX<double>, &CloudStorageUtils::DoubleToVector },
        { TYPE_INDEX<std::string>, &CloudStorageUtils::TextToVector },
        { TYPE_INDEX<Bytes>, &CloudStorageUtils::BlobToVector },
        { TYPE_INDEX<Asset>, &CloudStorageUtils::BlobToVector },
        { TYPE_INDEX<Assets>, &CloudStorageUtils::BlobToVector },
    };
    auto it = toVecFunc.find(field.type);
    if (it == toVecFunc.end()) {
        LOGE("unknown cloud type when convert field to vector.");
        return -E_CLOUD_ERROR;
    }
    std::vector<uint8_t> value;
    int errCode = it->second(vBucket, field, collateType, value);
    if (errCode != E_OK) {
        LOGE("convert cloud field fail, %d", errCode);
        return errCode;
    }
    return DBCommon::CalcValueHash(value, hashValue);
}

void CloudStorageUtils::TransferFieldToLower(VBucket &vBucket)
{
    for (auto it = vBucket.begin(); it != vBucket.end();) {
        std::string lowerField(it->first.length(), ' ');
        std::transform(it->first.begin(), it->first.end(), lowerField.begin(), ::tolower);
        if (lowerField != it->first) {
            vBucket[lowerField] = std::move(vBucket[it->first]);
            vBucket.erase(it++);
        } else {
            it++;
        }
    }
}

bool CloudStorageUtils::GetTypeCaseInsensitive(const std::string &fieldName, const VBucket &vBucket, Type &data)
{
    auto tmpFieldName = fieldName;
    auto tmpVBucket = vBucket;
    std::transform(tmpFieldName.begin(), tmpFieldName.end(), tmpFieldName.begin(), ::tolower);
    TransferFieldToLower(tmpVBucket);
    auto it = tmpVBucket.find(tmpFieldName);
    if (it == tmpVBucket.end()) {
        return false;
    }
    data = it->second;
    return true;
}

std::string CloudStorageUtils::GetSelectIncCursorSql(const std::string &tableName)
{
    return "(SELECT value FROM " + DBCommon::GetMetaTableName() + " WHERE key=x'" +
        DBCommon::TransferStringToHex(DBCommon::GetCursorKey(tableName)) + "')";
}

std::string CloudStorageUtils::GetCursorIncSql(const std::string &tableName)
{
    return "UPDATE " + DBCommon::GetMetaTableName() + " SET value=value+1 WHERE key=x'" +
        DBCommon::TransferStringToHex(DBCommon::GetCursorKey(tableName)) + "';";
}

std::string CloudStorageUtils::GetCursorIncSqlWhenAllow(const std::string &tableName)
{
    std::string prefix = DBConstant::RELATIONAL_PREFIX;
    return "UPDATE " + prefix + "metadata" + " SET value= case when (select 1 from " +
        prefix + "metadata" + " where key='cursor_inc_flag' AND value = 'true') then value + 1" +
        " else value end WHERE key=x'" + DBCommon::TransferStringToHex(DBCommon::GetCursorKey(tableName)) + "';";
}

std::string CloudStorageUtils::GetUpdateLockChangedSql()
{
    return " status = CASE WHEN status == 2 THEN 3 ELSE status END";
}

std::string CloudStorageUtils::GetDeleteLockChangedSql()
{
    return " status = CASE WHEN status == 2 or status == 3 THEN 1 ELSE status END";
}

std::string CloudStorageUtils::GetTableRefUpdateSql(const TableInfo &table, OpType opType)
{
    std::string sql;
    std::string rowid = std::string(DBConstant::SQLITE_INNER_ROWID);
    for (const auto &reference : table.GetTableReference()) {
        if (reference.columns.empty()) {
            return "";
        }
        std::string sourceLogName = DBCommon::GetLogTableName(reference.sourceTableName);
        sql += " UPDATE " + sourceLogName + " SET timestamp=get_raw_sys_time(), flag=flag|0x02 WHERE ";
        int index = 0;
        for (const auto &itCol : reference.columns) {
            if (opType != OpType::UPDATE) {
                continue;
            }
            if (index++ != 0) {
                sql += " OR ";
            }
            sql += " (OLD." + itCol.second + " IS NOT " + " NEW." + itCol.second + ")";
        }
        if (opType == OpType::UPDATE) {
            sql += " AND ";
        }
        sql += " (flag&0x08=0x00) AND data_key IN (SELECT " + sourceLogName + ".data_key FROM " + sourceLogName +
            " LEFT JOIN " + reference.sourceTableName + " ON " + sourceLogName + ".data_key = " +
            reference.sourceTableName + "." + rowid + " WHERE ";
        index = 0;
        for (const auto &itCol : reference.columns) {
            if (index++ != 0) {
                sql += " OR ";
            }
            if (opType == OpType::UPDATE) {
                sql += itCol.first + "=OLD." + itCol.second + " OR " + itCol.first + "=NEW." + itCol.second;
            } else if (opType == OpType::INSERT) {
                sql += itCol.first + "=NEW." + itCol.second;
            } else if (opType == OpType::DELETE) {
                sql += itCol.first + "=OLD." + itCol.second;
            }
        }
        sql += ");";
    }
    return sql;
}
}

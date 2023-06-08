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
#include "sqlite_single_ver_relational_storage_executor.h"

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"

namespace DistributedDB {
int SQLiteSingleVerRelationalStorageExecutor::GetQueryInfoSql(const std::string &tableName, const VBucket &vBucket,
    std::set<std::string> &pkSet, std::vector<Field> &assetFields, std::string &querySql)
{
    if (assetFields.empty()) {
        return GetQueryLogSql(tableName, vBucket, pkSet, querySql);
    }
    std::string cloudGid;
    int errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, vBucket, cloudGid);
    if (errCode != E_OK) {
        LOGE("Get cloud gid fail when query log table.");
        return errCode;
    }

    if (pkSet.empty() && cloudGid.empty()) {
        LOGE("query log table failed because of both primary key and gid are empty.");
        return -E_CLOUD_ERROR;
    }
    std::string sql = "select a.data_key, a.device, a.ori_device, a.timestamp, a.wtimestamp, a.flag, a.hash_key,"
        " a.cloud_gid";
    for (const auto &field: assetFields) {
        sql += ", b." + field.colName;
    }
    sql += " from '" + DBCommon::GetLogTableName(tableName) + "' AS a LEFT JOIN '" + tableName + "' AS b ";
    sql += " ON (a.data_key = b.rowid) WHERE ";
    if (!cloudGid.empty()) {
        sql += " a.cloud_gid = ? or ";
    }
    sql += "a.hash_key = ?";
    querySql = sql;
    return E_OK;
}

} // namespace DistributedDB
#endif
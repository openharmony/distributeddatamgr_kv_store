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
#include "db_errno.h"
#include "log_print.h"
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_sync_utils.h"

namespace DistributedDB {

int GetCloudPkVals(const VBucket &datum, const std::vector<std::string> &pkColNames, int64_t dataKey,
    std::vector<Type> &cloudPkVals)
{
    if (!cloudPkVals.empty()) {
        LOGE("[CloudSyncer] Output paramater should be empty");
        return -E_INVALID_ARGS;
    }
    if (pkColNames.size() == 1 && pkColNames[0] == CloudDbConstant::ROW_ID_FIELD_NAME) {
        // if data don't have primary key, then use rowID as value
        cloudPkVals.push_back(dataKey);
        return E_OK;
    }
    for (const auto &pkColName : pkColNames) {
        auto iter = datum.find(pkColName);
        if (iter == datum.end()) {
            LOGE("[CloudSyncer] Cloud data do not contain expected primary field value");
            return -E_CLOUD_ERROR;
        }
        cloudPkVals.push_back(iter->second);
    }
    return E_OK;
}

ChangeType OpTypeToChangeType(OpType strategy)
{
    switch (strategy) {
        case OpType::INSERT:
            return OP_INSERT;
        case OpType::DELETE:
            return OP_DELETE;
        case OpType::UPDATE:
            return OP_UPDATE;
        default:
            return OP_BUTT;
    }
}


bool IsCompositeKey(const std::vector<std::string> &pKColNames)
{
    return (pKColNames.size() > 1);
}

bool IsNoPrimaryKey(const std::vector<std::string> &pKColNames)
{
    if (pKColNames.empty()) {
        return true;
    }
    if (pKColNames.size() == 1 && pKColNames[0] == CloudDbConstant::ROW_ID_FIELD_NAME) {
        return true;
    }
    return false;
}

bool IsSinglePrimaryKey(const std::vector<std::string> &pKColNames)
{
    if (IsCompositeKey(pKColNames) || IsNoPrimaryKey(pKColNames)) {
        return false;
    }
    return true;
}

int GetSinglePk(const VBucket &datum, const std::vector<std::string> &pkColNames, int64_t dataKey, Type &pkVal)
{
    std::vector<Type> pkVals;
    int ret = E_OK;
    if (IsSinglePrimaryKey(pkColNames)) {
        ret = GetCloudPkVals(datum, pkColNames, dataKey, pkVals);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Cannot get single primary key for the datum %d", ret);
            return ret;
        }
        pkVal = pkVals[0];
        if (pkVal.index() == TYPE_INDEX<Nil>) {
            LOGE("[CloudSyncer] Invalid primary key type in TagStatus, it's Nil.");
            return -E_INTERNAL_ERROR;
        }
    }
    return E_OK;
}

void RemoveDataExceptExtendInfo(VBucket &datum, const std::vector<std::string> &pkColNames)
{
    for (auto item = datum.begin(); item != datum.end();) {
        const auto &key = item->first;
        if (key != CloudDbConstant::GID_FIELD &&
            key != CloudDbConstant::CREATE_FIELD &&
            key != CloudDbConstant::MODIFY_FIELD &&
            key != CloudDbConstant::DELETE_FIELD &&
            key != CloudDbConstant::CURSOR_FIELD &&
            (std::find(pkColNames.begin(), pkColNames.end(), key) == pkColNames.end())) {
                item = datum.erase(item);
            } else {
                item++;
            }
    }
}
}
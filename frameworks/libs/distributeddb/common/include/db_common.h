/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDB_COMMON_H
#define DISTRIBUTEDDB_COMMON_H

#include <list>
#include <string>

#include "cloud/cloud_db_types.h"
#include "db_types.h"
#include "kvdb_properties.h"
#include "store_types.h"

namespace DistributedDB {
class DBCommon final {
public:
    static int CreateDirectory(const std::string &directory);

    static void StringToVector(const std::string &src, std::vector<uint8_t> &dst);
    static void VectorToString(const std::vector<uint8_t> &src, std::string &dst);

    static inline std::string GetLogTableName(const std::string &tableName)
    {
        return DBConstant::RELATIONAL_PREFIX + tableName + DBConstant::LOG_POSTFIX;
    }

    static inline const std::vector<CloudWaterType> GetWaterTypeVec()
    {
        return {CloudWaterType::DELETE, CloudWaterType::UPDATE, CloudWaterType::INSERT};
    }

    static inline std::string GetMetaTableName()
    {
        return std::string(DBConstant::RELATIONAL_PREFIX) + DBConstant::META_TABLE_POSTFIX;
    }

    static std::string VectorToHexString(const std::vector<uint8_t> &inVec, const std::string &separator = "");

    static void PrintHexVector(const std::vector<uint8_t> &data, int line = 0, const std::string &tag = "");

    static std::string TransferStringToHex(const std::string &origStr);

    static std::string TransferHashString(const std::string &devName);

    static int CalcValueHash(const std::vector<uint8_t> &Value, std::vector<uint8_t> &hashValue);

    static int CreateStoreDirectory(const std::string &directory, const std::string &identifierName,
        const std::string &subDir, bool isCreate);

    static int CopyFile(const std::string &srcFile, const std::string &dstFile);

    static int RemoveAllFilesOfDirectory(const std::string &dir, bool isNeedRemoveDir = true);

    static std::string GenerateIdentifierId(const std::string &storeId,
        const std::string &appId, const std::string &userId, const std::string &subUser = "", int32_t instanceId = 0);

    static std::string GenerateDualTupleIdentifierId(const std::string &storeId, const std::string &appId);

    static void SetDatabaseIds(KvDBProperties &properties, const DbIdParam &dbIdParam);

    static std::string StringMasking(const std::string &oriStr, size_t remain = 3); // remain 3 unmask

    static std::string StringMiddleMasking(const std::string &name);

    static std::string GetDistributedTableName(const std::string &device, const std::string &tableName);

    static std::string GetDistributedTableName(const std::string &device, const std::string &tableName,
        const StoreInfo &info);

    static std::string GetDistributedTableNameWithHash(const std::string &device, const std::string &tableName);

    static std::string CalDistributedTableName(const std::string &device, const std::string &tableName);

    static void GetDeviceFromName(const std::string &deviceTableName, std::string &deviceHash, std::string &tableName);

    static std::string TrimSpace(const std::string &input);

    static void RTrim(std::string &oriString);

    static bool HasConstraint(const std::string &sql, const std::string &keyWord, const std::string &prePattern,
        const std::string &nextPattern);

    static bool IsSameCipher(CipherType srcType, CipherType inputType);

    static std::string ToLowerCase(const std::string &str);

    static std::string ToUpperCase(const std::string &str);

    static bool CaseInsensitiveCompare(const std::string &first, const std::string &second);

    static bool CheckIsAlnumOrUnderscore(const std::string &text);

    static bool CheckQueryWithoutMultiTable(const Query &query);

    static bool IsCircularDependency(int size, const std::vector<std::vector<int>> &dependency);

    static int SerializeWaterMark(Timestamp localMark, const std::string &cloudMark, Value &blobMeta);

    static Key GetPrefixTableName(const TableName &tableName);

    static std::list<std::string> GenerateNodesByNodeWeight(const std::vector<std::string> &nodes,
        const std::map<std::string, std::map<std::string, bool>> &graph,
        const std::map<std::string, int> &nodeWeight);

    static bool HasPrimaryKey(const std::vector<Field> &fields);

    static bool IsRecordError(const VBucket &record);

    static bool IsRecordFailed(const VBucket &record, DBStatus status);

    static bool IsIntTypeRecordError(const VBucket &record);

    static bool IsRecordIgnored(const VBucket &record);

    static bool IsRecordVersionConflict(const VBucket &record);

    static bool IsRecordAssetsMissing(const VBucket &record);

    static bool IsRecordDelete(const VBucket &record);

    static bool IsCloudRecordNotFound(const VBucket &record);

    static bool IsCloudRecordAlreadyExisted(const VBucket &record);

    static bool IsNeedCompensatedForUpload(const VBucket &uploadExtend, const CloudWaterType &type);

    static bool IsRecordIgnoredForReliability(const VBucket &uploadExtend, const CloudWaterType &type);

    static bool IsRecordSuccess(const VBucket &record);

    static std::string GenerateHashLabel(const DBInfo &dbInfo);

    static uint64_t EraseBit(uint64_t origin, uint64_t eraseBit);

    static void *LoadGrdLib(void);

    static void UnLoadGrdLib(void *handle);

    static bool IsGrdLibLoaded(void);

    static bool CheckCloudSyncConfigValid(const CloudSyncConfig &config);

    static std::string GetCursorKey(const std::string &tableName);

    static bool ConvertToUInt64(const std::string &str, uint64_t &value);

    static void RemoveDuplicateAssetsData(std::vector<Asset> &assets);

    static std::set<std::string, CaseInsensitiveComparator> TransformToCaseInsensitive(
        const std::vector<std::string> &origin);

    static std::string GetStoreIdentifier(const StoreInfo &info, const std::string &subUser, bool syncDualTupleMode,
        bool allowStoreIdWithDot);
private:
    static void InsertNodesByScore(const std::map<std::string, std::map<std::string, bool>> &graph,
        const std::vector<std::string> &generateNodes, const std::map<std::string, int> &scoreGraph,
        std::list<std::string> &insertTarget);
};

// Define short macro substitute for original long expression for convenience of using
#define VEC_TO_STR(x) DBCommon::VectorToHexString(x).c_str()
#define STR_MASK(x) DBCommon::StringMasking(x).c_str()
#define STR_TO_HEX(x) DBCommon::TransferStringToHex(x).c_str()
} // namespace DistributedDB

#endif // DISTRIBUTEDDB_COMMON_H

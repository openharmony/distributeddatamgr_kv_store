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

#ifndef PARAM_CHECK_UTILS_H
#define PARAM_CHECK_UTILS_H

#include <string>

#include "db_types.h"
#include "auto_launch_export.h"
#include "schema_object.h"

namespace DistributedDB {
class ParamCheckUtils final {
public:

    static bool CheckDataDir(const std::string &dir, std::string &canonicalDir);

    // Check if the storeID is a safe arg.
    static bool IsStoreIdSafe(const std::string &storeId, bool allowStoreIdWithDot = false);

    // check appId, userId, storeId, subUser.
    static bool CheckStoreParameter(const std::string &storeId, const std::string &appId, const std::string &userId,
        bool isIgnoreUserIdCheck = false, const std::string &subUser = "");

    static bool CheckStoreParameter(const StoreInfo &info, bool isIgnoreUserIdCheck = false,
        const std::string &subUser = "", bool allowStoreIdWithDot = false);

    // check encrypted args for KvStore.
    static bool CheckEncryptedParameter(CipherType cipher, const CipherPassword &passwd);

    static bool CheckConflictNotifierType(int conflictType);

    static bool CheckSecOption(const SecurityOption &secOption);

    static bool CheckObserver(const Key &key, unsigned int mode);

    static bool IsS3SECEOpt(const SecurityOption &secOpt);

    static int CheckAndTransferAutoLaunchParam(const AutoLaunchParam &param, bool checkDir,
        SchemaObject &schemaObject, std::string &canonicalDir);

    static uint8_t GetValidCompressionRate(uint8_t compressionRate);

    static bool CheckRelationalTableName(const std::string &tableName);

    static bool CheckTableReference(const std::vector<TableReferenceProperty> &tableReferenceProperty);

    static bool CheckSharedTableName(const DataBaseSchema &schema);

    static void TransferSchemaToLower(DataBaseSchema &schema);

    static bool IsSchemaTablesEmpty(const DistributedSchema &schema);
};
} // namespace DistributedDB
#endif // PARAM_CHECK_UTILS_H

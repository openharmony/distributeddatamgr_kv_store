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

#ifndef SINGLE_VER_UTILS_H
#define SINGLE_VER_UTILS_H

#include "kvdb_properties.h"
#include "storage_executor.h"
#include "types_export.h"

namespace DistributedDB {

const uint64_t CACHE_RECORD_DEFAULT_VERSION = 1;

enum class DbType : int32_t {
    MAIN,
    META,
    CACHE
};

struct OpenDbProperties {
    std::string uri {};
    bool createIfNecessary = true;
    bool isMemDb = false;
    std::vector<std::string> sqls {};
    CipherType cipherType = CipherType::AES_256_GCM;
    CipherPassword passwd {};
    std::string schema {};
    std::string subdir {};
    SecurityOption securityOpt {};
    int conflictReslovePolicy = DEFAULT_LAST_WIN;
    bool createDirByStoreIdOnly = false;
    uint32_t iterTimes = DBConstant::DEFAULT_ITER_TIMES;
    // newly added RD properties
    std::string rdConfig {};
    bool isNeedIntegrityCheck = false;
    bool isNeedRmCorruptedDb = false;
    bool readOnly = false;
    bool isHashTable = false;
};

int GetPathSecurityOption(const std::string &filePath, SecurityOption &secOpt);

std::string GetDbDir(const std::string &subDir, DbType type);

std::string GetDatabasePath(const KvDBProperties &kvDBProp);

std::string GetSubDirPath(const KvDBProperties &kvDBProp);

int ClearIncompleteDatabase(const KvDBProperties &kvDBPro);

int CreateNewDirsAndSetSecOption(const OpenDbProperties &option);

int GetExistedSecOpt(const OpenDbProperties &option, SecurityOption &secOption);

void InitCommitNotifyDataKeyStatus(SingleVerNaturalStoreCommitNotifyData *committedData, const Key &hashKey,
    const DataOperStatus &dataStatus);
} // namespace DistributedDB

#endif // SINGLE_VER_UTILS_H
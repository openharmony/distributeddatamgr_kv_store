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

#ifndef RD_SINGLE_VER_STORAGE_ENGINE_H
#define RD_SINGLE_VER_STORAGE_ENGINE_H

#include "db_common.h"
#include "grd_db_api.h"
#include "param_check_utils.h"
#include "platform_specific.h"
#include "runtime_context.h"
#include "single_ver_utils.h"
#include "sqlite_utils.h"
#include "storage_engine.h"

namespace DistributedDB {
class RdSingleVerStorageEngine : public StorageEngine {
public:
    RdSingleVerStorageEngine();
    ~RdSingleVerStorageEngine() override;

    int InitRdStorageEngine(const StorageEngineAttr &poolSize, const OpenDbProperties &option,
        const std::string &identifier = std::string());

protected:
    int CreateNewExecutor(bool isWrite, StorageExecutor *&handle) override;

private:
    int PreCreateExecutor(bool isWrite);

    int GetExistedSecOption(SecurityOption &secOption) const;

    int CheckDatabaseSecOpt(const SecurityOption &secOption) const;

    int CreateNewDirsAndSetSecOpt() const;

    int TryToOpenMainDatabase(bool isWrite, GRD_DB *&db);

    int GetDbHandle(bool isWrite, const SecurityOption &secOpt, GRD_DB *&dbHandle);

    int OpenGrdDb(const OpenDbProperties &option, GRD_DB *&db);

    int IndexPreLoad(GRD_DB *&db, const char *collectionName);

    std::atomic<bool> crcCheck_ = false;
};
} // namespace DistributedDB
#endif // RD_SINGLE_VER_STORAGE_ENGINE_H
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

#ifndef OHOS_DISTRIBUTED_DATA_STORE_UTIL_MOCK_H
#define OHOS_DISTRIBUTED_DATA_STORE_UTIL_MOCK_H

#include <gmock/gmock.h>

#include "store_types.h"
#include "store_util.h"

namespace OHOS::DistributedKv {
class BStoreUtil {
public:
    virtual bool IsFileExist(const std::string &) = 0;
    virtual bool Remove(const std::string &) = 0;
    virtual Status ConvertStatus(StoreUtil::DBStatus) = 0;
    virtual bool InitPath(const std::string) = 0;
    virtual StoreUtil::DBSecurity GetDBSecurity(int32_t) = 0;
    virtual StoreUtil::DBIndexType GetDBIndexType(IndexType) = 0;
    virtual std::vector<std::string> GetSubPath(const std::string &) = 0;
    virtual std::vector<StoreUtil::FileInfo> GetFiles(const std::string &) = 0;
    virtual bool Rename(const std::string &, const std::string &) = 0;
    virtual uint32_t Anonymous(const void *) = 0;
    virtual bool RemoveRWXForOthers(const std::string &) = 0;
    virtual std::string Anonymous(const std::string &) = 0;
    virtual bool CreateFile(const std::string &) = 0;
    BStoreUtil() = default;
    virtual ~BStoreUtil() = default;

public:
    static inline std::shared_ptr<BStoreUtil> storeUtil = nullptr;
};

class StoreUtilMock : public BStoreUtil {
public:
    MOCK_METHOD(bool, IsFileExist, (const std::string &));
    MOCK_METHOD(bool, Remove, (const std::string &));
    MOCK_METHOD(Status, ConvertStatus, (StoreUtil::DBStatus));
    MOCK_METHOD(StoreUtil::DBSecurity, GetDBSecurity, (int32_t));
    MOCK_METHOD(bool, InitPath, (const std::string));
    MOCK_METHOD(StoreUtil::DBIndexType, GetDBIndexType, (IndexType));
    MOCK_METHOD(std::vector<std::string>, GetSubPath, (const std::string &));
    MOCK_METHOD(std::vector<StoreUtil::FileInfo>, GetFiles, (const std::string &));
    MOCK_METHOD(bool, Rename, (const std::string &, const std::string &));
    MOCK_METHOD(uint32_t, Anonymous, (const void *));
    MOCK_METHOD(std::string, Anonymous, (const std::string &));
    MOCK_METHOD(bool, RemoveRWXForOthers, (const std::string &));
    MOCK_METHOD(bool, CreateFile, (const std::string &));
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_STORE_UTIL_MOCK_H
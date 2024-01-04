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
#ifndef MOCK_ICLOUD_DB_H
#define MOCK_ICLOUD_DB_H
#include <gmock/gmock.h>
#include "icloud_db.h"

namespace DistributedDB {

class MockICloudDB : public ICloudDb {
public:
    MOCK_METHOD3(BatchInsert, DBStatus(const std::string &, std::vector<VBucket> &&,
        std::vector<VBucket> &));
    MOCK_METHOD3(BatchUpdate, DBStatus(const std::string &, std::vector<VBucket> &&,
        std::vector<VBucket> &));
    MOCK_METHOD2(BatchDelete, DBStatus(const std::string &, std::vector<VBucket> &));
    MOCK_METHOD3(Query, DBStatus(const std::string &, VBucket &, std::vector<VBucket> &));
    MOCK_METHOD1(GetEmptyCursor, std::pair<DBStatus, std::string>(const std::string &));
    MOCK_METHOD0(Lock, std::pair<DBStatus, uint32_t>(void));
    MOCK_METHOD0(UnLock, DBStatus(void));
    MOCK_METHOD0(HeartBeat, DBStatus(void));
    MOCK_METHOD0(Close, DBStatus(void));
};
}
#endif // #define MOCK_ICLOUD_DB_H
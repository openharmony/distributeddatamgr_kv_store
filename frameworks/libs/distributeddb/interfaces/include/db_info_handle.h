/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef DB_INFO_HANDLE_H
#define DB_INFO_HANDLE_H

#include "iprocess_communicator.h"
#include "store_types.h"

namespace DistributedDB {
// For all functions with returnType DBStatus:
// return DBStatus::OK if successful, otherwise DBStatus::DB_ERROR if anything wrong.
class DBInfoHandle {
public:
    DBInfoHandle() = default;
    virtual ~DBInfoHandle() = default;
    // return true if you can notify with RuntimeConfig::NotifyDBInfo
    virtual bool IsSupport() = 0;
    // check is need auto sync when remote db is open
    // return true if data has changed
    virtual bool IsNeedAutoSync(const std::string &userId, const std::string &appId, const std::string &storeId,
        const DeviceInfos &devInfo) = 0;
};
}
#endif // DB_INFO_HANDLE_H

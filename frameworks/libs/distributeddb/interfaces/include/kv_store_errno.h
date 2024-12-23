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

#ifndef KV_STORE_ERRNO_H
#define KV_STORE_ERRNO_H

#include "store_types.h"

namespace DistributedDB {
// Transfer the db error code to the DBStatus.
DBStatus TransferDBErrno(int err, bool isPass = false);

// Transfer the DBStatus to db error code, return dbStatus if no matching error code
int TransferDBStatusToErr(DBStatus dbStatus);
} // DistributedDB
#endif // KV_STORE_ERRNO_H
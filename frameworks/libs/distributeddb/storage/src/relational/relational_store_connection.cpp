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
#ifdef RELATIONAL_STORE
#include "relational_store_connection.h"
#include "db_errno.h"

namespace DistributedDB {
RelationalStoreConnection::RelationalStoreConnection() : isExclusive_(false)
{}

RelationalStoreConnection::RelationalStoreConnection(IRelationalStore *store)
    : store_(store), isExclusive_(false)
{}

int RelationalStoreConnection::Pragma(int cmd, void *parameter)
{
    (void) cmd;
    (void) parameter;
    return E_OK;
}
}
#endif
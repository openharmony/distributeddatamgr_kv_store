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
#include "kvdb_windowed_result_set.h"

#include "db_errno.h"

namespace DistributedDB {
KvDBWindowedResultSet::KvDBWindowedResultSet()
    : window_(nullptr)
{
}

KvDBWindowedResultSet::~KvDBWindowedResultSet()
{
    window_ = nullptr;
}

int KvDBWindowedResultSet::Open(bool isMemDb)
{
    return -E_NOT_SUPPORT;
}

int KvDBWindowedResultSet::GetCount() const
{
    return 0;
}

int KvDBWindowedResultSet::GetPosition() const
{
    return -1; // return invalid position
}

int KvDBWindowedResultSet::Move(int offset) const
{
    return -E_NOT_SUPPORT;
}

int KvDBWindowedResultSet::MoveTo(int position) const
{
    return -E_NOT_SUPPORT;
}

int KvDBWindowedResultSet::MoveToFirst()
{
    return -E_NOT_SUPPORT;
}

int KvDBWindowedResultSet::MoveToLast()
{
    return -E_NOT_SUPPORT;
}

bool KvDBWindowedResultSet::IsFirst() const
{
    return -E_NOT_SUPPORT;
}

bool KvDBWindowedResultSet::IsLast() const
{
    return -E_NOT_SUPPORT;
}

bool KvDBWindowedResultSet::IsBeforeFirst() const
{
    return -E_NOT_SUPPORT;
}

bool KvDBWindowedResultSet::IsAfterLast() const
{
    return -E_NOT_SUPPORT;
}

int KvDBWindowedResultSet::GetEntry(Entry &entry) const
{
    return -E_NOT_SUPPORT;
}

int KvDBWindowedResultSet::Close()
{
    return E_OK;
}
} // namespace DistributedDB

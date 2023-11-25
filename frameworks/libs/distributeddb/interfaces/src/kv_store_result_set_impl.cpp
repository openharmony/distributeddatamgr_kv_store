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

#include "kv_store_result_set_impl.h"

#include "db_errno.h"

namespace DistributedDB {
const int KvStoreResultSetImpl::INIT_POSITION = -1;

KvStoreResultSetImpl::KvStoreResultSetImpl(IKvDBResultSet *resultSet)
    : resultSet_(resultSet)
{
}

int KvStoreResultSetImpl::GetCount() const
{
    if (resultSet_ == nullptr) {
        return 0;
    }
    return resultSet_->GetCount();
}

int KvStoreResultSetImpl::GetPosition() const
{
    if (resultSet_ == nullptr) {
        return INIT_POSITION;
    }
    return resultSet_->GetPosition();
}

bool KvStoreResultSetImpl::Move(int offset)
{
    if (resultSet_ == nullptr) {
        return false;
    }
    if (resultSet_->Move(offset) == E_OK) {
        return true;
    }
    return false;
}

bool KvStoreResultSetImpl::MoveToPosition(int position)
{
    if (resultSet_ == nullptr) {
        return false;
    }
    if (resultSet_->MoveTo(position) == E_OK) {
        return true;
    }
    return false;
}

bool KvStoreResultSetImpl::MoveToFirst()
{
    if (resultSet_ == nullptr) {
        return false;
    }
    if (resultSet_->MoveToFirst() == E_OK) {
        return true;
    }
    return false;
}

bool KvStoreResultSetImpl::MoveToLast()
{
    if (resultSet_ == nullptr) {
        return false;
    }
    if (resultSet_->MoveToLast() == E_OK) {
        return true;
    }
    return false;
}

bool KvStoreResultSetImpl::MoveToNext()
{
    // move 1 step forward in this result set
    return Move(1);
}

bool KvStoreResultSetImpl::MoveToPrevious()
{
    // move 1 step backward in this result set
    return Move(-1);
}

bool KvStoreResultSetImpl::IsFirst() const
{
    if (resultSet_ == nullptr) {
        return false;
    }
    return resultSet_->IsFirst();
}

bool KvStoreResultSetImpl::IsLast() const
{
    if (resultSet_ == nullptr) {
        return false;
    }
    return resultSet_->IsLast();
}

bool KvStoreResultSetImpl::IsBeforeFirst() const
{
    if (resultSet_ == nullptr) {
        return false;
    }
    return resultSet_->IsBeforeFirst();
}

bool KvStoreResultSetImpl::IsAfterLast() const
{
    if (resultSet_ == nullptr) {
        return false;
    }
    return resultSet_->IsAfterLast();
}

DBStatus KvStoreResultSetImpl::GetEntry(Entry &entry) const
{
    if (resultSet_ == nullptr) {
        return DB_ERROR;
    }

    if (resultSet_->GetEntry(entry) == E_OK) {
        return OK;
    }
    return NOT_FOUND;
}

void KvStoreResultSetImpl::GetResultSet(IKvDBResultSet *&resultSet) const
{
    resultSet = resultSet_;
}

bool KvStoreResultSetImpl::IsClosed() const
{
    return false;
}

void KvStoreResultSetImpl::Close()
{
    return;
}

void KvStoreResultSetImpl::GetColumnNames(std::vector<std::string> &columnNames) const
{
    (void)columnNames;
    return;
}

DBStatus KvStoreResultSetImpl::GetColumnType(int columnIndex, ColumnType &columnType) const
{
    (void)columnIndex;
    (void)columnType;
    return NOT_SUPPORT;
}

DBStatus KvStoreResultSetImpl::GetColumnIndex(const std::string &columnName, int &columnIndex) const
{
    (void)columnIndex;
    (void)columnName;
    return NOT_SUPPORT;
}

DBStatus KvStoreResultSetImpl::GetColumnName(int columnIndex, std::string &columnName) const
{
    (void)columnIndex;
    (void)columnName;
    return NOT_SUPPORT;
}

DBStatus KvStoreResultSetImpl::Get(int columnIndex, std::vector<uint8_t> &value) const
{
    (void)columnIndex;
    (void)value;
    return NOT_SUPPORT;
}

DBStatus KvStoreResultSetImpl::Get(int columnIndex, std::string &value) const
{
    (void)columnIndex;
    (void)value;
    return NOT_SUPPORT;
}

DBStatus KvStoreResultSetImpl::Get(int columnIndex, int64_t &value) const
{
    (void)columnIndex;
    (void)value;
    return NOT_SUPPORT;
}

DBStatus KvStoreResultSetImpl::Get(int columnIndex, double &value) const
{
    (void)columnIndex;
    (void)value;
    return NOT_SUPPORT;
}

DBStatus KvStoreResultSetImpl::IsColumnNull(int columnIndex, bool &isNull) const
{
    (void)columnIndex;
    (void)isNull;
    return NOT_SUPPORT;
}

DBStatus KvStoreResultSetImpl::GetRow(std::map<std::string, VariantData> &data) const
{
    (void)data;
    return NOT_SUPPORT;
}
} // namespace DistributedDB

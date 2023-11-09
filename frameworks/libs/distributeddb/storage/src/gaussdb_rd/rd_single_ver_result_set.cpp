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
#include "rd_single_ver_result_set.h"
#include "db_errno.h"
#include "grd_db_api.h"
#include "grd_resultset_api.h"
#include "log_print.h"
#include "rd_utils.h"
#include "sqlite_single_ver_storage_executor_sql.h"

namespace DistributedDB {
RdSingleVerResultSet::RdSingleVerResultSet(RdSingleVerNaturalStore *kvDB, const Key &key)
    : type_(ResultSetType::KEYPREFIX), key_(key), kvDB_(kvDB) {}

RdSingleVerResultSet::~RdSingleVerResultSet()
{
    isOpen_ = false;
    position_ = INIT_POSITION;
    kvDB_ = nullptr;
    handle_ = nullptr;
}

int RdSingleVerResultSet::Open(bool isMemDb)
{
    (void)isMemDb;
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (isOpen_) {
        LOGW("[RdSinResSet] Not need to open result set again!");
        return E_OK;
    }
    if (kvDB_ == nullptr) { // Unlikely
        return -E_INVALID_ARGS;
    }
    if (type_ != ResultSetType::KEYPREFIX) {
        LOGE("[RdSinResSet] Open result set only support prefix mode for now.");
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    handle_ = kvDB_->GetHandle(false, errCode);
    if (handle_ == nullptr) {
        LOGE("[RdSinResSet] Get handle failed, errCode=%d.", errCode);
        return errCode;
    }
    errCode = handle_->OpenResultSet(key_, KV_SCAN_PREFIX, &resultSet_);
    if (errCode != E_OK) {
        LOGE("[RdSinResSet] open result set failed, %d.", errCode);
        kvDB_->ReleaseHandle(handle_);
        return errCode;
    }
    isOpen_ = true;
    return E_OK;
}

int RdSingleVerResultSet::Close()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    int errCode = PreCheckResultSet();
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = handle_->CloseResultSet(resultSet_);
    if (errCode != E_OK) {
        LOGE("[RdSinResSet] close rd result set failed, errCode=%d.", errCode);
        kvDB_->ReleaseHandle(handle_);
        return errCode;
    }
    kvDB_->ReleaseHandle(handle_);
    isOpen_ = false;
    position_ = INIT_POSITION;
    return E_OK;
}

int RdSingleVerResultSet::PreCheckResultSet() const
{
    if (!isOpen_) {
        LOGE("[RdSinResSet] Result set not open yet.");
        return -E_RESULT_SET_STATUS_INVALID;
    }
    if (kvDB_ == nullptr) { // Unlikely
        LOGE("[RdSinResSet] KvDB not set yet.");
        return -E_INVALID_ARGS;
    }

    if (handle_ == nullptr) {
        LOGE("[RdSinResSet] Handle not set yet.");
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int RdSingleVerResultSet::GetCount() const
{
    int errCode = PreCheckResultSet();
    if (errCode != E_OK) {
        return errCode;
    }
    int count = 0;
    errCode = handle_->GetCount(key_, count);
    if (errCode != E_OK && errCode != -E_RESULT_SET_EMPTY) {
        return errCode;
    }
    return count;
}

int RdSingleVerResultSet::GetPosition() const
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return position_;
}

int RdSingleVerResultSet::MoveToNext()
{
    int errCode = PreCheckResultSet();
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = handle_->MoveToNext(resultSet_);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[RdSinResSet] move next failed, errCode=%d.", errCode);
        return errCode;
    }
    if (errCode != -E_NOT_FOUND) {
        ++position_;
    }
    return errCode;
}

int RdSingleVerResultSet::MoveToPrev()
{
    int errCode = PreCheckResultSet();
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = handle_->MoveToPrev(resultSet_);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[RdSinResSet] move prev failed, errCode=%d.", errCode);
        return errCode;
    }
    if (errCode != -E_NOT_FOUND) {
        --position_;
    }
    return errCode;
}

int RdSingleVerResultSet::MoveTo(int position) const
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    int errCode = PreCheckResultSet();
    if (errCode != E_OK) {
        return errCode;
    }
    if (position < 0) {
        LOGW("[SqlSinResSet][MoveTo] Target Position=%d invalid.", position);
    }
    errCode = handle_->MoveTo(position, resultSet_, position_);
    if (errCode != E_OK) {
        LOGE("[SqlSinResSet][MoveTo] fail to move to, %d", errCode);
    }
    return errCode;
}

int RdSingleVerResultSet::GetEntry(Entry &entry) const
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    int errCode = PreCheckResultSet();
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = handle_->GetEntry(resultSet_, entry);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[SqlSinResSet][GetEntry] failed to get entry form result set.");
    }
    return errCode == -E_NOT_FOUND ? -E_NO_SUCH_ENTRY : errCode;
}

} // namespace DistributedDB
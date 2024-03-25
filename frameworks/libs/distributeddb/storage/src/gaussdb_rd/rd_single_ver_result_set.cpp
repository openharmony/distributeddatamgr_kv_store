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
    : key_(key), kvDB_(kvDB)
{
    kvScanMode_ = KV_SCAN_PREFIX;
}

RdSingleVerResultSet::RdSingleVerResultSet(RdSingleVerNaturalStore *kvDB, const Key &beginKey,
    const Key &endKey, GRD_KvScanModeE kvScanMode)
    : beginKey_(beginKey), endKey_(endKey), kvScanMode_(kvScanMode), kvDB_(kvDB) {}

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
    if (kvScanMode_ >= KV_SCAN_BUTT) {
        LOGE("[RdSinResSet] Open result scan mode is invalid.");
        return -E_INVALID_ARGS;
    }
    int errCode = E_OK;
    handle_ = kvDB_->GetHandle(false, errCode);
    if (handle_ == nullptr) {
        LOGE("[RdSinResSet] Get handle failed, errCode=%d.", errCode);
        return errCode;
    }
    switch (kvScanMode_) {
        case KV_SCAN_PREFIX: {
            errCode = handle_->OpenResultSet(key_, kvScanMode_, &resultSet_);
            break;
        }
        case KV_SCAN_RANGE: {
            errCode = handle_->OpenResultSet(beginKey_, endKey_, &resultSet_);
            break;
        }
        default:
            return -E_INVALID_ARGS;
    }
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
    switch (kvScanMode_) {
        case KV_SCAN_PREFIX: {
            errCode = handle_->GetCount(key_, count, kvScanMode_);
            break;
        }
        case KV_SCAN_RANGE: {
            errCode = handle_->GetCount(beginKey_, endKey_, count, kvScanMode_);
            break;
        }
        default:
            return -E_INVALID_ARGS;
    }
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

int RdSingleVerResultSet::Move(int offset) const
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    int errCode = E_OK;
    if (offset == 0) {
        return errCode;
    }
    offset = offset > INT_MAX ? INT_MAX : offset;
    offset = offset < INT_MIN ? INT_MIN : offset;

    while (offset > 0) {
        errCode = MoveToNext();
        if (errCode != E_OK && errCode != -E_NOT_FOUND) {
            LOGE("[RdSinResSet] move by offset failed, errCode=%d, offset=%d", errCode, offset);
            return errCode;
        }
        --offset;
        if (errCode == -E_NOT_FOUND && offset >= 0) {
            LOGE("[RdSingleVerStorageExecutor] move offset: %d, out of bounds or result set empty, position: %d",
                offset, position_);
            return -E_INVALID_ARGS;
        }
    }
    while (offset < 0) {
        errCode = MoveToPrev();
        if (errCode != E_OK && errCode != -E_NOT_FOUND) {
            LOGE("[RdSinResSet] move by offset failed, errCode=%d, offset=%d", errCode, offset);
            return errCode;
        }
        ++offset;
        if (errCode == -E_NOT_FOUND && offset <= 0) {
            LOGE("[RdSingleVerStorageExecutor] move offset: %d, out of bounds or result set empty, position: %d",
                offset, position_);
            return -E_INVALID_ARGS;
        }
    }
    return errCode;
}

int RdSingleVerResultSet::MoveToNext() const
{
    int errCode = PreCheckResultSet();
    if (errCode != E_OK) {
        LOGE("[RdSinResSet] PreCheckResultSet failed");
        return errCode;
    }

    if (position_ == INIT_POSITION && isMovedBefore_) {
        ++position_;
        return E_OK;
    }
    errCode = handle_->MoveToNext(resultSet_);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[RdSinResSet] move next failed, errCode=%d.", errCode);
        return errCode;
    } else if (errCode == -E_NOT_FOUND) {
        if (!isMovedBefore_) {
            LOGE("[RdSinResSet] move next failed, result set is empty");
            return -E_RESULT_SET_EMPTY;
        }
        // nothing to do when position_ equal with endposition_ which isn't equal with INIT_POSITION
        if (endPosition_ == INIT_POSITION || position_ != endPosition_) {
            ++position_;
            endPosition_ = position_;
        }
        return errCode;
    } else {
        // E_OK
        ++position_;
        isMovedBefore_ = true;
        if (position_ == endPosition_ && endPosition_ != INIT_POSITION) {
            // incase we have chosen an end, but new data put in after that
            endPosition_ = position_ + 1;
        }
    }
    return errCode;
}

int RdSingleVerResultSet::MoveToPrev() const
{
    int errCode = PreCheckResultSet();
    if (errCode != E_OK) {
        LOGE("[RdSinResSet] PreCheckResultSet failed");
        return errCode;
    }

    if (position_ == endPosition_ && endPosition_ != INIT_POSITION) {
        --position_;
        return E_OK;
    }

    errCode = handle_->MoveToPrev(resultSet_);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[RdSinResSet] move prev failed, errCode=%d.", errCode);
        return errCode;
    }

    if (errCode == -E_NOT_FOUND) {
        position_ = INIT_POSITION;
    } else {
        if (position_ <= 0) {
            position_ = 0;
        } else {
            --position_;
        }
    }
    return errCode;
}

int RdSingleVerResultSet::MoveTo(int position) const
{
    int errCode = PreCheckResultSet();
    if (errCode != E_OK) {
        return errCode;
    }

    if (position < 0) {
        LOGW("[RdSinResSet][MoveTo] Target Position=%d invalid.", position);
    }

    return Move(position - position_);
}

int RdSingleVerResultSet::MoveToFirst()
{
    if (!isMovedBefore_) {
        return MoveToNext();
    }
    if (position_ == INIT_POSITION) {
        ++position_;
        return E_OK;
    }

    int errCode = E_OK;
    do {
        errCode = MoveToPrev();
        if (errCode != E_OK && errCode != -E_NOT_FOUND) {
            LOGE("[RdSinResSet] move to first failed, errCode=%d, position=%d", errCode, position_);
            return errCode;
        }
        if (errCode == -E_NOT_FOUND) {
            ++position_;
            break;
        }
    } while (errCode == E_OK);

    return E_OK;
}

int RdSingleVerResultSet::MoveToLast()
{
    int errCode = E_OK;
    do {
        errCode = MoveToNext();
        if (errCode != E_OK && errCode != -E_NOT_FOUND) {
            LOGE("[RdSinResSet] move to last failed, errCode=%d, position=%d", errCode, position_);
            return errCode;
        }
        if (errCode == -E_NOT_FOUND) {
            --position_;
            break;
        }
    } while (errCode == E_OK);

    return E_OK;
}

bool RdSingleVerResultSet::IsFirst() const
{
    int position = GetPosition();
    if (GetCount() == 0) {
        return false;
    }
    if (position == 0) {
        return true;
    }
    return false;
}

bool RdSingleVerResultSet::IsLast() const
{
    int position = GetPosition();
    int count = GetCount();
    if (count == 0) {
        return false;
    }
    if (position == (count - 1)) {
        return true;
    }
    return false;
}

bool RdSingleVerResultSet::IsBeforeFirst() const
{
    int position = GetPosition();

    if (GetCount() == 0) {
        return true;
    }
    if (position <= INIT_POSITION) {
        return true;
    }
    return false;
}

bool RdSingleVerResultSet::IsAfterLast() const
{
    int position = GetPosition();
    int count = GetCount();
    if (count == 0) {
        return true;
    }
    if (position >= count) {
        return true;
    }
    return false;
}

int RdSingleVerResultSet::GetEntry(Entry &entry) const
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    int errCode = PreCheckResultSet();
    if (errCode != E_OK) {
        return errCode;
    }

    if (position_ == INIT_POSITION || (position_ == endPosition_ && endPosition_ != INIT_POSITION)) {
        return -E_NO_SUCH_ENTRY;
    }
    errCode = handle_->GetEntry(resultSet_, entry);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[RdSinResSet][GetEntry] failed to get entry form result set.");
    }
    return errCode == -E_NOT_FOUND ? -E_NO_SUCH_ENTRY : errCode;
}
} // namespace DistributedDB
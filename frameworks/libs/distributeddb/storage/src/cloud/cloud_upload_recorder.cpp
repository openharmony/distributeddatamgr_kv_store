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

#include "cloud/cloud_upload_recorder.h"

namespace DistributedDB {
void CloudUploadRecorder::RecordUploadRecord(const std::string &table, const Bytes &hashKey, const CloudWaterType &type,
    int64_t modifyTime)
{
    std::lock_guard<std::mutex> autoLock(recordMutex_);
    uploadRecord_[currentUser_][table][type][hashKey] = modifyTime;
}

bool CloudUploadRecorder::IsIgnoreUploadRecord(const std::string &table, const Bytes &hashKey,
    const CloudWaterType &type, int64_t modifyTime) const
{
    std::lock_guard<std::mutex> autoLock(recordMutex_);
    auto userRecord = uploadRecord_.find(currentUser_);
    if (userRecord == uploadRecord_.end()) {
        return false;
    }
    auto tableRecord = userRecord->second.find(table);
    if (tableRecord == userRecord->second.end()) {
        return false;
    }
    auto typeRecord = tableRecord->second.find(type);
    if (typeRecord == tableRecord->second.end()) {
        return false;
    }
    auto it = typeRecord->second.find(hashKey);
    return it != typeRecord->second.end() && it->second == modifyTime;
}

void CloudUploadRecorder::ReleaseUploadRecord(const std::string &table, const CloudWaterType &type,
    Timestamp localWaterMark)
{
    std::lock_guard<std::mutex> autoLock(recordMutex_);
    auto &records = uploadRecord_[currentUser_][table][type];
    for (auto it = records.begin(); it != records.end();) {
        if (it->second <= static_cast<int64_t>(localWaterMark)) {
            it = records.erase(it);
        } else {
            it++;
        }
    }
}

void CloudUploadRecorder::SetUser(const std::string &user)
{
    std::lock_guard<std::mutex> autoLock(recordMutex_);
    currentUser_ = user;
}
}
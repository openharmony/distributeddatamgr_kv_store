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

#include "process_recorder.h"

namespace DistributedDB {
void ProcessRecorder::ClearRecord()
{
    std::lock_guard<std::mutex> autoLock(recordMutex_);
    downloadRecord_.clear();
    uploadRecord_.clear();
}

bool ProcessRecorder::IsDownloadFinish(int userIndex, const std::string &table) const
{
    return IsRecordFinish(userIndex, table, downloadRecord_);
}

void ProcessRecorder::MarkDownloadFinish(int userIndex, const std::string &table, bool finish)
{
    RecordFinish(userIndex, table, finish, downloadRecord_);
}

bool ProcessRecorder::IsUploadFinish(int userIndex, const std::string &table) const
{
    return IsRecordFinish(userIndex, table, uploadRecord_);
}

void ProcessRecorder::MarkUploadFinish(int userIndex, const std::string &table, bool finish)
{
    RecordFinish(userIndex, table, finish, uploadRecord_);
}

bool ProcessRecorder::IsRecordFinish(int userIndex, const std::string &table,
    const std::map<int, std::map<std::string, bool>> &record) const
{
    std::lock_guard<std::mutex> autoLock(recordMutex_);
    if (record.find(userIndex) == record.end()) {
        return false;
    }
    if (record.at(userIndex).find(table) == record.at(userIndex).end()) {
        return false;
    }
    return record.at(userIndex).at(table);
}

void ProcessRecorder::RecordFinish(int userIndex, const std::string &table, bool finish,
    std::map<int, std::map<std::string, bool>> &record) const
{
    std::lock_guard<std::mutex> autoLock(recordMutex_);
    record[userIndex][table] = finish;
}
}
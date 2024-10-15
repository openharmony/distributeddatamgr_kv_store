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

#ifndef PROCESS_RECORDER_H
#define PROCESS_RECORDER_H

#include <mutex>
#include "store_types.h"

namespace DistributedDB {
class ProcessRecorder {
public:
    ProcessRecorder() = default;
    ~ProcessRecorder() = default;

    void ClearRecord();

    bool IsDownloadFinish(int userIndex, const std::string &table) const;
    void MarkDownloadFinish(int userIndex, const std::string &table, bool finish);

    bool IsUploadFinish(int userIndex, const std::string &table) const;
    void MarkUploadFinish(int userIndex, const std::string &table, bool finish);
private:
    bool IsRecordFinish(int userIndex, const std::string &table,
        const std::map<int, std::map<std::string, bool>> &record) const;
    void RecordFinish(int userIndex, const std::string &table, bool finish,
        std::map<int, std::map<std::string, bool>> &record);

    mutable std::mutex recordMutex_;
    std::map<int, std::map<std::string, bool>> downloadRecord_; // record table download finish
    std::map<int, std::map<std::string, bool>> uploadRecord_;   // record table upload finish
};
}

#endif // PROCESS_RECORDER_H
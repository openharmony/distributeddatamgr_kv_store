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

#ifndef CLOUD_UPLOAD_RECORDER_H
#define CLOUD_UPLOAD_RECORDER_H

#include <map>
#include <mutex>

#include "cloud/cloud_db_types.h"
#include "db_types.h"

namespace DistributedDB {
class CloudUploadRecorder {
public:
    void RecordUploadRecord(const std::string &table, const Bytes &hashKey, const CloudWaterType &type,
        int64_t modifyTime);
    bool IsIgnoreUploadRecord(const std::string &table, const Bytes &hashKey, const CloudWaterType &type,
        int64_t modifyTime) const;
    void ReleaseUploadRecord(const std::string &table, const CloudWaterType &type, Timestamp localWaterMark);
    void SetUser(const std::string &user);

    CloudUploadRecorder() = default;
    ~CloudUploadRecorder() = default;
private:
    mutable std::mutex recordMutex_;
    using TableUploadRecord = std::map<CloudWaterType, std::map<Bytes, int64_t>>;
    using UserRecord = std::map<std::string, TableUploadRecord>;
    std::map<std::string, UserRecord> uploadRecord_;
    std::string currentUser_;
};
}

#endif // CLOUD_UPLOAD_RECORDER_H
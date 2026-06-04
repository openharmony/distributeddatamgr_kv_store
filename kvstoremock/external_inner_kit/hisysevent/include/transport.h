/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef HISYSEVENT_TRANSPORT_H
#define HISYSEVENT_TRANSPORT_H

#include <list>
#include <mutex>
#include <string>

#include "raw_data.h"

namespace OHOS {
namespace HiviewDFX {
using namespace Encoded;
class Transport {
public:
    static Transport& GetInstance();
    int SendData(RawData& rawData);

private:
    Transport() {}
    ~Transport() {}
    Transport& operator=(const Transport&) = delete;
    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&&) = delete;
    Transport(const Transport&&) = delete;

private:
    void AddFailedData(RawData& rawData);
    void InitRecvBuffer(int socketId);
    void RetrySendFailedData();
    int SendToHiSysEventDataSource(RawData& rawData);

private:
    static Transport instance_;
    static constexpr std::size_t RETRY_QUEUE_SIZE = 10;
    static constexpr int RETRY_TIMES = 3;
    std::mutex mutex_;
    std::list<RawData> retryDataList_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HISYSEVENT_TRANSPORT_H

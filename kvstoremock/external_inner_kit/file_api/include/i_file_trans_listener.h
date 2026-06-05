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

#ifndef OHOS_STORAGE_I_FILE_TRANS_LISTENER_H
#define OHOS_STORAGE_I_FILE_TRANS_LISTENER_H
#include <cstdint>

#include "iremote_broker.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
class IFileTransListener : public IRemoteBroker {
public:
    enum {
        File_Trans_Listener_SUCCESS = 0,
        File_Trans_Listener_DESCRIPTOR_IS_EMPTY,
    };
    virtual int32_t OnFileReceive(uint64_t totalBytes, uint64_t processedBytes) = 0;
    virtual int32_t OnFailed(const std::string &sessionName, int32_t errorCode) = 0;
    virtual int32_t OnFinished(const std::string &sessionName) = 0;
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.storage.distributedfile.translistener")
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS

#endif // OHOS_STORAGE_I_FILE_TRANS_LISTENER_H

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

#ifndef OHOS_STORAGE_I_ASSET_SEND_CALLBACK_H
#define OHOS_STORAGE_I_ASSET_SEND_CALLBACK_H

#include <cstdint>
#include <string>
#include "asset/asset_obj.h"
#include "iremote_broker.h"
#include "refbase.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
class IAssetSendCallback : public IRemoteBroker {
public:
    enum {
        ASSET_SEND_CALLBACK_SUCCESS = 0,
        ASSET_SEND_CALLBACK_DESCRIPTOR_IS_EMPTY,
    };
    virtual int32_t OnSendResult(const sptr<AssetObj> &assetObj, int32_t result) = 0;
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.storage.distributedfile.assetsendcallback")
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif // OHOS_STORAGE_I_ASSET_SEND_CALLBACK_H

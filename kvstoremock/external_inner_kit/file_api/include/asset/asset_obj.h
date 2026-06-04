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

#ifndef OHOS_STORAGE_ASSET_OBJ_H
#define OHOS_STORAGE_ASSET_OBJ_H

#include <string>
#include <vector>
#include "parcel.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
struct AssetObj : public Parcelable {
public:
    std::string sessionId_;
    std::string dstNetworkId_;
    std::string srcBundleName_;
    std::string dstBundleName_;
    std::vector<std::string> uris_;
    bool ReadFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static AssetObj *Unmarshalling(Parcel &parcel);

    AssetObj() = default;
    AssetObj(const AssetObj &assetObj)
        : sessionId_(assetObj.sessionId_),
          dstNetworkId_(assetObj.dstNetworkId_),
          srcBundleName_(assetObj.srcBundleName_),
          dstBundleName_(assetObj.dstBundleName_),
          uris_(assetObj.uris_)
    {
    }
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif // OHOS_STORAGE_ASSET_OBJ_H

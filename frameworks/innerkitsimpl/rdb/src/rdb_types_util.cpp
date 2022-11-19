/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "rdb_types_util.h"
namespace OHOS::ITypesUtil {
template<>
bool Marshalling(const SyncerParam &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, input.bundleName_, input.hapName_, input.storeName_, input.area_,
        input.level_, input.type_, input.isAutoSync_, input.isEncrypt_, input.password_);
}
template<>
bool Unmarshalling(SyncerParam &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.bundleName_, output.hapName_, output.storeName_, output.area_,
        output.level_, output.type_, output.isAutoSync_, output.isEncrypt_, output.password_);
}

template<>
bool Marshalling(const SyncOption &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, static_cast<int32_t>(input.mode), input.isBlock);
}

template<>
bool Unmarshalling(SyncOption &output, MessageParcel &data)
{
    int32_t mode = static_cast<int32_t>(output.mode);
    auto ret = ITypesUtil::Unmarshal(data, mode, output.isBlock);
    output.mode = static_cast<decltype(output.mode)>(mode);
    return ret;
}

template<>
bool Marshalling(const RdbPredicates &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, input.table_, input.devices_, input.operations_);
}
template<>
bool Unmarshalling(RdbPredicates &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.table_, output.devices_, output.operations_);
}

template<>
bool Marshalling(const RdbOperation &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, static_cast<int32_t>(input.operator_), input.field_, input.values_);
}

template<>
bool Unmarshalling(RdbOperation &output, MessageParcel &data)
{
    int32_t option;
    auto ret = ITypesUtil::Unmarshal(data, option, output.field_, output.values_);
    output.operator_ = static_cast<decltype(output.operator_)>(option);
    return ret;
}
}
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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_KV_TYPES_UTIL_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_KV_TYPES_UTIL_H
#include "change_notification.h"
#include "kvdb_service.h"
#include "itypes_util.h"
#include "types.h"
namespace OHOS::ITypesUtil {
using Blob = DistributedKv::Blob;
using Key = DistributedKv::Key;
using Value = DistributedKv::Value;
using Entry = DistributedKv::Entry;
using AppId = DistributedKv::AppId;
using StoreId = DistributedKv::StoreId;
using DeviceInfo = DistributedKv::DeviceInfo;
using ChangeNotification = DistributedKv::ChangeNotification;
using Options = DistributedKv::Options;
using SyncPolicy = DistributedKv::SyncPolicy;
using DevBrief = DistributedKv::KVDBService::DevBrief;
template<>
API_EXPORT bool Marshalling(const Blob &input, MessageParcel &data);
template<>
API_EXPORT bool Unmarshalling(Blob &output, MessageParcel &data);

template<>
API_EXPORT bool Marshalling(const AppId &input, MessageParcel &data);
template<>
API_EXPORT bool Unmarshalling(AppId &output, MessageParcel &data);

template<>
API_EXPORT bool Marshalling(const StoreId &input, MessageParcel &data);
template<>
API_EXPORT bool Unmarshalling(StoreId &output, MessageParcel &data);

template<>
API_EXPORT bool Marshalling(const Entry &input, MessageParcel &data);
template<>
API_EXPORT bool Unmarshalling(Entry &output, MessageParcel &data);

template<>
API_EXPORT bool Marshalling(const DeviceInfo &input, MessageParcel &data);
template<>
API_EXPORT bool Unmarshalling(DeviceInfo &output, MessageParcel &data);

template<>
API_EXPORT bool Marshalling(const ChangeNotification &notification, MessageParcel &parcel);
template<>
API_EXPORT bool Unmarshalling(ChangeNotification &output, MessageParcel &parcel);

template<>
API_EXPORT bool Marshalling(const Options &input, MessageParcel &data);
template<>
API_EXPORT bool Unmarshalling(Options &output, MessageParcel &data);

template<>
API_EXPORT bool Marshalling(const SyncPolicy &input, MessageParcel &data);
template<>
API_EXPORT bool Unmarshalling(SyncPolicy &output, MessageParcel &data);

template<>
API_EXPORT bool Marshalling(const DevBrief &input, MessageParcel &data);
template<>
API_EXPORT bool Unmarshalling(DevBrief &output, MessageParcel &data);

int64_t GetTotalSize(const std::vector<Entry> &entries);
int64_t GetTotalSize(const std::vector<Key> &entries);
} // namespace OHOS::ITypesUtil
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_KV_TYPES_UTIL_H

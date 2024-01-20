/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef RUNTIME_CONFIG_H
#define RUNTIME_CONFIG_H

#include <memory>
#include <mutex>

#include "auto_launch_export.h"
#include "db_info_handle.h"
#include "cloud/icloud_data_translate.h"
#include "iprocess_communicator.h"
#include "iprocess_system_api_adapter.h"
#include "ithread_pool.h"
#include "store_types.h"
namespace DistributedDB {
enum class DBType {
    DB_KV,
    DB_RELATION,
};

class RuntimeConfig final {
public:
    DB_API RuntimeConfig() = default;
    DB_API ~RuntimeConfig() = default;

    DB_API static DBStatus SetProcessLabel(const std::string &appId, const std::string &userId);

    DB_API static DBStatus SetProcessCommunicator(const std::shared_ptr<IProcessCommunicator> &inCommunicator);

    DB_API static DBStatus SetPermissionCheckCallback(const PermissionCheckCallbackV2 &callback);

    DB_API static DBStatus SetPermissionCheckCallback(const PermissionCheckCallbackV3 &callback);

    DB_API static DBStatus SetProcessSystemAPIAdapter(const std::shared_ptr<IProcessSystemApiAdapter> &adapter);

    DB_API static void Dump(int fd, const std::vector<std::u16string> &args);

    DB_API static DBStatus SetSyncActivationCheckCallback(const SyncActivationCheckCallback &callback);

    DB_API static DBStatus NotifyUserChanged();

    DB_API static DBStatus SetSyncActivationCheckCallback(const SyncActivationCheckCallbackV2 &callback);

    DB_API static DBStatus SetPermissionConditionCallback(const PermissionConditionCallback &callback);

    DB_API static bool IsProcessSystemApiAdapterValid();

    DB_API static void SetDBInfoHandle(const std::shared_ptr<DBInfoHandle> &handle);

    DB_API static void NotifyDBInfos(const DeviceInfos &devInfos, const std::vector<DBInfo> &dbInfos);

    DB_API static void SetTranslateToDeviceIdCallback(const TranslateToDeviceIdCallback &callback);

    DB_API static void SetAutoLaunchRequestCallback(const AutoLaunchRequestCallback &callback, DBType type);

    DB_API static std::string GetStoreIdentifier(const std::string &userId, const std::string &appId,
        const std::string &storeId, bool syncDualTupleMode = false);

    DB_API static void ReleaseAutoLaunch(const std::string &userId, const std::string &appId,
        const std::string &storeId, DBType type);

    DB_API static void SetThreadPool(const std::shared_ptr<IThreadPool> &threadPool);

    DB_API static void SetCloudTranslate(const std::shared_ptr<ICloudDataTranslate> &dataTranslate);
private:
    static std::mutex communicatorMutex_;
    static std::mutex multiUserMutex_;
    static std::shared_ptr<IProcessCommunicator> processCommunicator_;
};
} // namespace DistributedDB

#endif // RUNTIME_CONFIG_H
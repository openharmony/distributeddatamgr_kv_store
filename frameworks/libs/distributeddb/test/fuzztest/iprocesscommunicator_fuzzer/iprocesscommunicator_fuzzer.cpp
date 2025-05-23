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

#include "iprocesscommunicator_fuzzer.h"
#include <list>
#include "distributeddb_tools_test.h"
#include "db_info_handle.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "iprocess_communicator.h"
#include "iprocess_system_api_adapter.h"
#include "runtime_config.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

class IProcessCommunicatorFuzzer {
    /* Keep C++ file names the same as the class name */
};

namespace OHOS {
class ProcessCommunicatorFuzzTest : public DistributedDB::IProcessCommunicator {
public:
    ProcessCommunicatorFuzzTest() {}
    ~ProcessCommunicatorFuzzTest() {}
    DistributedDB::DBStatus Start(const std::string &processLabel) override
    {
        return DistributedDB::OK;
    }
    // The Stop should only be called after Start successfully
    DistributedDB::DBStatus Stop() override
    {
        return DistributedDB::OK;
    }
    DistributedDB::DBStatus RegOnDeviceChange(const DistributedDB::OnDeviceChange &callback) override
    {
        onDeviceChange_ = callback;
        return DistributedDB::OK;
    }
    DistributedDB::DBStatus RegOnDataReceive(const DistributedDB::OnDataReceive &callback) override
    {
        onDataReceive_ = callback;
        return DistributedDB::OK;
    }
    DistributedDB::DBStatus SendData(const DistributedDB::DeviceInfos &dstDevInfo,
        const uint8_t *datas, uint32_t length) override
    {
        return DistributedDB::OK;
    }
    uint32_t GetMtuSize() override
    {
        return 1 * 1024  * 1024; // 1 * 1024 * 1024 Byte.
    }
    DistributedDB::DeviceInfos GetLocalDeviceInfos() override
    {
        DistributedDB::DeviceInfos info;
        info.identifier = "default";
        return info;
    }

    std::vector<DistributedDB::DeviceInfos> GetRemoteOnlineDeviceInfosList() override
    {
        std::vector<DistributedDB::DeviceInfos> info;
        return info;
    }

    bool IsSameProcessLabelStartedOnPeerDevice(const DistributedDB::DeviceInfos &peerDevInfo) override
    {
        return true;
    }

    void FuzzOnDeviceChange(const DistributedDB::DeviceInfos &devInfo, bool isOnline)
    {
        if (onDeviceChange_ == nullptr) {
            return;
        }
        onDeviceChange_(devInfo, isOnline);
    }

    void FuzzOnDataReceive(const  DistributedDB::DeviceInfos &devInfo, FuzzedDataProvider &provider)
    {
        if (onDataReceive_ == nullptr) {
            return;
        }
        size_t dataLen = 1024;
        uint32_t size = provider.ConsumeIntegralInRange<size_t>(0, dataLen);;
        uint8_t* data = static_cast<uint8_t*>(new uint8_t[size]);
        provider.ConsumeData(data, size);
        onDataReceive_(devInfo, data, size);
        delete[] static_cast<uint8_t*>(data);
        data = nullptr;
    }

private:
    DistributedDB::OnDeviceChange onDeviceChange_ = nullptr;
    DistributedDB::OnDataReceive onDataReceive_ = nullptr;
};

class ProcessSystemApiAdapterFuzzTest : public DistributedDB::IProcessSystemApiAdapter {
public:
    ~ProcessSystemApiAdapterFuzzTest() {}
    DistributedDB::DBStatus RegOnAccessControlledEvent(const OnAccessControlledEvent &callback) override
    {
        callback_ = callback;
        return DistributedDB::OK;
    }

    bool IsAccessControlled() const override
    {
        return true;
    }

    DistributedDB::DBStatus SetSecurityOption(const std::string &filePath, const SecurityOption &option) override
    {
        return DistributedDB::OK;
    }

    DistributedDB::DBStatus GetSecurityOption(const std::string &filePath, SecurityOption &option) const override
    {
        return DistributedDB::OK;
    }

    bool CheckDeviceSecurityAbility(const std::string &devId, const SecurityOption &option) const override
    {
        return true;
    }
private:
    DistributedDB::OnAccessControlledEvent callback_;
};

class DBInfoHandleFuzzTest : public DistributedDB::DBInfoHandle {
public:
    ~DBInfoHandleFuzzTest() {}
    bool IsSupport() override
    {
        return true;
    }

    bool IsNeedAutoSync(const std::string &userId, const std::string &appId, const std::string &storeId,
        const DistributedDB::DeviceInfos &devInfo) override
    {
        return true;
    }
};

void CommunicatorFuzzer(FuzzedDataProvider &provider)
{
    static auto kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
    std::string rawString = provider.ConsumeRandomLengthString();
    KvStoreDelegateManager::SetProcessLabel(rawString, "defaut");
    auto communicator = std::make_shared<ProcessCommunicatorFuzzTest>();
    KvStoreDelegateManager::SetProcessCommunicator(communicator);
    RuntimeConfig::SetProcessCommunicator(communicator);
    auto adapter = std::make_shared<ProcessSystemApiAdapterFuzzTest>();
    RuntimeConfig::SetProcessSystemAPIAdapter(adapter);
    auto handleTest = std::make_shared<DBInfoHandleFuzzTest>();
    RuntimeConfig::SetDBInfoHandle(handleTest);
    std::string testDir;
    DistributedDBToolsTest::TestDirInit(testDir);
    KvStoreConfig config;
    config.dataDir = testDir;
    kvManager.SetKvStoreConfig(config);
    KvStoreNbDelegate::Option option = {true, false, false};
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;
    kvManager.GetKvStore(rawString, option,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    DeviceInfos device = {"defaut"};
    communicator->FuzzOnDataReceive(device, provider);
    if (kvNbDelegatePtr != nullptr) {
        kvManager.CloseKvStore(kvNbDelegatePtr);
        kvManager.DeleteKvStore(rawString);
    }
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // 4 bytes is required
    if (size < 4) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    OHOS::CommunicatorFuzzer(fdp);
    return 0;
}

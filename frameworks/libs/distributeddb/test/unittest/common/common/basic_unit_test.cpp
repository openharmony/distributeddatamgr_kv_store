/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "basic_unit_test.h"
#include "platform_specific.h"

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDBUnitTest;
std::string g_testDir;
void BasicUnitTest::SetUpTestCase()
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("[BasicUnitTest] Rm test db files error!");
    }
}

void BasicUnitTest::TearDownTestCase()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("[BasicUnitTest] Rm test db files error!");
    }
}

void BasicUnitTest::SetUp()
{
    if (OS::CheckPathExistence(g_testDir)) {  // if test dir exist, delete it and printf log
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
        LOGE("[BasicUnitTest] The previous test case not clean db files.");
    }
    RuntimeContext::GetInstance()->ClearAllDeviceTimeInfo();
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    ASSERT_EQ(communicatorAggregator_, nullptr);
    communicatorAggregator_ = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicatorAggregator_ != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicatorAggregator_);
}

void BasicUnitTest::TearDown()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("[BasicUnitTest] Rm test db files error.");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAdapter(nullptr);
    communicatorAggregator_ = nullptr;
    RuntimeContext::GetInstance()->ClearAllDeviceTimeInfo();
    RuntimeContext::GetInstance()->SetTimeChanged(false);
}

int BasicUnitTest::InitDelegate(const StoreInfo &info, const std::string &deviceId)
{
    if (communicatorAggregator_ == nullptr) {
        LOGE("[BasicUnitTest] Init delegate with null aggregator");
        return -E_INTERNAL_ERROR;
    }
    communicatorAggregator_->SetRemoteDeviceId(deviceId);
    int errCode = InitDelegate(info);
    if (errCode != E_OK) {
        LOGE("[BasicUnitTest] Init delegate failed %d", errCode);
    } else {
        SetDevice(info, deviceId);
    }
    return errCode;
}

std::string BasicUnitTest::GetTestDir()
{
    return g_testDir;
}

std::string BasicUnitTest::GetDevice(const StoreInfo &info) const
{
    std::lock_guard<std::mutex> autoLock(deviceMutex_);
    auto iter = deviceMap_.find(info);
    if (iter == deviceMap_.end()) {
        LOGW("[BasicUnitTest] Not exist device app %s store %s user %s",
            info.appId.c_str(),
            info.storeId.c_str(),
            info.userId.c_str());
        return "";
    }
    return iter->second;
}

void BasicUnitTest::SetDevice(const StoreInfo &info, const std::string &device)
{
    std::lock_guard<std::mutex> autoLock(deviceMutex_);
    deviceMap_[info] = device;
    LOGW("[BasicUnitTest] Set device app %s store %s user %s device %s",
        info.appId.c_str(),
        info.storeId.c_str(),
        info.userId.c_str(),
        device.c_str());
}

StoreInfo BasicUnitTest::GetStoreInfo1()
{
    StoreInfo info;
    info.userId = DistributedDBUnitTest::USER_ID;
    info.storeId = DistributedDBUnitTest::STORE_ID_1;
    info.appId = DistributedDBUnitTest::APP_ID;
    return info;
}

StoreInfo BasicUnitTest::GetStoreInfo2()
{
    StoreInfo info;
    info.userId = DistributedDBUnitTest::USER_ID;
    info.storeId = DistributedDBUnitTest::STORE_ID_2;
    info.appId = DistributedDBUnitTest::APP_ID;
    return info;
}

StoreInfo BasicUnitTest::GetStoreInfo3()
{
    StoreInfo info;
    info.userId = DistributedDBUnitTest::USER_ID;
    info.storeId = DistributedDBUnitTest::STORE_ID_3;
    info.appId = DistributedDBUnitTest::DISTRIBUTED_APP_ID;
    return info;
}

void BasicUnitTest::SetProcessCommunicator(const std::shared_ptr<IProcessCommunicator> &processCommunicator)
{
    processCommunicator_ = processCommunicator;
}

uint64_t BasicUnitTest::GetAllSendMsgSize() const
{
    if (communicatorAggregator_ == nullptr) {
        return 0u;
    }
    return communicatorAggregator_->GetAllSendMsgSize();
}

void BasicUnitTest::RegBeforeDispatch(const std::function<void(const std::string &, const Message *)> &beforeDispatch)
{
    ASSERT_NE(communicatorAggregator_, nullptr);
    communicatorAggregator_->RegBeforeDispatch(beforeDispatch);
}

void BasicUnitTest::SetMtu(const std::string &dev, uint32_t mtu)
{
    ASSERT_NE(communicatorAggregator_, nullptr);
    communicatorAggregator_->SetDeviceMtuSize(dev, mtu);
}
}  // namespace DistributedDB

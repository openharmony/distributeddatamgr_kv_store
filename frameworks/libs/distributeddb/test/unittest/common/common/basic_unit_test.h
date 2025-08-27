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

#ifndef BASIC_UNIT_TEST_H
#define BASIC_UNIT_TEST_H
#include <gtest/gtest.h>

#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "virtual_communicator_aggregator.h"

namespace DistributedDB {

class BasicUnitTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
protected:
    int InitDelegate(const StoreInfo &info, const std::string &deviceId);
    virtual int InitDelegate(const StoreInfo &info) = 0;
    virtual int CloseDelegate(const StoreInfo &info) = 0;
    virtual void CloseAllDelegate() = 0;
    std::string GetDevice(const StoreInfo &info) const;
    void SetProcessCommunicator(const std::shared_ptr<IProcessCommunicator> &processCommunicator);
    uint64_t GetAllSendMsgSize() const;
    void RegBeforeDispatch(const std::function<void(const std::string &, const Message *)> &beforeDispatch);
    static StoreInfo GetStoreInfo1();
    static StoreInfo GetStoreInfo2();
    static StoreInfo GetStoreInfo3();
    void SetDevice(const StoreInfo &info, const std::string &device);
    void SetMtu(const std::string &dev, uint32_t mtu);
    static std::string GetTestDir();
    VirtualCommunicatorAggregator *communicatorAggregator_ = nullptr;
    std::shared_ptr<IProcessCommunicator> processCommunicator_ = nullptr;
    mutable std::mutex deviceMutex_;
    std::map<StoreInfo, std::string> deviceMap_;
};
}
#endif // BASIC_UNIT_TEST_H

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

#include <gtest/gtest.h>

#include "cloud_store_types.h"
#include "log_table_manager_factory.h"
#include "native_sqlite.h"
#include "split_device_log_table_manager.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace std;

class DistributedDBInterfacesLogTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
protected:
};

void DistributedDBInterfacesLogTest::SetUpTestCase(void)
{
}

void DistributedDBInterfacesLogTest::TearDownTestCase(void)
{
}

void DistributedDBInterfacesLogTest::SetUp()
{
}

void DistributedDBInterfacesLogTest::TearDown()
{
}

/**
  * @tc.name: DBFactoryTest001
  * @tc.desc: check table manager with mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesLogTest, DBFactoryTest001, TestSize.Level1)
{
    DistributedTableMode mode = DistributedTableMode::COLLABORATION;
    TableSyncType tableSyncType = TableSyncType::DEVICE_COOPERATION;
    TableInfo tableInfo;
    auto tableManager = LogTableManagerFactory::GetTableManager(tableInfo, mode, tableSyncType);
    EXPECT_TRUE(tableManager != nullptr);
}

/**
  * @tc.name: DeviceLogTest001
  * @tc.desc: Calc primary key hash
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesLogTest, DeviceLogTest001, TestSize.Level1)
{
    string key1 = "NEW.";
    TableInfo tableInfo;
    tableInfo.SetPrimaryKey("key2", 1);
    tableInfo.SetPrimaryKey("key3", 2);
    string key4 = "hashKey";
    SplitDeviceLogTableManager manager;
    string value = manager.CalcPrimaryKeyHash(key1, tableInfo, key4);
    EXPECT_EQ(value, "calc_hash(calc_hash(NEW.'key2', 0)||calc_hash(NEW.'key3', 0), 0)");
}

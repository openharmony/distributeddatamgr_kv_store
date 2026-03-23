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

#include <cstdint>
#include <gtest/gtest.h>
#include <variant>
#include <vector>

#include "change_notification.h"
#include "iremote_broker.h"
#include "iremote_object.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "itypes_util.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS;
using var_t = std::variant<std::monostate, uint32_t, std::string, int32_t, uint64_t>;
namespace OHOS::Test {
class TypesUtilTest : public testing::Test {
public:
    class ITestRemoteObject : public IRemoteBroker {
    public:
        DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.ITestRemoteObject");
    };
    class TestRemoteObjectStub : public IRemoteStub<ITestRemoteObject> {
    public:
    };
    class TestRemoteObjectProxy : public IRemoteProxy<ITestRemoteObject> {
    public:
        explicit TestRemoteObjectProxy(const sptr<IRemoteObject> &impl)
            : IRemoteProxy<ITestRemoteObject>(impl)
        {}
        ~TestRemoteObjectProxy() = default;
    private:
        static inline BrokerDelegator<TestRemoteObjectProxy> delegator_;
    };
    class TestRemoteObjectClient : public TestRemoteObjectStub {
    public:
        TestRemoteObjectClient() {}
        virtual ~TestRemoteObjectClient() {}
    };
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

HWTEST_F(TypesUtilTest, DeviceInfo, TestSize.Level0)
{
    MessageParcel parcel;
    DeviceInfo clientDev;
    clientDev.deviceId = "123";
    clientDev.deviceName = "rk3568";
    clientDev.deviceType = "phone";
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, clientDev));
    DeviceInfo serverDev;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, serverDev));
    ASSERT_EQ(clientDev.deviceId, serverDev.deviceId);
    ASSERT_EQ(clientDev.deviceName, serverDev.deviceName);
    ASSERT_EQ(clientDev.deviceType, serverDev.deviceType);
}

HWTEST_F(TypesUtilTest, Entry, TestSize.Level0)
{
    MessageParcel parcel;
    Entry entryIn;
    entryIn.key = "student_name_mali";
    entryIn.value = "age:20";
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, entryIn));
    Entry entryOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, entryOut));
    EXPECT_EQ(entryOut.key.ToString(), std::string("student_name_mali"));
    EXPECT_EQ(entryOut.value.ToString(), std::string("age:20"));
}

HWTEST_F(TypesUtilTest, ChangeNotification, TestSize.Level1)
{
    Entry insert, update, del;
    insert.key = "insert";
    update.key = "update";
    del.key = "delete";
    insert.value = "insert_value";
    update.value = "update_value";
    del.value = "delete_value";
    std::vector<Entry> inserts, updates, deleteds;
    inserts.push_back(insert);
    updates.push_back(update);
    deleteds.push_back(del);

    ChangeNotification changeIn(std::move(inserts), std::move(updates), std::move(deleteds), std::string(), false);
    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, changeIn));
    ChangeNotification changeOut({}, {}, {}, "", false);
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, changeOut));
    ASSERT_EQ(changeOut.GetInsertEntries().size(), 1UL);
    EXPECT_EQ(changeOut.GetInsertEntries().front().key.ToString(), std::string("insert"));
    EXPECT_EQ(changeOut.GetInsertEntries().front().value.ToString(), std::string("insert_value"));
    ASSERT_EQ(changeOut.GetUpdateEntries().size(), 1UL);
    EXPECT_EQ(changeOut.GetUpdateEntries().front().key.ToString(), std::string("update"));
    EXPECT_EQ(changeOut.GetUpdateEntries().front().value.ToString(), std::string("update_value"));
    ASSERT_EQ(changeOut.GetDeleteEntries().size(), 1UL);
    EXPECT_EQ(changeOut.GetDeleteEntries().front().key.ToString(), std::string("delete"));
    EXPECT_EQ(changeOut.GetDeleteEntries().front().value.ToString(), std::string("delete_value"));
    EXPECT_EQ(changeOut.IsClear(), false);
}


HWTEST_F(TypesUtilTest, Multiple, TestSize.Level1)
{
    uint32_t input1 = 10;
    int32_t input2 = -10;
    std::string input3 = "i test";
    Blob input4 = "input 4";
    Entry input5;
    input5.key = "my test";
    input5.value = "test value";
    DeviceInfo input6 = {.deviceId = "mock deviceId", .deviceName = "mock phone", .deviceType = "0"};
    sptr<ITestRemoteObject> input7 = new TestRemoteObjectClient();
    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, input1, input2, input3, input4, input5, input6, input7->AsObject()));
    uint32_t output1 = 0;
    int32_t output2 = 0;
    std::string output3 = "";
    Blob output4;
    Entry output5;
    DeviceInfo output6;
    sptr<IRemoteObject> output7;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, output1, output2, output3, output4, output5, output6, output7));
    ASSERT_EQ(output1, input1);
    ASSERT_EQ(output2, input2);
    ASSERT_EQ(output3, input3);
    ASSERT_EQ(output4, input4);
    ASSERT_EQ(output5.key, input5.key);
    ASSERT_EQ(output5.value, input5.value);
    ASSERT_EQ(output6.deviceId, input6.deviceId);
    ASSERT_EQ(output6.deviceName, input6.deviceName);
    ASSERT_EQ(output6.deviceType, input6.deviceType);
    ASSERT_EQ(output7, input7->AsObject());
}

HWTEST_F(TypesUtilTest, Variant, TestSize.Level0)
{
    MessageParcel parcelNull;
    var_t valueNullIn;
    ASSERT_TRUE(ITypesUtil::Marshal(parcelNull, valueNullIn));
    var_t valueNullOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcelNull, valueNullOut));
    ASSERT_EQ(valueNullOut.index(), 0);

    MessageParcel parcelUint;
    var_t valueUintIn;
    valueUintIn.emplace<1>(100);
    ASSERT_TRUE(ITypesUtil::Marshal(parcelUint, valueUintIn));
    var_t valueUintOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcelUint, valueUintOut));
    ASSERT_EQ(valueUintOut.index(), 1);
    ASSERT_EQ(std::get<uint32_t>(valueUintOut), 100);

    MessageParcel parcelString;
    var_t valueStringIn;
    valueStringIn.emplace<2>("valueString");
    ASSERT_TRUE(ITypesUtil::Marshal(parcelString, valueStringIn));
    var_t valueStringOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcelString, valueStringOut));
    ASSERT_EQ(valueStringOut.index(), 2);
    ASSERT_EQ(std::get<std::string>(valueStringOut), "valueString");

    MessageParcel parcelInt;
    var_t valueIntIn;
    valueIntIn.emplace<3>(101);
    ASSERT_TRUE(ITypesUtil::Marshal(parcelInt, valueIntIn));
    var_t valueIntOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcelInt, valueIntOut));
    ASSERT_EQ(valueIntOut.index(), 3);
    ASSERT_EQ(std::get<int32_t>(valueIntOut), 101);

    MessageParcel parcelUint64;
    var_t valueUint64In;
    valueUint64In.emplace<4>(110);
    ASSERT_TRUE(ITypesUtil::Marshal(parcelUint64, valueUint64In));
    var_t valueUint64Out;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcelUint64, valueUint64Out));
    ASSERT_EQ(valueUint64Out.index(), 4);
    ASSERT_EQ(std::get<uint64_t>(valueUint64Out), 110);
}

/**
* @tc.name: MarshalToBufferLimitTest001
* @tc.desc: construct a invalid vector and check MarshalToBuffer function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(TypesUtilTest, MarshalToBufferLimitTest001, TestSize.Level1)
{
    MessageParcel parcel;
    std::vector<Entry> exceedMaxCountInput(ITypesUtil::MAX_COUNT + 1);
    ASSERT_FALSE(ITypesUtil::MarshalToBuffer(exceedMaxCountInput, sizeof(int) * exceedMaxCountInput.size(), parcel));
}

/**
* @tc.name: MarshalToBufferLimitTest002
* @tc.desc: construct a invalid vector and check MarshalToBuffer function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(TypesUtilTest, MarshalToBufferLimitTest002, TestSize.Level1)
{
    MessageParcel parcel;
    std::vector<Entry> inputNormal(10);
    ASSERT_FALSE(ITypesUtil::MarshalToBuffer(inputNormal, ITypesUtil::MAX_SIZE + 1, parcel));
    ASSERT_FALSE(ITypesUtil::MarshalToBuffer(inputNormal, -1, parcel));
}

/**
* @tc.name: UnmarshalFromBufferLimitTest001
* @tc.desc: construct a invalid parcel and check UnmarshalFromBuffer function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(TypesUtilTest, UnmarshalFromBufferLimitTest001, TestSize.Level1)
{
    MessageParcel parcel;
    int32_t normalSize = 100;
    parcel.WriteInt32(normalSize);                //normal size
    parcel.WriteInt32(ITypesUtil::MAX_COUNT + 1); //exceed MAX_COUNT
    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(normalSize);
    parcel.WriteRawData(buffer.get(), normalSize);

    std::vector<Entry> output;
    ASSERT_FALSE(ITypesUtil::UnmarshalFromBuffer(parcel, output));
    ASSERT_TRUE(output.empty());
}

/**
* @tc.name: UnmarshalFromBufferLimitTest002
* @tc.desc: construct a invalid parcel and check UnmarshalFromBuffer function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(TypesUtilTest, UnmarshalFromBufferLimitTest002, TestSize.Level1)
{
    MessageParcel parcel;
    parcel.WriteInt32(ITypesUtil::MAX_SIZE + 1); //exceedMaxSize size
    std::vector<Entry> output;
    ASSERT_FALSE(ITypesUtil::UnmarshalFromBuffer(parcel, output));
    ASSERT_TRUE(output.empty());
}

/**
 * @tc.name: BackupInfoSerialization001
 * @tc.desc: test BackupInfo serialization with all fields set to default values
 * @tc.type: FUNC
 */
HWTEST_F(TypesUtilTest, BackupInfoSerialization001, TestSize.Level0)
{
    MessageParcel parcel;
    BackupInfo backupIn;
    backupIn.name = "backup_file";
    backupIn.baseDir = "/data/database";
    backupIn.appId = "test.app";
    backupIn.storeId = "test_store";

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, backupIn));
    BackupInfo backupOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, backupOut));
    ASSERT_EQ(backupIn.name, backupOut.name);
    ASSERT_EQ(backupIn.baseDir, backupOut.baseDir);
    ASSERT_EQ(backupIn.appId, backupOut.appId);
    ASSERT_EQ(backupIn.storeId, backupOut.storeId);
    ASSERT_EQ(backupIn.encrypt, backupOut.encrypt);
    ASSERT_EQ(backupIn.isCheckIntegrity, backupOut.isCheckIntegrity);
    ASSERT_EQ(backupIn.subUser, backupOut.subUser);
    ASSERT_EQ(backupIn.isCustomDir, backupOut.isCustomDir);
}

/**
 * @tc.name: BackupInfoSerialization002
 * @tc.desc: test BackupInfo serialization with all boolean fields enabled
 * @tc.type: FUNC
 */
HWTEST_F(TypesUtilTest, BackupInfoSerialization002, TestSize.Level0)
{
    MessageParcel parcel;
    BackupInfo backupIn;
    backupIn.name = "encrypted_backup";
    backupIn.baseDir = "/data/custom/database";
    backupIn.appId = "secure.app";
    backupIn.storeId = "secure_store";
    backupIn.encrypt = true;
    backupIn.isCheckIntegrity = true;
    backupIn.subUser = 100;
    backupIn.isCustomDir = true;

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, backupIn));
    BackupInfo backupOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, backupOut));
    ASSERT_EQ(backupIn.name, backupOut.name);
    ASSERT_EQ(backupIn.baseDir, backupOut.baseDir);
    ASSERT_EQ(backupIn.appId, backupOut.appId);
    ASSERT_EQ(backupIn.storeId, backupOut.storeId);
    ASSERT_EQ(backupIn.encrypt, backupOut.encrypt);
    ASSERT_EQ(backupIn.isCheckIntegrity, backupOut.isCheckIntegrity);
    ASSERT_EQ(backupIn.subUser, backupOut.subUser);
    ASSERT_EQ(backupIn.isCustomDir, backupOut.isCustomDir);
    ASSERT_TRUE(backupOut.encrypt);
    ASSERT_TRUE(backupOut.isCheckIntegrity);
    ASSERT_TRUE(backupOut.isCustomDir);
    ASSERT_EQ(backupOut.subUser, 100);
}

/**
 * @tc.name: BackupInfoSerialization003
 * @tc.desc: test BackupInfo serialization with large subUser value
 * @tc.type: FUNC
 */
HWTEST_F(TypesUtilTest, BackupInfoSerialization003, TestSize.Level0)
{
    MessageParcel parcel;
    BackupInfo backupIn;
    backupIn.name = "large_user_backup";
    backupIn.baseDir = "/data/large/user/database";
    backupIn.appId = "large.app";
    backupIn.storeId = "large_user_store";
    backupIn.subUser = 2147483647; // INT32_MAX

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, backupIn));
    BackupInfo backupOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, backupOut));
    ASSERT_EQ(backupIn.name, backupOut.name);
    ASSERT_EQ(backupIn.baseDir, backupOut.baseDir);
    ASSERT_EQ(backupIn.appId, backupOut.appId);
    ASSERT_EQ(backupIn.storeId, backupOut.storeId);
    ASSERT_EQ(backupIn.subUser, backupOut.subUser);
    ASSERT_EQ(backupOut.subUser, 2147483647);
}

/**
 * @tc.name: BackupInfoSerialization004
 * @tc.desc: test BackupInfo serialization with negative subUser value
 * @tc.type: FUNC
 */
HWTEST_F(TypesUtilTest, BackupInfoSerialization004, TestSize.Level0)
{
    MessageParcel parcel;
    BackupInfo backupIn;
    backupIn.name = "negative_user_backup";
    backupIn.baseDir = "/data/negative/user/database";
    backupIn.appId = "negative.app";
    backupIn.storeId = "negative_user_store";
    backupIn.subUser = -1;

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, backupIn));
    BackupInfo backupOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, backupOut));
    ASSERT_EQ(backupIn.name, backupOut.name);
    ASSERT_EQ(backupIn.baseDir, backupOut.baseDir);
    ASSERT_EQ(backupIn.appId, backupOut.appId);
    ASSERT_EQ(backupIn.storeId, backupOut.storeId);
    ASSERT_EQ(backupIn.subUser, backupOut.subUser);
    ASSERT_EQ(backupOut.subUser, -1);
}

/**
 * @tc.name: BackupInfoSerialization005
 * @tc.desc: test BackupInfo serialization with empty strings
 * @tc.type: FUNC
 */
HWTEST_F(TypesUtilTest, BackupInfoSerialization005, TestSize.Level0)
{
    MessageParcel parcel;
    BackupInfo backupIn;
    backupIn.name = "";
    backupIn.baseDir = "";
    backupIn.appId = "";
    backupIn.storeId = "";

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, backupIn));
    BackupInfo backupOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, backupOut));
    ASSERT_EQ(backupIn.name, backupOut.name);
    ASSERT_EQ(backupIn.baseDir, backupOut.baseDir);
    ASSERT_EQ(backupIn.appId, backupOut.appId);
    ASSERT_EQ(backupIn.storeId, backupOut.storeId);
    ASSERT_TRUE(backupOut.name.empty());
    ASSERT_TRUE(backupOut.baseDir.empty());
    ASSERT_TRUE(backupOut.appId.empty());
    ASSERT_TRUE(backupOut.storeId.empty());
}

/**
 * @tc.name: BackupInfoSerialization006
 * @tc.desc: test BackupInfo serialization with special characters in strings
 * @tc.type: FUNC
 */
HWTEST_F(TypesUtilTest, BackupInfoSerialization006, TestSize.Level0)
{
    MessageParcel parcel;
    BackupInfo backupIn;
    backupIn.name = "backup_test_special@#$%";
    backupIn.baseDir = "/data/test/database/path";
    backupIn.appId = "com.example.test.app";
    backupIn.storeId = "store_test_123";

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, backupIn));
    BackupInfo backupOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, backupOut));
    ASSERT_EQ(backupIn.name, backupOut.name);
    ASSERT_EQ(backupIn.baseDir, backupOut.baseDir);
    ASSERT_EQ(backupIn.appId, backupOut.appId);
    ASSERT_EQ(backupIn.storeId, backupOut.storeId);
}

/**
 * @tc.name: BackupInfoSerialization007
 * @tc.desc: test BackupInfo serialization with long path strings
 * @tc.type: FUNC
 */
HWTEST_F(TypesUtilTest, BackupInfoSerialization007, TestSize.Level0)
{
    MessageParcel parcel;
    BackupInfo backupIn;
    backupIn.name = std::string(256, 'a'); // Long file name
    backupIn.baseDir = "/data/very/long/path/directory/" + std::string(100, 'b');
    backupIn.appId = "com.very.long.application.name.testing";
    backupIn.storeId = "store_with_very_long_name_for_testing_purposes";

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, backupIn));
    BackupInfo backupOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, backupOut));
    ASSERT_EQ(backupIn.name, backupOut.name);
    ASSERT_EQ(backupIn.baseDir, backupOut.baseDir);
    ASSERT_EQ(backupIn.appId, backupOut.appId);
    ASSERT_EQ(backupIn.storeId, backupOut.storeId);
}

/**
 * @tc.name: BackupInfoSerialization008
 * @tc.desc: test BackupInfo serialization with mixed boolean states
 * @tc.type: FUNC
 */
HWTEST_F(TypesUtilTest, BackupInfoSerialization008, TestSize.Level0)
{
    MessageParcel parcel;
    BackupInfo backupIn;
    backupIn.name = "mixed_bool_backup";
    backupIn.baseDir = "/data/mixed/database";
    backupIn.appId = "mixed.app";
    backupIn.storeId = "mixed_store";
    backupIn.encrypt = true;
    backupIn.isCheckIntegrity = false;
    backupIn.isCustomDir = true;

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, backupIn));
    BackupInfo backupOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, backupOut));
    ASSERT_EQ(backupIn.name, backupOut.name);
    ASSERT_EQ(backupIn.baseDir, backupOut.baseDir);
    ASSERT_EQ(backupIn.appId, backupOut.appId);
    ASSERT_EQ(backupIn.storeId, backupOut.storeId);
    ASSERT_TRUE(backupOut.encrypt);
    ASSERT_FALSE(backupOut.isCheckIntegrity);
    ASSERT_TRUE(backupOut.isCustomDir);
}

/**
 * @tc.name: BackupInfoSerialization009
 * @tc.desc: test BackupInfo serialization with zero subUser
 * @tc.type: FUNC
 */
HWTEST_F(TypesUtilTest, BackupInfoSerialization009, TestSize.Level0)
{
    MessageParcel parcel;
    BackupInfo backupIn;
    backupIn.name = "zero_user_backup";
    backupIn.baseDir = "/data/zero/database";
    backupIn.appId = "zero.app";
    backupIn.storeId = "zero_store";
    backupIn.subUser = 0;

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, backupIn));
    BackupInfo backupOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, backupOut));
    ASSERT_EQ(backupIn.name, backupOut.name);
    ASSERT_EQ(backupIn.baseDir, backupOut.baseDir);
    ASSERT_EQ(backupIn.appId, backupOut.appId);
    ASSERT_EQ(backupIn.storeId, backupOut.storeId);
    ASSERT_EQ(backupIn.subUser, backupOut.subUser);
    ASSERT_EQ(backupOut.subUser, 0);
}

/**
 * @tc.name: BackupInfoSerialization010
 * @tc.desc: test BackupInfo serialization with realistic production values
 * @tc.type: FUNC
 */
HWTEST_F(TypesUtilTest, BackupInfoSerialization010, TestSize.Level0)
{
    MessageParcel parcel;
    BackupInfo backupIn;
    backupIn.name = "app_data_backup_20250321";
    backupIn.baseDir = "/data/service/el2/100/database/com.example.app/kvstore";
    backupIn.appId = "com.example.app";
    backupIn.storeId = "user_preferences";
    backupIn.encrypt = true;
    backupIn.isCheckIntegrity = true;
    backupIn.subUser = 100;
    backupIn.isCustomDir = false;

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, backupIn));
    BackupInfo backupOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, backupOut));
    ASSERT_EQ(backupIn.name, backupOut.name);
    ASSERT_EQ(backupIn.baseDir, backupOut.baseDir);
    ASSERT_EQ(backupIn.appId, backupOut.appId);
    ASSERT_EQ(backupIn.storeId, backupOut.storeId);
    ASSERT_EQ(backupIn.encrypt, backupOut.encrypt);
    ASSERT_EQ(backupIn.isCheckIntegrity, backupOut.isCheckIntegrity);
    ASSERT_EQ(backupIn.subUser, backupOut.subUser);
}
}

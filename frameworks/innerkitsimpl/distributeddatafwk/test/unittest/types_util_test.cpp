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
}

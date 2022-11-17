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

#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "types.h"
#include "itypes_util.h"
namespace {
using namespace testing::ext;
using namespace OHOS::DistributedKv;

class BlobTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void BlobTest::SetUpTestCase(void)
{}

void BlobTest::TearDownTestCase(void)
{}

void BlobTest::SetUp(void)
{}

void BlobTest::TearDown(void)
{}

/**
* @tc.name: Size001
* @tc.desc: construct a Blob and check its size.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, Size001, TestSize.Level0)
{
    Blob blob1;
    EXPECT_EQ(blob1.Size(), (size_t)0);
    Blob blob2 = "1234567890";
    EXPECT_EQ(blob2.Size(), (size_t)10);
    Blob blob3("12345");
    EXPECT_EQ(blob3.Size(), (size_t)5);
    std::string strTmp = "123";
    Blob blob4(strTmp.c_str());
    EXPECT_EQ(blob4.Size(), (size_t)3);
    std::vector<uint8_t> vec = {'1', '2', '3', '4'};
    Blob blob5(vec);
    EXPECT_EQ(blob5.Size(), (size_t)4);
    const char *chr1 = strTmp.c_str();
    Blob blob6(chr1, strlen(chr1));
    EXPECT_EQ(blob6.Size(), (size_t)3);
    Blob blob7(nullptr);
    EXPECT_EQ(blob7.Size(), (size_t)0);
    Blob blob8(nullptr, strlen(chr1));
    EXPECT_EQ(blob8.Size(), (size_t)0);
}

/**
* @tc.name: Empty001
* @tc.desc: construct a Blob and check its empty.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, Empty001, TestSize.Level0)
{
    Blob blob1;
    EXPECT_EQ(blob1.Empty(), true);
    Blob blob2 = "1234567890";
    EXPECT_EQ(blob2.Empty(), false);
    Blob blob3("12345");
    EXPECT_EQ(blob3.Empty(), false);
    std::string strTmp = "123";
    Blob blob4(strTmp.c_str());
    EXPECT_EQ(blob4.Empty(), false);
    std::vector<uint8_t> vec = {'1', '2', '3', '4'};
    Blob blob5(vec);
    EXPECT_EQ(blob5.Empty(), false);
    const char *chr1 = strTmp.c_str();
    Blob blob6(chr1, strlen(chr1));
    EXPECT_EQ(blob6.Empty(), false);
}

/**
* @tc.name: Clear001
* @tc.desc: construct a Blob and check it clear function.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, Clear001, TestSize.Level0)
{
    Blob blob1 = "1234567890";
    blob1.Clear();
    EXPECT_EQ(blob1.Empty(), true);
    Blob blob2("12345");
    blob2.Clear();
    EXPECT_EQ(blob2.Empty(), true);
    std::string strTmp = "123";
    const char *chr = strTmp.c_str();
    Blob blob3(chr);
    blob3.Clear();
    EXPECT_EQ(blob3.Empty(), true);
    std::vector<uint8_t> vec = {'1', '2', '3', '4'};
    Blob blob4(vec);
    blob4.Clear();
    EXPECT_EQ(blob4.Empty(), true);
}

/**
* @tc.name: StartsWith001
* @tc.desc: construct a Blob and check it StartsWith function.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, StartsWith001, TestSize.Level0)
{
    Blob blob1 = "1234567890";
    Blob blob2("12345");
    EXPECT_EQ(blob1.StartsWith(blob2), true);
    EXPECT_EQ(blob2.StartsWith(blob1), false);
    Blob blob3("234");
    EXPECT_EQ(blob1.StartsWith(blob3), false);
    EXPECT_EQ(blob2.StartsWith(blob3), false);
}

/**
* @tc.name: Compare001
* @tc.desc: construct a Blob and check it compare function.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, Compare001, TestSize.Level0)
{
    Blob blob1 = "1234567890";
    Blob blob2("12345");
    EXPECT_EQ(blob1.Compare(blob2), 1);
    EXPECT_EQ(blob2.Compare(blob1), -1);
    Blob blob3("12345");
    EXPECT_EQ(blob2.Compare(blob3), 0);
}

/**
* @tc.name: Data001
* @tc.desc: construct a Blob and check it Data function.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, Data001, TestSize.Level0)
{
    std::vector<uint8_t> result = {'1', '2', '3', '4'};
    Blob blob1("1234");
    EXPECT_EQ(blob1.Data(), result);
    std::vector<uint8_t> result2 = {'1', '2', '3', '4', '5'};
    Blob blob2("12345");
    EXPECT_EQ(blob2.Data(), result2);
}

/**
* @tc.name: ToString001
* @tc.desc: construct a Blob and check it ToString function.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, ToString001, TestSize.Level0)
{
    Blob blob1("1234");
    std::string str = "1234";
    EXPECT_EQ(blob1.ToString(), str);
}

/**
* @tc.name: OperatorEqual001
* @tc.desc: construct a Blob and check it operator== function.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, OperatorEqual001, TestSize.Level0)
{
    Blob blob1("1234");
    Blob blob2("1234");
    EXPECT_EQ(blob1 == blob2, true);
    Blob blob3("12345");
    EXPECT_EQ(blob1 == blob3, false);
}

/**
* @tc.name: Operator001
* @tc.desc: construct a Blob and check it operator[] function.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, Operator001, TestSize.Level0)
{
    Blob blob1("1234");
    EXPECT_EQ(blob1[0], '1');
    EXPECT_EQ(blob1[1], '2');
    EXPECT_EQ(blob1[2], '3');
    EXPECT_EQ(blob1[3], '4');
    EXPECT_EQ(blob1[4], 0);
}

/**
* @tc.name: Operator002
* @tc.desc: construct a Blob and check it operator= function.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, Operator002, TestSize.Level0)
{
    Blob blob1("1234");
    Blob blob2 = blob1;
    EXPECT_EQ(blob1 == blob2, true);
    EXPECT_EQ(blob2.ToString(), "1234");
}

/**
* @tc.name: Operator003
* @tc.desc: construct a Blob and check it operator= function.
* @tc.type: FUNC
* @tc.require: AR000C6GBG
* @tc.author: liqiao
*/
HWTEST_F(BlobTest, Operator003, TestSize.Level0)
{
    Blob blob1("1234");
    Blob blob2 = std::move(blob1);
    EXPECT_EQ(blob1 == blob2, false);
    EXPECT_EQ(blob1.Empty(), true);
    EXPECT_EQ(blob2.ToString(), "1234");
}

/**
* @tc.name: Operator004
* @tc.desc: construct a Blob and check it operator std::vector<uint8_t> && function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(BlobTest, Operator004, TestSize.Level0)
{
    std::vector<uint8_t> blob = { 1, 2, 3, 4 };
    Blob blob1(move(blob));
    EXPECT_EQ(blob1.Size(), 4);
    std::vector<uint8_t> blob2 = std::move(blob1);
    EXPECT_EQ(blob2.size(), 4);
}

/**
* @tc.name: Operator005
* @tc.desc: construct a Blob and check it operator std::vector<uint8_t> & function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(BlobTest, Operator005, TestSize.Level0)
{
    const std::vector<uint8_t> blob = { 1, 2, 3, 4 };
    Blob blob1(blob);
    EXPECT_EQ(blob1.Size(), 4);
}

/**
* @tc.name: RawSize001
* @tc.desc: construct a Blob and check it RawSize function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(BlobTest, RawSize001, TestSize.Level0)
{
    Blob blob1("1234");
    Blob blob2("12345");
    EXPECT_EQ(blob1.RawSize(), sizeof(int) + 4);
    EXPECT_EQ(blob2.RawSize(), sizeof(int) + 5);
}

/**
* @tc.name: WriteToBuffer001
* @tc.desc: construct a Blob and check it WriteToBuffer and ReadFromBuffer function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(BlobTest, WriteToBuffer001, TestSize.Level1)
{
    Entry insert, update, del;
    insert.key = "insert";
    update.key = "update";
    del.key = "delete";
    insert.value = "insert_value";
    update.value = "update_value";
    del.value = "delete_value";
    std::vector<Entry> inserts, updates, deletes;
    inserts.push_back(insert);
    updates.push_back(update);
    deletes.push_back(del);

    ChangeNotification changeIn(std::move(inserts), std::move(updates), std::move(deletes), std::string(), false);
    OHOS::MessageParcel parcel;
    int64_t insertSize = ITypesUtil::GetTotalSize(changeIn.GetInsertEntries());
    int64_t updateSize = ITypesUtil::GetTotalSize(changeIn.GetUpdateEntries());
    int64_t deleteSize = ITypesUtil::GetTotalSize(changeIn.GetDeleteEntries());
    ASSERT_TRUE(ITypesUtil::MarshalToBuffer(changeIn.GetInsertEntries(), insertSize, parcel));
    ASSERT_TRUE(ITypesUtil::MarshalToBuffer(changeIn.GetUpdateEntries(), updateSize, parcel));
    ASSERT_TRUE(ITypesUtil::MarshalToBuffer(changeIn.GetDeleteEntries(), deleteSize, parcel));
    std::vector<Entry> outInserts;
    std::vector<Entry> outUpdates;
    std::vector<Entry> outDeletes;
    ASSERT_TRUE(ITypesUtil::UnmarshalFromBuffer(parcel, outInserts));
    ASSERT_TRUE(ITypesUtil::UnmarshalFromBuffer(parcel, outUpdates));
    ASSERT_TRUE(ITypesUtil::UnmarshalFromBuffer(parcel, outDeletes));
}
} // namespace
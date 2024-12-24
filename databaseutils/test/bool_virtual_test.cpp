/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "BlobVirtualTest"

#include "blob.h"
#include "kv_types_util.h"
#include "types.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

using namespace OHOS::DistributedKv;
using namespace testing;
using namespace testing::ext;
namespace OHOS::Test {
class BlobVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void BlobVirtualTest::SetUpTestCase(void)
{}

void BlobVirtualTest::TearDownTestCase(void)
{}

void BlobVirtualTest::SetUp(void)
{}

void BlobVirtualTest::TearDown(void)
{}

/**
 * @tc.name: DefaultConstructor
 * @tc.desc:
 * @tc.type: DefaultConstructor test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, DefaultConstructor)
{
    ZLOGI("DefaultConstructor begin.");
    Blob defaultConstructor;
    EXPECT_TRUE(defaultConstructor.Empty());
}

/**
 * @tc.name: CopyConstructor
 * @tc.desc:
 * @tc.type: CopyConstructor test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, CopyConstructor)
{
    ZLOGI("CopyConstructor begin.");
    Blob copyConstructor1("hello", 5);
    Blob copyConstructor2(blob1);
    EXPECT_EQ(copyConstructor1, copyConstructor2);
}

/**
 * @tc.name: MoveConstructor
 * @tc.desc:
 * @tc.type: MoveConstructor test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, MoveConstructor)
{
    ZLOGI("MoveConstructor begin.");
    Blob moveConstructor1("hello", 5);
    Blob moveConstructor2(std::move(blob1));
    EXPECT_TRUE(moveConstructor1.Empty());
    EXPECT_EQ(std::string(moveConstructor2.Data().begin(),
        moveConstructor2.Data().end()), "hello");
}

/**
 * @tc.name: CopyAssignmentOperator
 * @tc.desc:
 * @tc.type: CopyAssignmentOperator test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, CopyAssignmentOperator)
{
    ZLOGI("CopyAssignmentOperator begin.");
    Blob copyAssignmentOperator1("hello", 5);
    Blob copyAssignmentOperator2;
    copyAssignmentOperator2 = copyAssignmentOperator1;
    EXPECT_EQ(copyAssignmentOperator1, copyAssignmentOperator2);
}

/**
 * @tc.name: MoveAssignmentOperator
 * @tc.desc:
 * @tc.type: MoveAssignmentOperator test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, MoveAssignmentOperator)
{
    ZLOGI("MoveAssignmentOperator begin.");
    Blob moveAssignmentOperator1("hello", 5);
    Blob moveAssignmentOperator2;
    blob2 = std::move(moveAssignmentOperator1);
    EXPECT_TRUE(moveAssignmentOperator1.Empty());
    EXPECT_EQ(std::string(moveAssignmentOperator2.Data().begin(),
        moveAssignmentOperator2.Data().end()), "hello");
}

/**
 * @tc.name: ConstCharConstructorWithSize
 * @tc.desc:
 * @tc.type: ConstCharConstructorWithSize test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, ConstCharConstructorWithSize)
{
    ZLOGI("ConstCharConstructorWithSize begin.");
    Blob blob("ConstCharConstructorWithSize", 5);
    EXPECT_EQ(std::string(blob.Data().begin(), blob.Data().end()), "ConstCharConstructorWithSize");
}

/**
 * @tc.name: ConstCharConstructorWithNullTerminator
 * @tc.desc:
 * @tc.type: ConstCharConstructorWithNullTerminator test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, ConstCharConstructorWithNullTerminator)
{
    ZLOGI("ConstCharConstructorWithNullTerminator begin.");
    Blob blob("ConstCharConstructorWithNullTerminator");
    EXPECT_EQ(std::string(blob.Data().begin(), blob.Data().end()), "ConstCharConstructorWithNullTerminator");
}

/**
 * @tc.name: StringConstructor
 * @tc.desc:
 * @tc.type: StringConstructor test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, StringConstructor)
{
    ZLOGI("StringConstructor begin.");
    std::string str = "StringConstructor";
    Blob stringConstructor(str);
    EXPECT_EQ(std::string(stringConstructor.Data().begin(),
        stringConstructor.Data().end()), str);
}

/**
 * @tc.name: StringAssignmentOperator
 * @tc.desc:
 * @tc.type: StringAssignmentOperator test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, StringAssignmentOperator)
{
    ZLOGI("StringAssignmentOperator begin.");
    std::string str = "StringAssignmentOperator";
    Blob stringAssignmentOperator;
    stringAssignmentOperator = str;
    EXPECT_EQ(std::string(stringAssignmentOperator.Data().begin(),
        stringAssignmentOperator.Data().end()), str);
}

/**
 * @tc.name: ConstCharAssignmentOperator
 * @tc.desc:
 * @tc.type: ConstCharAssignmentOperator test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, ConstCharAssignmentOperator)
{
    ZLOGI("ConstCharAssignmentOperator begin.");
    std::string str = "StringAssignmentOperator";
    Blob constCharAssignmentOperator;
    constCharAssignmentOperator = str;
    EXPECT_EQ(std::string(constCharAssignmentOperator.Data().begin(),
        constCharAssignmentOperator.Data().end()), str);
}

/**
 * @tc.name: VectorConstructor
 * @tc.desc:
 * @tc.type: VectorConstructor test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, VectorConstructor)
{
    ZLOGI("VectorConstructor begin.");
    std::vector<uint8_t> vec = {'h', 'e', 'l', 'l', 'o'};
    Blob vectorConstructor(vec);
    EXPECT_EQ(std::string(vectorConstructor.Data().begin(),
        vectorConstructor.Data().end()), "hello");
}

/**
 * @tc.name: MoveVectorConstructor
 * @tc.desc:
 * @tc.type: MoveVectorConstructor test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, MoveVectorConstructor)
{
    ZLOGI("MoveVectorConstructor begin.");
    std::vector<uint8_t> vec = {'h', 'e', 'l', 'l', 'o'};
    Blob moveVectorConstructor(std::move(vec));
    EXPECT_EQ(std::string(moveVectorConstructor.Data().begin(),
        moveVectorConstructor.Data().end()), "hello");
    EXPECT_TRUE(vec.empty());
}

/**
 * @tc.name: DataAccessor
 * @tc.desc:
 * @tc.type: DataAccessor test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, DataAccessor)
{
    ZLOGI("DataAccessor begin.");
    Blob dataAccessor("hello", 5);
    const std::vector<uint8_t>& data = dataAccessor.Data();
    EXPECT_EQ(data[0], 'h');
    EXPECT_EQ(data[4], 'o');
}

/**
 * @tc.name: SizeAccessor
 * @tc.desc:
 * @tc.type: SizeAccessor test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, SizeAccessor)
{
    ZLOGI("SizeAccessor begin.");
    Blob sizeAccessor("hello", 5);
    EXPECT_EQ(sizeAccessor.Size(), 5u);
}

/**
 * @tc.name: RawSizeAccessor
 * @tc.desc:
 * @tc.type: RawSizeAccessor test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, RawSizeAccessor)
{
    ZLOGI("RawSizeAccessor begin.");
    Blob rawSizeAccessor("hello", 5);
    EXPECT_EQ(rawSizeAccessor.RawSize(), 9); // sizeof(int) + 5
}

/**
 * @tc.name: EmptyAccessor
 * @tc.desc:
 * @tc.type: EmptyAccessor test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, EmptyAccessor)
{
    ZLOGI("EmptyAccessor begin.");
    Blob emptyAccessor;
    EXPECT_TRUE(emptyAccessor.Empty());
    Blob emptyAccessor2("hello");
    EXPECT_FALSE(emptyAccessor2.Empty());
}

/**
 * @tc.name: ElementAccessOperator
 * @tc.desc:
 * @tc.type: ElementAccessOperator test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, ElementAccessOperator)
{
    ZLOGI("ElementAccessOperator begin.");
    Blob elementAccessOperator("hello", 5);
    EXPECT_EQ(elementAccessOperator[0], 'h');
    EXPECT_EQ(elementAccessOperator[4], 'o');
    EXPECT_EQ(elementAccessOperator[5], 0);
}

/**
 * @tc.name: EqualityOperator
 * @tc.desc:
 * @tc.type: EqualityOperator test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, EqualityOperator)
{
    ZLOGI("EqualityOperator begin.");
    Blob equalityOperator1("hello", 5);
    Blob equalityOperator2("hello", 5);
    EXPECT_TRUE(equalityOperator1 == equalityOperator2);
    Blob equalityOperator3("world", 5);
    EXPECT_FALSE(equalityOperator1 == equalityOperator3);
}

/**
 * @tc.name: Clear
 * @tc.desc:
 * @tc.type: Clear test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, Clear)
{
    ZLOGI("Clear begin.");
    Blob blobClear("hello", 5);
    blobClear.Clear();
    EXPECT_TRUE(blobClear.Empty());
}

/**
 * @tc.name: ToString
 * @tc.desc:
 * @tc.type: ToString test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, ToString)
{
    ZLOGI("ToString begin.");
    Blob blobToString("hello", 5);
    EXPECT_EQ(blobToString.ToString(), "hello");
}

/**
 * @tc.name: Compare
 * @tc.desc:
 * @tc.type: Compare test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, Compare)
{
    ZLOGI("Compare begin.");
    Blob blobCompare1("abc", 3);
    Blob blobCompare2("abcd", 4);
    Blob blobCompare3("abc", 3);
    EXPECT_LT(blobCompare1.Compare(blobCompare2), 0);
    EXPECT_EQ(blobCompare1.Compare(blobCompare3), 0);
    EXPECT_GT(blobCompare2.Compare(blobCompare1), 0);
}

/**
 * @tc.name: StartsWith
 * @tc.desc:
 * @tc.type: StartsWith test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, StartsWith)
{
    ZLOGI("StartsWith begin.");
    Blob blobStartsWith1("abcdef", 6);
    Blob blobStartsWith2("abc", 3);
    Blob blobStartsWith3("abcd", 4);
    Blob blobStartsWith4("xyz", 3);
    EXPECT_TRUE(blobStartsWith1.StartsWith(blobStartsWith2));
    EXPECT_TRUE(blobStartsWith1.StartsWith(blobStartsWith3));
    EXPECT_FALSE(blobStartsWith1.StartsWith(blobStartsWith4));
}

/**
 * @tc.name: WriteToBuffer
 * @tc.desc:
 * @tc.type: WriteToBuffer test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, WriteToBuffer)
{
    ZLOGI("WriteToBuffer begin.");
    Blob blobWriteToBuffer("hello", 5);
    uint8_t buffer[100];
    uint8_t* cursorPtr = buffer;
    int bufferLeftSize = sizeof(buffer);
    EXPECT_TRUE(blobWriteToBuffer.WriteToBuffer(cursorPtr, bufferLeftSize));
    EXPECT_EQ(*reinterpret_cast<int*>(buffer), 5);
    EXPECT_EQ(std::string(reinterpret_cast<char*>(buffer + sizeof(int)), 5), "hello");
    EXPECT_EQ(bufferLeftSize, sizeof(buffer) - sizeof(int) - 5);
}

/**
 * @tc.name: ReadFromBuffer
 * @tc.desc:
 * @tc.type: ReadFromBuffer test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BlobVirtualTest, ReadFromBuffer)
{
    ZLOGI("ReadFromBuffer begin.");
    uint8_t buffer[100];
    int blobSize = 5;
    *reinterpret_cast<int*>(buffer) = blobSize;
    memcpy(buffer + sizeof(int), "hello", blobSize);
    const uint8_t* cursorPtr = buffer;
    int bufferLeftSize = sizeof(buffer);
    Blob blob;
    EXPECT_TRUE(blob.ReadFromBuffer(cursorPtr, bufferLeftSize));
    EXPECT_EQ(std::string(blob.Data().begin(), blob.Data().end()), "hello");
    EXPECT_EQ(bufferLeftSize, sizeof(buffer) - sizeof(int) - blobSize);
}
} // namespace OHOS::Test
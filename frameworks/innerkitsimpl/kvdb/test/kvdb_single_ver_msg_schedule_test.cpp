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

#include "distributeddb_tools_unit_test.h"
#include "single_ver_data_packet.h"
#include "single_ver_kv_sync_task_context.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
}

class KvDBSingleVerMsgScheduleTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvDBSingleVerMsgScheduleTest::SetUpTestCase(void)
{
}

void KvDBSingleVerMsgScheduleTest::TearDownTestCase(void)
{
}

void KvDBSingleVerMsgScheduleTest::SetUp(void)
{
}

void KvDBSingleVerMsgScheduleTest::TearDown(void)
{
}

/**
 * @tc.name: MsgSchedule001
 * @tc.desc: Test MessageSchedule function with normal sequenceId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerMsgScheduleTest, MsgSchedule001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put message sequence_3, sequence_2, sequence_1
     * @tc.expected: put message ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerKvSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo msgInfo;
    msgInfo.sessionId_ = 10;
    bool needHandle = true;
    bool needContinue = true;
    for (uint32_t i = 3; i >= 1; i--) {
        msgInfo.sequenceId_ = i;
        msgInfo.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
        msgSchedule.PutMsg(message);
        if (i > 1) {
            Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
            ASSERT_TRUE(message == nullptr);
        }
    }
    /**
     * @tc.steps: step2. get message
     * @tc.expected: get message by sequence_1, sequence_2, sequence_3
     */
    for (uint32_t i = 1; i <= 3; i++) {
        Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
        ASSERT_TRUE(message != nullptr);
        EXPECT_EQ(needContinue, true);
        EXPECT_EQ(needHandle, true);
        EXPECT_EQ(message->GetSequenceId(), i);
        msgSchedule.ScheduleInfoHandle(needHandle, false, message);
        delete message;
    }
    Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
    ASSERT_TRUE(message == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule002
 * @tc.desc: Test MessageSchedule function with by low version
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerMsgScheduleTest, MsgSchedule002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put message session1_sequence1, session2_sequence1
     * @tc.expected: put message ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerKvSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_RELEASE_2_0);
    DataSyncMessageInfo msgInfo;
    bool needHandle = true;
    bool needContinue = true;
    for (uint32_t i = 1; i <= 2; i++) {
        msgInfo.sessionId_ = i;
        msgInfo.sequenceId_ = 1;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
        msgSchedule.PutMsg(message);
    }
    /**
     * @tc.steps: step2. get message
     * @tc.expected: get message by session2_sequence1
     */
    Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
    ASSERT_TRUE(message != nullptr);
    EXPECT_EQ(needContinue, true);
    EXPECT_EQ(needHandle, true);
    EXPECT_EQ(message->GetSequenceId(), 1u);
    msgSchedule.ScheduleInfoHandle(false, false, message);
    delete message;
    message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
    ASSERT_TRUE(message == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule003
 * @tc.desc: Test MessageSchedule function with cross sessionId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerMsgScheduleTest, MsgSchedule003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put message session1_seq1, session2_seq1, session1_seq2, session2_seq2, and handle session1_seq1
     * @tc.expected: handle ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerKvSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo msgInfo;
    bool needHandle = true;
    bool needContinue = true;
    msgInfo.sessionId_ = 1;
    msgInfo.sequenceId_ = 1;
    msgInfo.packetId_ = 1;
    DistributedDB::Message *message = nullptr;
    DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
    msgSchedule.PutMsg(message);
    Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
    ASSERT_TRUE(message != nullptr);
    EXPECT_EQ(needContinue, true);
    EXPECT_EQ(needHandle, true);
    msgSchedule.ScheduleInfoHandle(needHandle, false, message);
    delete message;
    msgInfo.sessionId_ = 2;
    msgInfo.sequenceId_ = 1;
    msgInfo.packetId_ = 1;
    DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
    msgSchedule.PutMsg(message);
    msgInfo.sessionId_ = 1;
    msgInfo.sequenceId_ = 2;
    msgInfo.packetId_ = 2;
    DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
    msgSchedule.PutMsg(message);
    msgInfo.sessionId_ = 2;
    msgInfo.sequenceId_ = 2;
    msgInfo.packetId_ = 2;
    DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
    msgSchedule.PutMsg(message);

    /**
     * @tc.steps: step2. get message
     * @tc.expected: get message by session2_seq1, session2_seq2
     */
    for (uint32_t i = 1; i <= 2; i++) {
        message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
        ASSERT_TRUE(message != nullptr);
        EXPECT_EQ(needContinue, true);
        EXPECT_EQ(needHandle, true);
        EXPECT_EQ(message->GetSequenceId(), i);
        EXPECT_EQ(message->GetSessionId(), 2u);
        msgSchedule.ScheduleInfoHandle(needHandle, false, message);
        delete message;
    }
    message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
    ASSERT_TRUE(message == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule004
 * @tc.desc: Test MessageSchedule function with same sessionId with different packetId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerMsgScheduleTest, MsgSchedule004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put message seq2_packet2, seq3_packet3, seq1_packet4, seq2_packet5, seq3_packet6
     * @tc.expected: put message ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerKvSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo msgInfo;
    msgInfo.sessionId_ = 10;
    bool needHandle = true;
    bool needContinue = true;
    for (uint32_t i = 2; i <= 3; i++) {
        msgInfo.sequenceId_ = i;
        msgInfo.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
        msgSchedule.PutMsg(message);
        Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
        ASSERT_TRUE(message == nullptr);
    }
    for (uint32_t i = 1; i <= 3; i++) {
        msgInfo.sequenceId_ = i;
        msgInfo.packetId_ = i + 3;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
        msgSchedule.PutMsg(message);
    }
    /**
     * @tc.steps: step2. get message
     * @tc.expected: drop seq2_packet2, seq3_packet3 and get seq1_packet4, seq2_packet5, seq3_packet6
     */
    needHandle = true;
    needContinue = true;
    for (uint32_t i = 1; i <= 3; i++) {
        Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
        ASSERT_TRUE(message != nullptr);
        EXPECT_EQ(needContinue, true);
        EXPECT_EQ(needHandle, true);
        EXPECT_EQ(message->GetSequenceId(), i);
        const DataRequestPacket *packet = message->GetObject<DataRequestPacket>();
        EXPECT_EQ(packet->GetPacketId(), i + 3);
        msgSchedule.ScheduleInfoHandle(needHandle, false, message);
        delete message;
    }
    Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
    ASSERT_TRUE(message == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule005
 * @tc.desc: Test MessageSchedule function with same sessionId with different packetId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerMsgScheduleTest, MsgSchedule005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put message seq1_packet4, seq2_packet5, seq2_packet2, seq3_packet3
     * @tc.expected: put message ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerKvSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo msgInfo;
    msgInfo.sessionId_ = 10;
    bool needHandle = true;
    bool needContinue = true;
    for (uint32_t i = 1; i <= 2; i++) {
        msgInfo.sequenceId_ = i;
        msgInfo.packetId_ = i + 3;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
        msgSchedule.PutMsg(message);
    }
    for (uint32_t i = 2; i <= 3; i++) {
        msgInfo.sequenceId_ = i;
        msgInfo.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
        msgSchedule.PutMsg(message);
    }
    /**
     * @tc.steps: step2. get message
     * @tc.expected: drop seq2_packet2, seq3_packet3 and get seq1_packet4, seq2_packet5
     */
    needHandle = true;
    needContinue = true;
    for (uint32_t i = 1; i <= 2; i++) {
        Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
        ASSERT_TRUE(message != nullptr);
        EXPECT_EQ(needContinue, true);
        EXPECT_EQ(needHandle, true);
        EXPECT_EQ(message->GetSequenceId(), i);
        const DataRequestPacket *packet = message->GetObject<DataRequestPacket>();
        EXPECT_EQ(packet->GetPacketId(), i + 3);
        msgSchedule.ScheduleInfoHandle(needHandle, false, message);
        delete message;
    }
    Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
    ASSERT_TRUE(message == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule006
 * @tc.desc: Test MessageSchedule function with same sessionId and same sequenceId and packetId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerMsgScheduleTest, MsgSchedule006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put message seq1_packet1, seq2_packet2
     * @tc.expected: put message ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerKvSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo msgInfo;
    msgInfo.sessionId_ = 10;
    bool needHandle = true;
    bool needContinue = true;
    for (uint32_t i = 1; i <= 2; i++) {
        msgInfo.sequenceId_ = i;
        msgInfo.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
        msgSchedule.PutMsg(message);
    }
    for (uint32_t i = 1; i <= 2; i++) {
        Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
        ASSERT_TRUE(message != nullptr);
        EXPECT_EQ(needContinue, true);
        EXPECT_EQ(needHandle, true);
        EXPECT_EQ(message->GetSequenceId(), i);
        msgSchedule.ScheduleInfoHandle(needHandle, false, message);
        delete message;
    }
    /**
     * @tc.steps: step2. put message seq1_packet1, seq2_packet2 again and seq3_packet3
     * @tc.expected: get message ok and get seq1_packet1, seq2_packet2 and seq3_packet3
     */
    for (uint32_t i = 1; i <= 3; i++) {
        msgInfo.sequenceId_ = i;
        msgInfo.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
        msgSchedule.PutMsg(message);
    }
    needHandle = true;
    needContinue = true;
    for (uint32_t i = 1; i <= 3; i++) {
        Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
        ASSERT_TRUE(message != nullptr);
        EXPECT_EQ(needContinue, true);
        EXPECT_EQ(needHandle, (i == 3) ? true : false);
        EXPECT_EQ(message->GetSequenceId(), i);
        const DataRequestPacket *packet = message->GetObject<DataRequestPacket>();
        EXPECT_EQ(packet->GetPacketId(), i);
        msgSchedule.ScheduleInfoHandle(needHandle, false, message);
        delete message;
    }
    Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
    ASSERT_TRUE(message == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule007
 * @tc.desc: Test MessageSchedule function with same sessionId and duplicate sequenceId and low packetId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerMsgScheduleTest, MsgSchedule007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put message seq1_packet4, seq2_packet5
     * @tc.expected: put message ok and get message seq1_packet4, seq2_packet5
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerKvSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo msgInfo;
    msgInfo.sessionId_ = 10;
    bool needHandle = true;
    bool needContinue = true;
    for (uint32_t i = 1; i <= 2; i++) {
        msgInfo.sequenceId_ = i;
        msgInfo.packetId_ = i + 3;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
        msgSchedule.PutMsg(message);
    }
    for (uint32_t i = 1; i <= 2; i++) {
        Message *message = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
        ASSERT_TRUE(message != nullptr);
        EXPECT_EQ(needContinue, true);
        EXPECT_EQ(needHandle, true);
        EXPECT_EQ(message->GetSequenceId(), i);
        const DataRequestPacket *packet = message->GetObject<DataRequestPacket>();
        EXPECT_EQ(packet->GetPacketId(), i + 3);
        msgSchedule.ScheduleInfoHandle(needHandle, false, message);
        delete message;
    }
    Message *msg2 = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
    ASSERT_TRUE(msg2 == nullptr);
    /**
     * @tc.steps: step2. put message seq1_packet1, seq2_packet2
     * @tc.expected: get nullptr
     */
    for (uint32_t i = 1; i <= 2; i++) {
        msgInfo.sequenceId_ = i;
        msgInfo.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(msgInfo, message);
        msgSchedule.PutMsg(message);
    }
    needHandle = true;
    needContinue = true;
    for (uint32_t i = 1; i <= 3; i++) {
        Message *msg3 = msgSchedule.MoveNextMsg(context, needHandle, needContinue);
        EXPECT_EQ(needContinue, true);
        ASSERT_TRUE(msg3 == nullptr);
    }
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}
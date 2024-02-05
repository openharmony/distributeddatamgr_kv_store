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

#include <atomic>
#include <gtest/gtest.h>
#include <thread>

#include "db_errno.h"
#include "distributeddb_tools_unit_test.h"
#include "evloop/src/event_impl.h"
#include "evloop/src/event_loop_epoll.h"
#include "evloop/src/ievent.h"
#include "evloop/src/ievent_loop.h"
#include "log_print.h"
#include "platform_specific.h"

using namespace testing::ext;
using namespace DistributedDB;

namespace {
    IEventLoop *g_loop = nullptr;
    constexpr int MAX_RETRY_TIMES = 1000;
    constexpr int RETRY_TIMES_5 = 5;
    constexpr int EPOLL_INIT_REVENTS = 32;
    constexpr int ET_READ = 0x01;
    constexpr int ET_WRITE = 0x02;
    constexpr int ET_TIMEOUT = 0x08;
    constexpr EventTime TIME_INACCURACY = 100LL;
    constexpr EventTime TIME_PIECE_1 = 1LL;
    constexpr EventTime TIME_PIECE_10 = 10LL;
    constexpr EventTime TIME_PIECE_50 = 50LL;
    constexpr EventTime TIME_PIECE_100 = 100LL;
    constexpr EventTime TIME_PIECE_1000 = 1000LL;
    constexpr EventTime TIME_PIECE_10000 = 10000LL;

class TimerTester {
public:
    static EventTime GetCurrentTime();
};

EventTime TimerTester::GetCurrentTime()
{
    uint64_t now;
    int errCode = OS::GetCurrentSysTimeInMicrosecond(now);
    if (errCode != E_OK) {
        LOGE("Get current time failed.");
        return 0;
    }
    return now / 1000; // 1 ms equals to 1000 us
}

class DistributedDBEventLoopTimerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBEventLoopTimerTest::SetUpTestCase(void) {}

void DistributedDBEventLoopTimerTest::TearDownTestCase(void) {}

void DistributedDBEventLoopTimerTest::SetUp(void)
{
    DistributedDBUnitTest::DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: Create a loop object.
     */
    if (g_loop == nullptr) {
        int errCode = E_OK;
        g_loop = IEventLoop::CreateEventLoop(errCode);
        if (g_loop == nullptr) {
            LOGE("Prepare loop in SetUp() failed.");
        }
    }
}

void DistributedDBEventLoopTimerTest::TearDown(void)
{
    /**
     * @tc.teardown: Destroy the loop object.
     */
    if (g_loop != nullptr) {
        g_loop->KillAndDecObjRef(g_loop);
        g_loop = nullptr;
    }
}

/**
 * @tc.name: EventLoopTimerTest001
 * @tc.desc: Create and destroy the event loop object.
 * @tc.type: FUNC
 * @tc.require: AR000CKRTB AR000CQE0C
 * @tc.author: fangyi
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventLoopTimerTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create a loop.
     * @tc.expected: step1. create successfully.
     */
    int errCode = E_OK;
    IEventLoop *loop = IEventLoop::CreateEventLoop(errCode);
    ASSERT_EQ(loop != nullptr, true);

    /**
     * @tc.steps: step2. destroy the loop.
     * @tc.expected: step2. destroy successfully.
     */
    bool finalized = false;
    loop->OnLastRef([&finalized]() { finalized = true; });
    loop->DecObjRef(loop);
    loop = nullptr;
    EXPECT_EQ(finalized, true);
}

/**
 * @tc.name: EventLoopTimerTest002
 * @tc.desc: Start and stop the loop
 * @tc.type: FUNC
 * @tc.require: AR000CKRTB AR000CQE0C
 * @tc.author: fangyi
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventLoopTimerTest002, TestSize.Level1)
{
    // ready data
    ASSERT_EQ(g_loop != nullptr, true);

    /**
     * @tc.steps: step1. create a loop.
     * @tc.expected: step1. create successfully.
     */
    std::atomic<bool> running(false);
    EventTime delta = 0;
    std::thread loopThread([&running, &delta]() {
            running = true;
            EventTime start = TimerTester::GetCurrentTime();
            g_loop->Run();
            EventTime end = TimerTester::GetCurrentTime();
            delta = end - start;
        });
    while (!running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PIECE_1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PIECE_100));
    g_loop->KillObj();
    loopThread.join();
    EXPECT_EQ(delta > TIME_PIECE_50, true);
}

/**
 * @tc.name: EventLoopTimerTest003
 * @tc.desc: Create and destroy a timer object.
 * @tc.type: FUNC
 * @tc.require: AR000CKRTB AR000CQE0C
 * @tc.author: fangyi
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventLoopTimerTest003, TestSize.Level0)
{
     /**
     * @tc.steps: step1. create event(timer) object.
     * @tc.expected: step1. create successfully.
     */
    int errCode = E_OK;
    IEvent *timer = IEvent::CreateEvent(TIME_PIECE_1, errCode);
    ASSERT_EQ(timer != nullptr, true);

    /**
     * @tc.steps: step2. destroy the event object.
     * @tc.expected: step2. destroy successfully.
     */
    bool finalized = false;
    errCode = timer->SetAction([](EventsMask revents) -> int {
            return E_OK;
        }, [&finalized]() {
            finalized = true;
        });
    EXPECT_EQ(errCode, E_OK);
    timer->KillAndDecObjRef(timer);
    timer = nullptr;
    EXPECT_EQ(finalized, true);
}

/**
 * @tc.name: EventLoopTimerTest004
 * @tc.desc: Start a timer
 * @tc.type: FUNC
 * @tc.require: AR000CKRTB AR000CQE0C
 * @tc.author: fangyi
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventLoopTimerTest004, TestSize.Level1)
{
    // ready data
    ASSERT_EQ(g_loop != nullptr, true);

    /**
     * @tc.steps: step1. start the loop.
     * @tc.expected: step1. start successfully.
     */
    std::atomic<bool> running(false);
    std::thread loopThread([&running]() {
            running = true;
            g_loop->Run();
        });

    int tryCounter = 0;
    while (!running) {
        tryCounter++;
        if (tryCounter >= MAX_RETRY_TIMES) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PIECE_1));
    }
    EXPECT_EQ(running, true);

    /**
     * @tc.steps: step2. create and start a timer.
     * @tc.expected: step2. start successfully.
     */
    int errCode = E_OK;
    IEvent *timer = IEvent::CreateEvent(TIME_PIECE_10, errCode);
    ASSERT_EQ(timer != nullptr, true);
    std::atomic<int> counter(0);
    errCode = timer->SetAction([&counter](EventsMask revents) -> int { ++counter; return E_OK; }, nullptr);
    EXPECT_EQ(errCode, E_OK);
    errCode = g_loop->Add(timer);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step3. wait and check.
     * @tc.expected: step3. 'counter' increased by the timer.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PIECE_100));
    EXPECT_EQ(counter > 0, true);
    g_loop->KillObj();
    loopThread.join();
    timer->DecObjRef(timer);
}

/**
 * @tc.name: EventLoopTimerTest005
 * @tc.desc: Stop a timer
 * @tc.type: FUNC
 * @tc.require: AR000CKRTB AR000CQE0C
 * @tc.author: fangyi
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventLoopTimerTest005, TestSize.Level1)
{
    // ready data
    ASSERT_EQ(g_loop != nullptr, true);

    /**
     * @tc.steps: step1. start the loop.
     * @tc.expected: step1. start successfully.
     */
    std::atomic<bool> running(false);
    std::thread loopThread([&running]() {
            running = true;
            g_loop->Run();
        });

    int tryCounter = 1;
    while (!running && tryCounter <= MAX_RETRY_TIMES) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        tryCounter++;
    }
    EXPECT_EQ(running, true);

    /**
     * @tc.steps: step2. create and start a timer.
     * @tc.expected: step2. start successfully.
     */
    int errCode = E_OK;
    IEvent *timer = IEvent::CreateEvent(10, errCode);
    ASSERT_EQ(timer != nullptr, true);
    std::atomic<int> counter(0);
    std::atomic<bool> finalize(false);
    errCode = timer->SetAction(
        [&counter](EventsMask revents) -> int {
            ++counter;
            return E_OK;
        }, [&finalize]() { finalize = true; });
    EXPECT_EQ(errCode, E_OK);
    errCode = g_loop->Add(timer);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step3. wait and check.
     * @tc.expected: step3. 'counter' increased by the timer and the timer object finalized.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PIECE_100));
    timer->KillAndDecObjRef(timer);
    timer = nullptr;
    g_loop->KillObj();
    loopThread.join();
    EXPECT_EQ(counter > 0, true);
    EXPECT_EQ(finalize, true);
}

/**
 * @tc.name: EventLoopTimerTest006
 * @tc.desc: Stop a timer
 * @tc.type: FUNC
 * @tc.require: AR000CKRTB AR000CQE0C
 * @tc.author: fangyi
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventLoopTimerTest006, TestSize.Level1)
{
    // ready data
    ASSERT_EQ(g_loop != nullptr, true);

    /**
     * @tc.steps: step1. start the loop.
     * @tc.expected: step1. start successfully.
     */
    std::atomic<bool> running(false);
    std::thread loopThread([&running]() {
            running = true;
            g_loop->Run();
        });

    int tryCounter = 1;
    while (!running && tryCounter <= MAX_RETRY_TIMES) {
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PIECE_10));
        tryCounter++;
    }
    EXPECT_EQ(running, true);

    /**
     * @tc.steps: step2. create and start a timer.
     * @tc.expected: step2. start successfully.
     */
    int errCode = E_OK;
    IEvent *timer = IEvent::CreateEvent(TIME_PIECE_10, errCode);
    ASSERT_EQ(timer != nullptr, true);
    std::atomic<int> counter(0);
    std::atomic<bool> finalize(false);
    errCode = timer->SetAction([&counter](EventsMask revents) -> int { ++counter; return -E_STALE; },
        [&finalize]() { finalize = true; });
    EXPECT_EQ(errCode, E_OK);
    errCode = g_loop->Add(timer);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step3. wait and check.
     * @tc.expected: step3. 'counter' increased by the timer and the timer object finalized.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PIECE_100));
    g_loop->KillObj();
    loopThread.join();
    timer->DecObjRef(timer);
    timer = nullptr;
    EXPECT_EQ(finalize, true);
    EXPECT_EQ(counter > 0, true);
}

/**
 * @tc.name: EventLoopTimerTest007
 * @tc.desc: Modify a timer
 * @tc.type: FUNC
 * @tc.require: AR000CKRTB AR000CQE0C
 * @tc.author: fangyi
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventLoopTimerTest007, TestSize.Level2)
{
    // ready data
    ASSERT_EQ(g_loop != nullptr, true);

    /**
     * @tc.steps: step1. start the loop.
     * @tc.expected: step1. start successfully.
     */
    std::atomic<bool> running(false);
    std::thread loopThread([&running]() {
            running = true;
            g_loop->Run();
        });

    int tryCounter = 1;
    while (!running && tryCounter <= MAX_RETRY_TIMES) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        tryCounter++;
    }
    EXPECT_EQ(running, true);

    /**
     * @tc.steps: step2. create and start a timer.
     * @tc.expected: step2. start successfully.
     */
    int errCode = E_OK;
    IEvent *timer = IEvent::CreateEvent(TIME_PIECE_1000, errCode);
    ASSERT_EQ(timer != nullptr, true);
    int counter = 1; // Interval: 1 * TIME_PIECE_100
    EventTime lastTime = TimerTester::GetCurrentTime();
    errCode = timer->SetAction(
        [timer, &counter, &lastTime](EventsMask revents) -> int {
            EventTime now = TimerTester::GetCurrentTime();
            EventTime delta = now - lastTime;
            delta -= counter * TIME_PIECE_1000;
            EXPECT_EQ(delta >= -TIME_INACCURACY && delta <= TIME_INACCURACY, true);
            if (++counter > RETRY_TIMES_5) {
                return -E_STALE;
            }
            lastTime = TimerTester::GetCurrentTime();
            int ret = timer->SetTimeout(counter * TIME_PIECE_1000);
            EXPECT_EQ(ret, E_OK);
            return E_OK;
        }, nullptr);
    EXPECT_EQ(errCode, E_OK);
    errCode = g_loop->Add(timer);
    EXPECT_EQ(errCode, E_OK);

    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PIECE_10000));
    g_loop->KillObj();
    loopThread.join();
    timer->DecObjRef(timer);
}

/**
 * @tc.name: EventLoopTest001
 * @tc.desc: Test Initialize twice
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventLoopTest001, TestSize.Level0)
{
    EventLoopEpoll *loop = new (std::nothrow) EventLoopEpoll;

    EXPECT_EQ(loop->Initialize(), E_OK);
    EXPECT_EQ(loop->Initialize(), -E_INVALID_ARGS);
}

/**
 * @tc.name: EventLoopTest002
 * @tc.desc: Test interface if args is invalid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventLoopTest002, TestSize.Level0)
{
    // ready data
    ASSERT_NE(g_loop, nullptr);

    /**
     * @tc.steps: step1. test the loop interface.
     * @tc.expected: step1. return INVALID_AGRS.
     */
    EXPECT_EQ(g_loop->Add(nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(g_loop->Remove(nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(g_loop->Stop(), E_OK);

    EventLoopImpl *loopImpl= static_cast<EventLoopImpl *>(g_loop);
    EventsMask events = 1u;
    EXPECT_EQ(loopImpl->Modify(nullptr, true, events), -E_INVALID_ARGS);
    EXPECT_EQ(loopImpl->Modify(nullptr, 0), -E_INVALID_ARGS);
}

/**
 * @tc.name: EventTest001
 * @tc.desc: Test CreateEvent if args is invalid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. set EventTime = -1 and CreateEvent
     * @tc.expected: step1. return INVALID_ARGS
     */
    EventTime eventTime = -1; // -1 is invalid arg
    int errCode = E_OK;
    IEvent *event = IEvent::CreateEvent(eventTime, errCode);
    ASSERT_EQ(event, nullptr);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);

    /**
     * @tc.steps:step2. set EventsMask = 0 and CreateEvent
     * @tc.expected: step2. return INVALID_ARGS
     */
    EventFd eventFd = EventFd();
    EventsMask events = 0u;
    event = IEvent::CreateEvent(eventFd, events, eventTime, errCode);
    ASSERT_EQ(event, nullptr);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);

    /**
     * @tc.steps:step3. set EventsMask = 4 and EventFd is invalid then CreateEvent
     * @tc.expected: step3. return INVALID_ARGS
     */
    EventsMask eventsMask = 1u; // 1 is ET_READ
    event = IEvent::CreateEvent(eventFd, eventsMask, eventTime, errCode);
    ASSERT_EQ(event, nullptr);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);

    /**
     * @tc.steps:step4. set EventsMask = 8 and CreateEvent
     * @tc.expected: step4. return INVALID_ARGS
     */
    events |= ET_TIMEOUT;
    event = IEvent::CreateEvent(eventFd, events, eventTime, errCode);
    ASSERT_EQ(event, nullptr);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);

    /**
     * @tc.steps:step5. set EventTime = 1 and CreateEvent
     * @tc.expected: step5. return OK
     */
    eventTime = TIME_PIECE_1;
    eventFd = EventFd(epoll_create(EPOLL_INIT_REVENTS));
    event = IEvent::CreateEvent(eventFd, events, eventTime, errCode);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: EventTest002
 * @tc.desc: Test SetAction if action is nullptr
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. CreateEvent
     * @tc.expected: step1. return OK
     */
    EventTime eventTime = TIME_PIECE_1;
    int errCode = E_OK;
    IEvent *event = IEvent::CreateEvent(eventTime, errCode);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps:step2. SetAction with nullptr
     * @tc.expected: step2. return INVALID_ARGS
     */
    EXPECT_EQ(event->SetAction(nullptr), -E_INVALID_ARGS);
}

/**
 * @tc.name: EventTest003
 * @tc.desc: Test AddEvents and RemoveEvents with fd is invalid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventTest003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. CreateEvent
     * @tc.expected: step1. return OK
     */
    EventTime eventTime = TIME_PIECE_1;
    int errCode = E_OK;
    IEvent *event = IEvent::CreateEvent(eventTime, errCode);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps:step2. AddEvents and RemoveEvents with events is 0
     * @tc.expected: step2. return INVALID_ARGS
     */
    EventsMask events = 0u;
    EXPECT_EQ(event->AddEvents(events), -E_INVALID_ARGS);
    EXPECT_EQ(event->RemoveEvents(events), -E_INVALID_ARGS);

    /**
     * @tc.steps:step3. AddEvents and RemoveEvents with fd is invalid
     * @tc.expected: step3. return OK
     */
    events |= ET_READ;
    EXPECT_EQ(event->AddEvents(events), -E_INVALID_ARGS);
    EXPECT_EQ(event->RemoveEvents(events), -E_INVALID_ARGS);
}

/**
 * @tc.name: EventTest004
 * @tc.desc: Test AddEvents and RemoveEvents with fd is valid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventTest004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. CreateEvent
     * @tc.expected: step1. return OK
     */
    EventTime eventTime = TIME_PIECE_1;
    EventFd eventFd = EventFd(epoll_create(EPOLL_INIT_REVENTS));
    EventsMask events = 1u; // 1 means ET_READ
    int errCode = E_OK;
    IEvent *event = IEvent::CreateEvent(eventFd, events, eventTime, errCode);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps:step2. AddEvents and RemoveEvents with fd is valid
     * @tc.expected: step2. return OK
     */
    events |= ET_WRITE;
    EXPECT_EQ(event->AddEvents(events), E_OK);
    EXPECT_EQ(event->RemoveEvents(events), E_OK);

    /**
     * @tc.steps:step3. AddEvents and RemoveEvents after set action
     * @tc.expected: step3. return OK
     */
    ASSERT_EQ(g_loop->Add(event), -E_INVALID_ARGS);
    ASSERT_EQ(event->SetAction([](EventsMask revents) -> int {
        return E_OK;
        }), E_OK);
    ASSERT_EQ(g_loop->Add(event), E_OK);
    EXPECT_EQ(event->AddEvents(events), E_OK);
    EXPECT_EQ(event->RemoveEvents(events), E_OK);
}

/**
 * @tc.name: EventTest005
 * @tc.desc: Test constructor method with timeout < 0
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventTest005, TestSize.Level0)
{
    /**
     * @tc.steps:step1. instantiation event with eventTime
     * @tc.expected: step1. return OK
     */
    EventTime eventTime = -1; // -1 is invalid arg
    IEvent *event = new (std::nothrow) EventImpl(eventTime);
    ASSERT_NE(event, nullptr);

    /**
     * @tc.steps:step2. instantiation event with eventFd, events, eventTime
     * @tc.expected: step2. return OK
     */
    EventFd eventFd = EventFd();
    EventsMask events = 1u; // 1 means ET_READ
    EventImpl *eventImpl = new (std::nothrow) EventImpl(eventFd, events, eventTime);
    ASSERT_NE(eventImpl, nullptr);
}

/**
 * @tc.name: EventTest006
 * @tc.desc: Test SetTimeout
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventTest006, TestSize.Level0)
{
    /**
     * @tc.steps:step1. CreateEvent
     * @tc.expected: step1. return OK
     */
    EventTime eventTime = TIME_PIECE_1;
    int errCode = E_OK;
    IEvent *event = IEvent::CreateEvent(eventTime, errCode);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps:step2. SetTimeout
     * @tc.expected: step2. return INVALID_ARGS
     */
    event->IgnoreFinalizer();
    EXPECT_EQ(event->SetTimeout(eventTime), E_OK);
    eventTime = -1; // -1 is invalid args
    EXPECT_EQ(event->SetTimeout(eventTime), -E_INVALID_ARGS);
}

/**
 * @tc.name: EventTest007
 * @tc.desc: Test SetEvents and GetEvents
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventTest007, TestSize.Level0)
{
    /**
     * @tc.steps:step1. CreateEvent
     * @tc.expected: step1. return OK
     */
    EventTime eventTime = TIME_PIECE_1;
    EventFd eventFd = EventFd(epoll_create(EPOLL_INIT_REVENTS));
    EventsMask events = 1u; // 1 means ET_READ
    int errCode = E_OK;
    IEvent *event = IEvent::CreateEvent(eventFd, events, eventTime, errCode);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps:step2. Test GetEventFd and GetEvents
     * @tc.expected: step2. return OK
     */
    EventImpl *eventImpl = static_cast<EventImpl *>(event);
    eventImpl->SetRevents(events);
    EXPECT_EQ(eventImpl->GetEventFd(), eventFd);
    EXPECT_EQ(eventImpl->GetEvents(), events);
    events = 2u; // 2 means ET_WRITE
    eventImpl->SetEvents(true, events);
    EXPECT_EQ(eventImpl->GetEvents(), 3u); // 3 means ET_WRITE | ET_READ
    eventImpl->SetEvents(false, events);
    EXPECT_EQ(eventImpl->GetEvents(), 1u); // 1 means ET_READ
    EXPECT_FALSE(eventImpl->GetTimeoutPoint(eventTime));
}

/**
 * @tc.name: EventTest008
 * @tc.desc: Test SetTimeoutPeriod and GetTimeoutPoint
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBEventLoopTimerTest, EventTest008, TestSize.Level0)
{
    /**
     * @tc.steps:step1. CreateEvent
     * @tc.expected: step1. return OK
     */
    EventTime eventTime = TIME_PIECE_1;
    int errCode = E_OK;
    IEvent *event = IEvent::CreateEvent(eventTime, errCode);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps:step2. SetTimeoutPeriod and GetTimeoutPoint
     * @tc.expected: step2. return OK
     */
    EventImpl *eventImpl = static_cast<EventImpl *>(event);
    eventTime = -1; // -1 is invalid args
    eventImpl->SetTimeoutPeriod(eventTime);
    EXPECT_TRUE(eventImpl->GetTimeoutPoint(eventTime));

    /**
     * @tc.steps:step3. Dispatch
     * @tc.expected: step3. return INVALID_ARGS
     */
    EXPECT_EQ(eventImpl->Dispatch(), -E_INVALID_ARGS);
}
}

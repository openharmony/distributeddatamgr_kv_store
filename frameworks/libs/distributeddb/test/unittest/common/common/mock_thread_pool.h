/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef MOCK_THREAD_POOL_H
#define MOCK_THREAD_POOL_H
#include <gmock/gmock.h>
#include "thread_pool_test_stub.h"

namespace DistributedDB {
class MockThreadPool : public ThreadPoolTestStub {
public:
    MOCK_METHOD1(Execute, TaskId(const Task &));
    MOCK_METHOD2(Execute, TaskId(const Task &, Duration));
    MOCK_METHOD2(Schedule, TaskId(const Task &, Duration));
    MOCK_METHOD3(Schedule, TaskId(const Task &, Duration, Duration));
    MOCK_METHOD4(Schedule, TaskId(const Task &, Duration, Duration, uint64_t));
    MOCK_METHOD2(Remove, bool(const TaskId &, bool));
    MOCK_METHOD2(Reset, TaskId(const TaskId &, Duration));
};
}
#endif // MOCK_THREAD_POOL_H

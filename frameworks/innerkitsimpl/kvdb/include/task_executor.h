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
#ifndef DISTRIBUTED_DATA_TASK_EXECUTOR_H
#define DISTRIBUTED_DATA_TASK_EXECUTOR_H
#include <map>
#include "task_scheduler.h"
namespace OHOS::DistributedKv {
class TaskExecutor {
public:
    API_EXPORT static TaskExecutor &GetInstance();
    API_EXPORT bool Execute(TaskScheduler::Task &&task);

private:
    TaskExecutor();
    ~TaskExecutor();

    static constexpr int POOL_SIZE = 3;

    std::shared_ptr<TaskScheduler> pool_;
};
} // namespace OHOS::DistributedKv
#endif // DISTRIBUTEDDATAMGR_DATAMGR_EXECUTOR_FACTORY_H

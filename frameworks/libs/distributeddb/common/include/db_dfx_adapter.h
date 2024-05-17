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

#ifndef DB_DFX_ADAPTER_H
#define DB_DFX_ADAPTER_H

#include <string>
#include <vector>

namespace DistributedDB {
enum DBEventType {
    FAULT = 1,
    STATISTIC = 2,
    SECURITY = 3,
    BEHAVIOR = 4
};

enum class Scene {
    OPEN_CONN = 1,
    SEND_RECV_DATA,
    CLOUD_SYNC,
    DATA_ACCESS,
    SEARCH_DATA,
    DEVICE_SYNC
};

enum class State {
    BEGIN = 0,
    END
};

enum class Stage {
    GET_DB = 0,
    CHECK_OPT,
    GET_DB_CONN,
    CLOUD_SYNC,
    CLOUD_DOWNLOAD,
    CLOUD_UPLOAD,
    CLOUD_NOTIFY,
    DEVICE_SYNC,
};

enum class StageResult {
    IDLE = 0,
    SUCC,
    FAIL,
    CANCLE,
    UNKNOWN
};

struct ReportTask {
    std::string funcName;
    Scene scene;
    State state;
    Stage stage;
    StageResult result;
    int errCode = 0;
};

class DBDfxAdapter final {
public:
    static void Dump(int fd, const std::vector<std::u16string> &args);

    static void ReportBehavior(const ReportTask &reportTask);

    static void StartTrace(const std::string &action);
    static void FinishTrace();

    static void StartTracing();
    static void FinishTracing();

    static void StartAsyncTrace(const std::string &action, int32_t taskId);
    static void FinishAsyncTrace(const std::string &action, int32_t taskId);

    static const std::string SYNC_ACTION;
    static const std::string EVENT_OPEN_DATABASE_FAILED;
private:
    static const std::string ORG_PKG;
    static const std::string FUNC;
    static const std::string BIZ_SCENE;
    static const std::string BIZ_STATE;
    static const std::string BIZ_STAGE;
    static const std::string STAGE_RES;
    static const std::string ERROR_CODE;
    static const std::string DISTRIBUTED_DB_BEHAVIOR;
    static const std::string ORG_PKG_NAME;
    static const std::string SQLITE_EXECUTE;

    static constexpr int E_DB_DFX_BASE = 27328512;
};
} // namespace DistributedDB

#endif // DB_DFX_ADAPTER_H

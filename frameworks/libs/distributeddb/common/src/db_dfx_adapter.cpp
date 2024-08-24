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
#include "db_dfx_adapter.h"

#include <codecvt>
#include <cstdio>
#include <locale>
#include <string>

#include "log_print.h"
#include "db_dump_helper.h"
#include "db_errno.h"
#include "kvdb_manager.h"
#include "relational_store_instance.h"
#include "runtime_context.h"
#include "sqlite_utils.h"
#ifdef USE_DFX_ABILITY
#include "hitrace_meter.h"
#include "hisysevent.h"
#endif

namespace DistributedDB {
namespace {
#ifdef USE_DFX_ABILITY
constexpr uint64_t HITRACE_LABEL = HITRACE_TAG_DISTRIBUTEDDATA;
#endif
constexpr const char *DUMP_LONG_PARAM = "--database";
constexpr const char *DUMP_SHORT_PARAM = "-d";
}

const std::string DBDfxAdapter::ORG_PKG = "ORG_PKG";
const std::string DBDfxAdapter::FUNC = "FUNC";
const std::string DBDfxAdapter::BIZ_SCENE = "BIZ_SCENE";
const std::string DBDfxAdapter::BIZ_STATE = "BIZ_STATE";
const std::string DBDfxAdapter::BIZ_STAGE = "BIZ_STAGE";
const std::string DBDfxAdapter::STAGE_RES = "STAGE_RES";
const std::string DBDfxAdapter::ERROR_CODE = "ERROR_CODE";
const std::string DBDfxAdapter::ORG_PKG_NAME = "distributeddata";
const std::string DBDfxAdapter::DISTRIBUTED_DB_BEHAVIOR = "DISTRIBUTED_DB_BEHAVIOR";
const std::string DBDfxAdapter::SQLITE_EXECUTE = "SQLITE_EXECUTE";
const std::string DBDfxAdapter::SYNC_ACTION = "SYNC_ACTION";
const std::string DBDfxAdapter::EVENT_OPEN_DATABASE_FAILED = "OPEN_DATABASE_FAILED";

void DBDfxAdapter::Dump(int fd, const std::vector<std::u16string> &args)
{
    if (!args.empty()) {
        const std::u16string longParam =
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(DUMP_LONG_PARAM);
        const std::u16string shortParam =
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(DUMP_SHORT_PARAM);
        auto find = std::any_of(args.begin(), args.end(), [&longParam, &shortParam](const std::u16string &arg) {
            return arg == longParam || arg == shortParam;
        });
        if (!find) {
            return;
        }
    }
    DBDumpHelper::Dump(fd, "DistributedDB Dump Message Info:\n\n");
    DBDumpHelper::Dump(fd, "DistributedDB Database Basic Message Info:\n");
    auto kvDBManager = KvDBManager::GetInstance();
    if (kvDBManager == nullptr) {
        return;
    }
    kvDBManager->Dump(fd);
    RelationalStoreInstance::GetInstance()->Dump(fd);
    DBDumpHelper::Dump(fd, "DistributedDB Common Message Info:\n");
    RuntimeContext::GetInstance()->DumpCommonInfo(fd);
    DBDumpHelper::Dump(fd, "\tlast error msg = %s\n", SQLiteUtils::GetLastErrorMsg().c_str());
}

#ifdef USE_DFX_ABILITY
void DBDfxAdapter::ReportBehavior(const ReportTask &reportTask)
{
    int dbDfxErrCode = -(reportTask.errCode - E_BASE) + E_DB_DFX_BASE;
    RuntimeContext::GetInstance()->ScheduleTask([=]() {
        // call hievent here
        HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            DISTRIBUTED_DB_BEHAVIOR,
            OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
            ORG_PKG, ORG_PKG_NAME,
            FUNC, reportTask.funcName,
            BIZ_SCENE, static_cast<int>(reportTask.scene),
            BIZ_STATE, static_cast<int>(reportTask.state),
            BIZ_STAGE, static_cast<int>(reportTask.stage),
            STAGE_RES, static_cast<int>(reportTask.result),
            ERROR_CODE, dbDfxErrCode);
    });
}

void DBDfxAdapter::StartTrace(const std::string &action)
{
    ::StartTrace(HITRACE_LABEL, action);
}

void DBDfxAdapter::FinishTrace()
{
    ::FinishTrace(HITRACE_LABEL);
}

void DBDfxAdapter::StartTracing()
{
#ifdef TRACE_SQLITE_EXECUTE
    ::StartTrace(HITRACE_LABEL, SQLITE_EXECUTE);
#endif
}

void DBDfxAdapter::FinishTracing()
{
#ifdef TRACE_SQLITE_EXECUTE
    ::FinishTrace(HITRACE_LABEL);
#endif
}

void DBDfxAdapter::StartAsyncTrace(const std::string &action, int32_t taskId)
{
    // call hitrace here
    // need include bytrace.h
    ::StartAsyncTrace(HITRACE_LABEL, action, taskId);
}

void DBDfxAdapter::FinishAsyncTrace(const std::string &action, int32_t taskId)
{
    // call hitrace here
    ::FinishAsyncTrace(HITRACE_LABEL, action, taskId);
}

#else
void DBDfxAdapter::ReportBehavior(const ReportTask &reportTask)
{
    (void) reportTask;
}

void DBDfxAdapter::StartTrace(const std::string &action)
{
    (void) action;
}

void DBDfxAdapter::FinishTrace()
{
}

void DBDfxAdapter::StartAsyncTrace(const std::string &action, int32_t taskId)
{
    (void) action;
    (void) taskId;
}

void DBDfxAdapter::FinishAsyncTrace(const std::string &action, int32_t taskId)
{
    (void) action;
    (void) taskId;
}

void DBDfxAdapter::StartTracing()
{
}

void DBDfxAdapter::FinishTracing()
{
}
#endif
} // namespace DistributedDB
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

#include "log_print.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <mutex>

#include "securec.h"
#include "platform_specific.h"
#include "hilog/log.h"

namespace DistributedDB {
Logger *Logger::logHandler = nullptr;
const std::string Logger::PRIVATE_TAG = "s{private}";

class HiLogger : public Logger {
public:
    void Print(Level level, const std::string &tag, const std::string &msg) override
    {
        if (msg.empty()) {
            return;
        }
#ifdef DB_DEBUG_ENV
#define FORMAT "%s"
#else
#define FORMAT "%{public}s"
#endif
        OHOS::HiviewDFX::HiLogLabel label = { LOG_CORE, 0xD001630, tag.c_str() }; // log module id.
        switch (level) {
            case Level::LEVEL_DEBUG:
                (void)HILOG_IMPL(label.type, LOG_DEBUG, label.domain, label.tag, FORMAT, msg.c_str());
                break;
            case Level::LEVEL_INFO:
                (void)HILOG_IMPL(label.type, LOG_INFO, label.domain, label.tag, FORMAT, msg.c_str());
                break;
            case Level::LEVEL_WARN:
                (void)HILOG_IMPL(label.type, LOG_WARN, label.domain, label.tag, FORMAT, msg.c_str());
                break;
            case Level::LEVEL_ERROR:
                (void)HILOG_IMPL(label.type, LOG_ERROR, label.domain, label.tag, FORMAT, msg.c_str());
                break;
            case Level::LEVEL_FATAL:
                (void)HILOG_IMPL(label.type, LOG_FATAL, label.domain, label.tag, FORMAT, msg.c_str());
                break;
            default:
                break;
        }
    }
};

Logger *Logger::GetInstance()
{
    static std::mutex logInstanceLock;
    static std::atomic<Logger *> logInstance = nullptr;
    // For Double-Checked Locking, we need check logInstance twice
    if (logInstance == nullptr) {
        std::lock_guard<std::mutex> lock(logInstanceLock);
        if (logInstance == nullptr) {
            // Here, we new logInstance to print log, if new failed, we can do nothing.
            logInstance = new (std::nothrow) HiLogger;
        }
    }
    return logInstance;
}

void Logger::RegisterLogger(Logger *logger)
{
    static std::mutex logHandlerLock;
    if (logger == nullptr) {
        return;
    }
    if (logHandler == nullptr) {
        std::lock_guard<std::mutex> lock(logHandlerLock);
        if (logHandler == nullptr) {
            logHandler = logger;
        }
    }
}

void Logger::Log(Level level, const std::string &tag, const char *func, int line, const char *format, ...)
{
    (void)func;
    (void)line;
    if (format == nullptr) {
        return;
    }

    static const int maxLogLength = 1024;
    va_list argList;
    va_start(argList, format);
    char logBuff[maxLogLength];
    std::string msg;
    std::string formatTemp;
    PreparePrivateLog(format, formatTemp);
    int bytes = vsnprintf_s(logBuff, maxLogLength, maxLogLength - 1, formatTemp.c_str(), argList);
    if (bytes < 0) {
        msg = "log buffer overflow!";
    } else {
        msg = logBuff;
    }
    va_end(argList);
    if (logHandler != nullptr) {
        logHandler->Print(level, tag, msg);
        return;
    }

    Logger::RegisterLogger(Logger::GetInstance());
    if (logHandler != nullptr) {
        logHandler->Print(level, tag, msg);
    }
}

void Logger::PreparePrivateLog(const char *format, std::string &outStrFormat)
{
    outStrFormat = format;
    std::string::size_type pos = outStrFormat.find(PRIVATE_TAG);
    if (pos != std::string::npos) {
#ifdef DB_DEBUG_ENV
        outStrFormat.replace(pos, PRIVATE_TAG.size(), "s");
#else
        outStrFormat.replace(pos, PRIVATE_TAG.size(), ".3s");
#endif
    }
}
} // namespace DistributedDB

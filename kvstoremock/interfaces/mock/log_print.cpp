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

#include "log_print.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <mutex>

#include "securec.h"

Logger *Logger::logHandler = nullptr;
const std::string Logger::PRIVATE_TAG = "{private}";
const std::string Logger::PUBLIC_TAG = "{public}";

void Logger::Print(Level level,  const char *func, int line, const std::string &tag, const std::string &msg)
{
    printf("%d %s:%d [%s]:%s \n", level, func, line, tag.c_str(), msg.c_str());
}

Logger *Logger::GetInstance()
{
    static Logger logger;
    return &logger;
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
        logHandler->Print(level, func, line, tag, msg);
        return;
    }

    Logger::RegisterLogger(Logger::GetInstance());
    if (logHandler != nullptr) {
        logHandler->Print(level, func, line, tag, msg);
    }
}

void Logger::PreparePrivateLog(const char *format, std::string &outStrFormat)
{
    outStrFormat = format;
    std::string::size_type pos;
    while ((pos = outStrFormat.find(PRIVATE_TAG)) != std::string::npos) {
        outStrFormat.replace(pos, PRIVATE_TAG.size(), "");
    }

    while ((pos = outStrFormat.find(PUBLIC_TAG)) != std::string::npos) {
        outStrFormat.replace(pos, PUBLIC_TAG.size(), "");
    }
}

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
#include "performance_analysis.h"

#include <climits>

#include "db_errno.h"
#include "log_print.h"
#include "macro_utils.h"
#include "platform_specific.h"
#include "time_helper.h"

namespace DistributedDB {
const std::string PerformanceAnalysis::STATISTICAL_DATA_FILE_NAME_HEADER = "/data/log/statistic";
const std::string PerformanceAnalysis::CSV_FILE_EXTENSION = ".csv";
const std::string PerformanceAnalysis::DEFAULT_FILE_NAME = "default00";
std::once_flag PerformanceAnalysis::initFlag_;

PerformanceAnalysis *PerformanceAnalysis::GetInstance(int stepNum)
{
    static PerformanceAnalysis inst(stepNum);
    std::call_once(initFlag_, [] { inst.Initialization(); });
    return &inst;
}

PerformanceAnalysis::PerformanceAnalysis(uint32_t inStepNum)
    : isOpen_(false)
{
    stepNum_ = inStepNum;
    fileNumber_ = 0;
    fileName_ = std::string(DEFAULT_FILE_NAME) + std::to_string(fileNumber_);
}

void PerformanceAnalysis::Initialization()
{
    counts_.resize(stepNum_);
    timeRecordData_.timeInfo.resize(stepNum_);
    stepTimeRecordInfo_.resize(stepNum_);
    for (auto &stepIter : stepTimeRecordInfo_) {
        stepIter.max = 0;
        stepIter.min = ULLONG_MAX;
        stepIter.average = 0;
    }
    for (auto iter = counts_.begin(); iter != counts_.end(); ++iter) {
        *iter = 0;
    }
}

PerformanceAnalysis::~PerformanceAnalysis() {};

bool PerformanceAnalysis::IsStepValid(uint32_t step) const
{
    return (stepNum_ < MAX_TIMERECORD_STEP_NUM && step < stepNum_);
}

bool PerformanceAnalysis::IsOpen() const
{
    return isOpen_;
}

void PerformanceAnalysis::OpenPerformanceAnalysis()
{
    isOpen_ = true;
}

void PerformanceAnalysis::ClosePerformanceAnalysis()
{
    isOpen_ = false;
}

bool PerformanceAnalysis::InsertTimeRecord(const TimePair &timePair, uint32_t step)
{
    if (!IsStepValid(step)) { // LCOV_EXCL_BR_LINE
        return false;
    }
    timeRecordData_.timeInfo[step] = timePair;
    return true;
}

bool PerformanceAnalysis::GetTimeRecord(uint32_t step, TimePair &timePair) const
{
    if (!IsStepValid(step)) { // LCOV_EXCL_BR_LINE
        return false;
    }
    timePair = timeRecordData_.timeInfo[step];
    return true;
}

void PerformanceAnalysis::TimeRecordStart()
{
    if (!IsOpen()) { // LCOV_EXCL_BR_LINE
        return;
    }
    StepTimeRecordStart(0);
}

void PerformanceAnalysis::TimeRecordEnd()
{
    if (!IsOpen()) { // LCOV_EXCL_BR_LINE
        return;
    }
    StepTimeRecordEnd(0);
}

void PerformanceAnalysis::StepTimeRecordStart(uint32_t step)
{
    if (!IsOpen()) {
        return;
    }
    if (!IsStepValid(step)) { // LCOV_EXCL_BR_LINE
        return;
    }
    TimePair timePair = {0, 0};
    uint64_t curTime = 0;
    int errCode = OS::GetCurrentSysTimeInMicrosecond(curTime);
    if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
        LOGE("[performance_analysis] GetCurrentSysTimeInMicrosecond fail");
    } else {
        timePair.startTime = curTime;
        LOGD("[performance_analysis] StepTimeRecordStart step:%" PRIu32 ", curTime:%" PRIu64, step, curTime);
        (void)InsertTimeRecord(timePair, step);
    }
}

void PerformanceAnalysis::StepTimeRecordEnd(uint32_t step)
{
    if (!IsOpen()) {
        return;
    }
    if (!IsStepValid(step)) { // LCOV_EXCL_BR_LINE
        return;
    }
    TimePair timePair = {0, 0};
    bool errCode = GetTimeRecord(step, timePair);
    if (!errCode) { // LCOV_EXCL_BR_LINE
        return;
    }
    (void)InsertTimeRecord({0, 0}, step);

    uint64_t curTime = 0;
    (void)OS::GetCurrentSysTimeInMicrosecond(curTime);
    timePair.endTime = curTime;
    LOGD("[performance_analysis] StepTimeRecordEnd step:%" PRIu32 ", curTime:%" PRIu64, step, curTime);

    if ((timePair.endTime < timePair.startTime) || (timePair.startTime == 0) ||
        (timePair.endTime == 0)) { // LCOV_EXCL_BR_LINE
        return;
    }
    Timestamp offset = timePair.endTime - timePair.startTime;
    if (stepTimeRecordInfo_[step].max < offset) { // LCOV_EXCL_BR_LINE
        stepTimeRecordInfo_[step].max = offset;
    }
    if (offset < stepTimeRecordInfo_[step].min) { // LCOV_EXCL_BR_LINE
        stepTimeRecordInfo_[step].min = offset;
    }
    counts_[step]++;
    if (counts_[step] == 0) { // LCOV_EXCL_BR_LINE
        stepTimeRecordInfo_[step].average = 0;
        return;
    }
    stepTimeRecordInfo_[step].average += (static_cast<float>(offset) -
        stepTimeRecordInfo_[step].average) / counts_[step];
}

std::string PerformanceAnalysis::GetStatistics()
{
    std::string result;
    for (size_t i = 0; i < stepTimeRecordInfo_.size(); i++) { // LCOV_EXCL_BR_LINE
        if (stepTimeRecordInfo_[i].max != 0) {
            result += "\nstep : " + std::to_string(i) + "\n";
            result += "max:                 " + std::to_string(stepTimeRecordInfo_[i].max) + "\n";
            result += "min:                 " + std::to_string(stepTimeRecordInfo_[i].min) + "\n";
            result += "average:             " +
                std::to_string(static_cast<uint64_t>(stepTimeRecordInfo_[i].average)) + "\n";
            result += "count:               " + std::to_string(counts_[i]) + "\n";
        }
    }
    OutStatistics();
    Clear();
    return result;
}

void PerformanceAnalysis::OutStatistics()
{
    std::string addrStatistics = STATISTICAL_DATA_FILE_NAME_HEADER + fileName_ + CSV_FILE_EXTENSION;
    outFile.open(addrStatistics, std::ios_base::app);
    if (!outFile.is_open()) {
        return;
    }
    // This part filters the zeros data
    outFile << "stepNum, maxTime(us), minTime(us), averageTime(us), count,\n";
    for (size_t i = 0; i < stepTimeRecordInfo_.size(); i++) { // output to performance file
        if (stepTimeRecordInfo_[i].max != 0) { // LCOV_EXCL_BR_LINE
            outFile << i << "," << stepTimeRecordInfo_[i].max<< "," << stepTimeRecordInfo_[i].min
                << "," << stepTimeRecordInfo_[i].average << "," << counts_[i] << "," << "\n";
        }
    }
    LOGD("outFile success and exit!");
    outFile.close();
}

void PerformanceAnalysis::Clear()
{
    counts_.clear();
    timeRecordData_.timeInfo.clear();
    stepTimeRecordInfo_.clear();
    counts_.resize(stepNum_);
    timeRecordData_.timeInfo.resize(stepNum_);
    stepTimeRecordInfo_.resize(stepNum_);
    for (auto &iter : stepTimeRecordInfo_) {
        iter.max = 0;
        iter.min = ULLONG_MAX;
        iter.average = 0;
    }
    fileName_ = std::string(DEFAULT_FILE_NAME) + std::to_string(fileNumber_);
}

void PerformanceAnalysis::SetFileName(const std::string &fileName)
{
    fileName_ = fileName;
}
}  // namespace DistributedDB

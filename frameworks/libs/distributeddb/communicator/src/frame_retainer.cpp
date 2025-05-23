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

#include "frame_retainer.h"
#include "db_common.h"
#include "log_print.h"
#include "serial_buffer.h"

namespace DistributedDB {
namespace {
const uint32_t MAX_RETAIN_CAPACITY = 67108864; // 64 M bytes
const uint32_t MAX_RETAIN_TIME = 10; // 10 s
const uint32_t MAX_RETAIN_FRAME_SIZE = 33554432; // 32 M bytes
const uint32_t MAX_RETAIN_FRAME_PER_LABEL_PER_TARGET = 5; // Allow 5 frame per communicator per source target
const int RETAIN_SURVAIL_PERIOD_IN_MILLISECOND = 1000; // Period is 1 s
inline void LogRetainInfo(const std::string &logPrefix, const LabelType &label, const std::string &target,
    uint64_t order, const RetainWork &work)
{
    LOGI("%s : Label=%.3s, target=%s{private}, retainOrder=%" PRIu64 ", frameId=%" PRIu32 ", remainTime=%" PRIu32
        ", frameSize=%" PRIu32 ".", logPrefix.c_str(), VEC_TO_STR(label), target.c_str(), ULL(order),
        work.frameId, work.remainTime, work.buffer->GetSize());
}
}

void FrameRetainer::Initialize()
{
    RuntimeContext *context = RuntimeContext::GetInstance();
    if (context == nullptr) {
        return; // Never gonna happen, context always be valid.
    }
    TimerAction action = [this](TimerId inTimerId)->int {
        PeriodicalSurveillance();
        return E_OK;
    };
    int errCode = context->SetTimer(RETAIN_SURVAIL_PERIOD_IN_MILLISECOND, action, nullptr, timerId_);
    if (errCode != E_OK) {
        LOGE("[Retainer][Init] Set timer fail, errCode=%d.", errCode);
        return;
    }
    isTimerWork_ = true;
}

void FrameRetainer::Finalize()
{
    RuntimeContext *context = RuntimeContext::GetInstance();
    if (context == nullptr) {
        return; // Never gonna happen, context always be valid.
    }
    // First: Stop the timer
    if (isTimerWork_) {
        // After return, the timer rely no more on retainer.
        context->RemoveTimer(timerId_, true);
        isTimerWork_ = false;
    }
    // Second: Clear the retainWorkPool_
    for (auto &eachLabel : retainWorkPool_) {
        for (auto &eachTarget : eachLabel.second) {
            for (auto &eachFrame : eachTarget.second) {
                LogRetainInfo("[Retainer][Final] DISCARD", eachLabel.first, eachTarget.first, eachFrame.first,
                    eachFrame.second);
                delete eachFrame.second.buffer;
                eachFrame.second.buffer = nullptr;
            }
        }
    }
    retainWorkPool_.clear();
    totalSizeByByte_ = 0;
    totalRetainFrames_ = 0;
}

void FrameRetainer::RetainFrame(const FrameInfo &inFrame)
{
    if (inFrame.buffer == nullptr) {
        return; // Never gonna happen
    }
    RetainWork work{inFrame.buffer, inFrame.sendUser, inFrame.frameId, MAX_RETAIN_TIME};
    if (work.buffer->GetSize() > MAX_RETAIN_FRAME_SIZE) {
        LOGE("[Retainer][Retain] Frame size=%u over limit=%u.", work.buffer->GetSize(), MAX_RETAIN_FRAME_SIZE);
        delete work.buffer;
        work.buffer = nullptr;
        return;
    }
    int errCode = work.buffer->ConvertForCrossThread();
    if (errCode != E_OK) {
        LOGE("[Retainer][Retain] ConvertForCrossThread fail, errCode=%d.", errCode);
        delete work.buffer;
        work.buffer = nullptr;
        return;
    }

    std::lock_guard<std::mutex> overallLockGuard(overallMutex_);
    std::map<uint64_t, RetainWork> &perLabelPerTarget = retainWorkPool_[inFrame.commLabel][inFrame.srcTarget];
    if (perLabelPerTarget.size() >= MAX_RETAIN_FRAME_PER_LABEL_PER_TARGET) {
        // Discard the oldest and obsolete one, update the statistics, free the buffer and remove from the map
        auto iter = perLabelPerTarget.begin();
        LogRetainInfo("[Retainer][Retain] DISCARD", inFrame.commLabel, inFrame.srcTarget, iter->first, iter->second);
        totalSizeByByte_ -= iter->second.buffer->GetSize();
        totalRetainFrames_--;
        delete iter->second.buffer;
        iter->second.buffer = nullptr;
        perLabelPerTarget.erase(iter);
    }
    // Retain the new frame, update the statistics
    perLabelPerTarget[incRetainOrder_++] = work;
    totalSizeByByte_ += inFrame.buffer->GetSize();
    totalRetainFrames_++;
    // Discard obsolete frames until totalSize under capacity.
    DiscardObsoleteFramesIfNeed();
    // Display the final statistics
    LOGI("[Retainer][Retain] Order=%" PRIu64 ". Statistics: TOTAL_BYTE=%" PRIu32 ", TOTAL_FRAME=%" PRIu32 ".",
        ULL(incRetainOrder_ - 1), totalSizeByByte_, totalRetainFrames_);
}

std::list<FrameInfo> FrameRetainer::FetchFramesForSpecificCommunicator(const LabelType &inCommLabel)
{
    std::lock_guard<std::mutex> overallLockGuard(overallMutex_);
    std::list<FrameInfo> outFrameList;
    if (retainWorkPool_.count(inCommLabel) == 0) {
        return outFrameList;
    }
    auto &perLabel = retainWorkPool_[inCommLabel];
    std::map<uint64_t, std::string> fetchOrder;
    for (const auto &eachTarget : perLabel) {
        for (const auto &eachFrame : eachTarget.second) {
            fetchOrder[eachFrame.first] = eachTarget.first;
        }
    }
    for (auto &entry : fetchOrder) {
        RetainWork &work = perLabel[entry.second][entry.first];
        LogRetainInfo("[Retainer][Fetch] FETCH-OUT", inCommLabel, entry.second, entry.first, work);
        outFrameList.emplace_back(FrameInfo{work.buffer, entry.second, work.sendUser, inCommLabel, work.frameId});
        // Update statistics
        totalSizeByByte_ -= work.buffer->GetSize();
        totalRetainFrames_--;
    }
    retainWorkPool_.erase(inCommLabel);
    return outFrameList;
}

void FrameRetainer::DecreaseRemainTimeAndDiscard(const LabelType &label,
    std::pair<const std::string, std::map<uint64_t, RetainWork>> &eachTarget, std::set<uint64_t> &frameToDiscard)
{
    for (auto &eachFrame : eachTarget.second) {
        // Decrease remainTime and discard if need. The remainTime will not be zero before decrease.
        eachFrame.second.remainTime--;
        if (eachFrame.second.remainTime != 0) {
            continue;
        }
        LogRetainInfo("[Retainer][Surveil] DISCARD", label, eachTarget.first, eachFrame.first,
            eachFrame.second);
        totalSizeByByte_ -= eachFrame.second.buffer->GetSize();
        totalRetainFrames_--;
        // Free this retain work first
        delete eachFrame.second.buffer;
        eachFrame.second.buffer = nullptr;
        // Record this frame in discard list
        frameToDiscard.insert(eachFrame.first);
    }
}

void FrameRetainer::PeriodicalSurveillance()
{
    std::lock_guard<std::mutex> overallLockGuard(overallMutex_);
    // First: Discard overtime frames.
    for (auto &eachLabel : retainWorkPool_) {
        for (auto &eachTarget : eachLabel.second) {
            std::set<uint64_t> frameToDiscard;
            DecreaseRemainTimeAndDiscard(eachLabel.first, eachTarget, frameToDiscard);
            // Remove the retain work from frameMap.
            for (auto &entry : frameToDiscard) {
                eachTarget.second.erase(entry);
            }
        }
    }
    // Second: Shrink the retainWorkPool_
    ShrinkRetainWorkPool();
}

void FrameRetainer::DiscardObsoleteFramesIfNeed()
{
    if (totalSizeByByte_ <= MAX_RETAIN_CAPACITY) {
        return;
    }
    std::map<uint64_t, std::pair<LabelType, std::string>> discardOrder;
    // Sort all the frames by their retain order ascendingly
    for (const auto &eachLabel : retainWorkPool_) {
        for (const auto &eachTarget : eachLabel.second) {
            for (const auto &eachFrame : eachTarget.second) {
                discardOrder[eachFrame.first] = {eachLabel.first, eachTarget.first};
            }
        }
    }
    // Discard obsolete frames until totalSize under capacity.
    while (totalSizeByByte_ > MAX_RETAIN_CAPACITY) {
        if (discardOrder.empty()) { // Unlikely to happen
            LOGE("[Retainer][Discard] Internal Error: Byte=%" PRIu32 ", Frames=%" PRIu32 ".",
                totalSizeByByte_, totalRetainFrames_);
            return;
        }
        auto iter = discardOrder.begin();
        RetainWork &workRef = retainWorkPool_[iter->second.first][iter->second.second][iter->first];
        LogRetainInfo("[Retainer][Discard] DISCARD", iter->second.first, iter->second.second, iter->first, workRef);
        // Discard the oldest and obsolete one, update the statistics, free the buffer and remove from the map
        totalSizeByByte_ -= workRef.buffer->GetSize();
        totalRetainFrames_--;
        delete workRef.buffer;
        workRef.buffer = nullptr;
        retainWorkPool_[iter->second.first][iter->second.second].erase(iter->first);
        // Remove from the discardOrder
        discardOrder.erase(iter);
    }
    // Shrink the retainWorkPool_ to remove out empty node on the map
    ShrinkRetainWorkPool();
}

void FrameRetainer::ShrinkRetainWorkPool()
{
    std::set<LabelType> emptyLabel;
    for (auto &eachLabel : retainWorkPool_) {
        std::set<std::string> emptyTarget;
        for (auto &eachTarget : eachLabel.second) {
            // Record corresponding target if its frameMap empty.
            if (eachTarget.second.empty()) {
                emptyTarget.insert(eachTarget.first);
            }
        }
        // Remove the empty frameMap from the targetMap. Record corresponding label if its targetMap empty.
        for (auto &entry : emptyTarget) {
            eachLabel.second.erase(entry);
        }
        if (eachLabel.second.empty()) {
            emptyLabel.insert(eachLabel.first);
        }
    }
    // Remove the empty targetMap from retainWorkPool_
    for (auto &entry : emptyLabel) {
        retainWorkPool_.erase(entry);
    }
}
} // namespace DistributedDB
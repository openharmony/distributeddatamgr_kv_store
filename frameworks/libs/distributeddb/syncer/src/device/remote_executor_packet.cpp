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

#include "remote_executor_packet.h"

namespace DistributedDB {
namespace {
    constexpr uint8_t REQUEST_FLAG_RESPONSE_ACK = 1u;
    constexpr uint8_t ACK_FLAG_LAST_ACK = 1u;
    constexpr uint8_t ACK_FLAG_SECURITY_OPTION = 2u;
}

uint32_t RemoteExecutorRequestPacket::GetVersion() const
{
    return version_;
}

void RemoteExecutorRequestPacket::SetVersion(uint32_t version)
{
    version_ = version;
}

uint32_t RemoteExecutorRequestPacket::GetFlag() const
{
    return flag_;
}

void RemoteExecutorRequestPacket::SetFlag(uint32_t flag)
{
    flag_ = flag;
}

const PreparedStmt &RemoteExecutorRequestPacket::GetPreparedStmt() const
{
    return preparedStmt_;
}

bool RemoteExecutorRequestPacket::IsNeedResponse() const
{
    return (flag_ & REQUEST_FLAG_RESPONSE_ACK) != 0;
}

void RemoteExecutorRequestPacket::SetNeedResponse()
{
    flag_ |= REQUEST_FLAG_RESPONSE_ACK;
}

void RemoteExecutorRequestPacket::SetExtraConditions(const std::map<std::string, std::string> &extraConditions)
{
    extraConditions_ = extraConditions;
}

std::map<std::string, std::string> RemoteExecutorRequestPacket::GetExtraConditions() const
{
    return extraConditions_;
}

uint32_t RemoteExecutorRequestPacket::CalculateLen() const
{
    uint32_t len = Parcel::GetUInt32Len(); // version
    len += Parcel::GetUInt32Len();  // flag
    uint32_t tmpLen = preparedStmt_.CalcLength();
    if ((len + tmpLen) > static_cast<uint32_t>(INT32_MAX) || tmpLen == 0u) {
        LOGE("[RemoteExecutorRequestPacket][CalculateLen] Prepared statement is too large");
        return 0;
    }
    len += tmpLen;
    len += Parcel::GetUInt32Len(); // conditions count
    for (const auto &entry : extraConditions_) {
        // each condition len never greater than 256
        len += Parcel::GetStringLen(entry.first);
        len += Parcel::GetStringLen(entry.second);
        if (len > static_cast<uint32_t>(INT32_MAX)) {
            LOGE("[RemoteExecutorRequestPacket][CalculateLen] conditions is too large");
            return 0;
        }
    }
    len = Parcel::GetEightByteAlign(len); // 8-byte align
    len += Parcel::GetIntLen();
    return len;
}

int RemoteExecutorRequestPacket::Serialization(Parcel &parcel) const
{
    (void) parcel.WriteUInt32(version_);
    (void) parcel.WriteUInt32(flag_);
    (void) preparedStmt_.Serialize(parcel);
    if (parcel.IsError()) {
        LOGE("[RemoteExecutorRequestPacket] Serialization failed");
        return -E_INVALID_ARGS;
    }
    if (extraConditions_.size() > DBConstant::MAX_CONDITION_COUNT) {
        LOGE("[RemoteExecutorRequestPacket] Serialization failed with too much condition");
        return -E_INVALID_ARGS;
    }
    parcel.WriteUInt32(static_cast<uint32_t>(extraConditions_.size()));
    for (const auto &entry : extraConditions_) {
        if (entry.first.length() > DBConstant::MAX_CONDITION_KEY_LEN ||
            entry.second.length() > DBConstant::MAX_CONDITION_VALUE_LEN) {
            LOGE("[RemoteExecutorRequestPacket] Serialization failed with too long key or value");
            return -E_INVALID_ARGS;
        }
        parcel.WriteString(entry.first);
        parcel.WriteString(entry.second);
    }
    parcel.EightByteAlign();
    parcel.WriteInt(secLabel_);
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int RemoteExecutorRequestPacket::DeSerialization(Parcel &parcel)
{
    (void) parcel.ReadUInt32(version_);
    (void) parcel.ReadUInt32(flag_);
    (void) preparedStmt_.DeSerialize(parcel);
    if (parcel.IsError()) {
        LOGE("[RemoteExecutorRequestPacket] DeSerialization failed");
        return -E_INVALID_ARGS;
    }
    if (version_ < REQUEST_PACKET_VERSION_V2) {
        return E_OK;
    }
    uint32_t conditionSize = 0u;
    (void) parcel.ReadUInt32(conditionSize);
    if (conditionSize > DBConstant::MAX_CONDITION_COUNT) {
        return -E_INVALID_ARGS;
    }
    for (uint32_t i = 0; i < conditionSize; i++) {
        std::string conditionKey;
        std::string conditionVal;
        (void) parcel.ReadString(conditionKey);
        (void) parcel.ReadString(conditionVal);
        if (conditionKey.length() > DBConstant::MAX_CONDITION_KEY_LEN ||
            conditionVal.length() > DBConstant::MAX_CONDITION_VALUE_LEN) {
            return -E_INVALID_ARGS;
        }
        extraConditions_[conditionKey] = conditionVal;
    }
    parcel.EightByteAlign();
    if (version_ >= REQUEST_PACKET_VERSION_V3) {
        parcel.ReadInt(secLabel_);
    }
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

void RemoteExecutorRequestPacket::SetOpCode(PreparedStmt::ExecutorOperation opCode)
{
    preparedStmt_.SetOpCode(opCode);
}

void RemoteExecutorRequestPacket::SetSql(const std::string &sql)
{
    preparedStmt_.SetSql(sql);
}

void RemoteExecutorRequestPacket::SetBindArgs(const std::vector<std::string> &bindArgs)
{
    preparedStmt_.SetBindArgs(bindArgs);
}

void RemoteExecutorRequestPacket::SetSecLabel(int32_t secLabel)
{
    secLabel_ = secLabel;
}

int32_t RemoteExecutorRequestPacket::GetSecLabel() const
{
    return secLabel_;
}

RemoteExecutorRequestPacket* RemoteExecutorRequestPacket::Create()
{
    return new (std::nothrow) RemoteExecutorRequestPacket();
}

void RemoteExecutorRequestPacket::Release(RemoteExecutorRequestPacket *&packet)
{
    delete packet;
    packet = nullptr;
}

uint32_t RemoteExecutorAckPacket::GetVersion() const
{
    return version_;
}

void RemoteExecutorAckPacket::SetVersion(uint32_t version)
{
    version_ = version;
}

uint32_t RemoteExecutorAckPacket::GetFlag() const
{
    return flag_;
}

void RemoteExecutorAckPacket::SetFlag(uint32_t flag)
{
    flag_ = flag;
}

int32_t RemoteExecutorAckPacket::GetAckCode() const
{
    return ackCode_;
}

void RemoteExecutorAckPacket::SetAckCode(int32_t ackCode)
{
    ackCode_ = ackCode;
}

void RemoteExecutorAckPacket::MoveInRowDataSet(RelationalRowDataSet &&rowDataSet)
{
    rowDataSet_ = std::move(rowDataSet);
}

RelationalRowDataSet &&RemoteExecutorAckPacket::MoveOutRowDataSet() const
{
    return std::move(rowDataSet_);
}

bool RemoteExecutorAckPacket::IsLastAck() const
{
    return (flag_ & ACK_FLAG_LAST_ACK) != 0;
}

void RemoteExecutorAckPacket::SetLastAck()
{
    flag_ |= ACK_FLAG_LAST_ACK;
}

uint32_t RemoteExecutorAckPacket::CalculateLen() const
{
    uint32_t len = Parcel::GetUInt32Len(); // version
    len += Parcel::GetIntLen();    // ackCode
    len += Parcel::GetUInt32Len();  // flag
    len = Parcel::GetEightByteAlign(len);
    len += static_cast<uint32_t>(rowDataSet_.CalcLength());
    len += Parcel::GetIntLen(); // secLabel
    len += Parcel::GetIntLen(); // secFlag
    return len;
}

int RemoteExecutorAckPacket::Serialization(Parcel &parcel) const
{
    (void) parcel.WriteUInt32(version_);
    (void) parcel.WriteInt(ackCode_);
    (void) parcel.WriteUInt32(flag_);
    parcel.EightByteAlign();
    (void) rowDataSet_.Serialize(parcel);
    (void) parcel.WriteInt(secLabel_);
    (void) parcel.WriteInt(secFlag_);
    if (parcel.IsError()) {
        LOGE("[RemoteExecutorAckPacket] Serialization failed");
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int RemoteExecutorAckPacket::DeSerialization(Parcel &parcel)
{
    (void) parcel.ReadUInt32(version_);
    (void) parcel.ReadInt(ackCode_);
    (void) parcel.ReadUInt32(flag_);
    parcel.EightByteAlign();
    if (parcel.IsError()) {
        LOGE("[RemoteExecutorAckPacket] DeSerialization failed");
        return -E_INVALID_ARGS;
    }
    int errCode = rowDataSet_.DeSerialize(parcel);
    if (errCode != E_OK) {
        return errCode;
    }
    if ((flag_ & ACK_FLAG_SECURITY_OPTION) != 0) {
        (void) parcel.ReadInt(secLabel_);
        (void) parcel.ReadInt(secFlag_);
    } else {
        secLabel_ = NOT_SUPPORT_SEC_CLASSIFICATION;
    }
    if (parcel.IsError()) {
        LOGE("[RemoteExecutorAckPacket] DeSerialization failed");
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

SecurityOption RemoteExecutorAckPacket::GetSecurityOption() const
{
    SecurityOption option = {secLabel_, secFlag_};
    return option;
}

void RemoteExecutorAckPacket::SetSecurityOption(const SecurityOption &option)
{
    secLabel_ = option.securityLabel;
    secFlag_ = option.securityFlag;
    flag_ |= ACK_FLAG_SECURITY_OPTION;
}
}
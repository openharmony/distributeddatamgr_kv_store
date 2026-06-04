/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef HISYSEVENT_INTERFACES_NATIVE_INNERKITS_EASY_ENCODER_H
#define HISYSEVENT_INTERFACES_NATIVE_INNERKITS_EASY_ENCODER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)
struct HiSysEventEasyParamValueType {
    uint8_t isArray : 1;
    uint8_t valueType : 4;
    uint8_t valueByteCnt : 3;
};
#pragma pack()

/**
 * @brief Encode a value type into raw data then append to it to event
 *
 * @param data       encoded raw data of event
 * @param dataLen    total length of the data to append value type
 * @param offset     position to append value type
 * @param valueType  value type
 * @return 0 means success, others means failure
 */
int EncodeValueType(uint8_t* data, const size_t dataLen, size_t* offset,
    const struct HiSysEventEasyParamValueType* valueType);


/**
 * @brief Encode a string into raw data then append to it to event
 *
 * @param data     encoded raw data of event
 * @param dataLen  total length of the data to append string value
 * @param offset   position to append string
 * @param content  string content
 * @return 0 means success, others means failure
 */
int EncodeStringValue(uint8_t* data, const size_t dataLen, size_t* offset, const char* content);

#ifdef __cplusplus
}
#endif
#endif // HISYSEVENT_INTERFACES_NATIVE_INNERKITS_EASY_ENCODER_H

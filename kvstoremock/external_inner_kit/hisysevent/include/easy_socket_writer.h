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

#ifndef HISYSEVENT_INTERFACES_NATIVE_INNERKITS_SOCKET_WRITER_H
#define HISYSEVENT_INTERFACES_NATIVE_INNERKITS_SOCKET_WRITER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write event to socket
 *
 * @param data     encoded raw data of sysevent
 * @param dataLen  total length of the data to write
 * @return 0 means success, others means failure.
 */
int Write(const uint8_t* data, const size_t dataLen);

#ifdef __cplusplus
}
#endif
#endif // HISYSEVENT_INTERFACES_NATIVE_INNERKITS_SOCKET_WRITER_H

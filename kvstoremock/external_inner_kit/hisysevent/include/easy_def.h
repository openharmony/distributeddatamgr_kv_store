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

#ifndef HISYSEVENT_INTERFACES_NATIVE_INNERKITS_HISYSEVENT_EASY_DEF_H
#define HISYSEVENT_INTERFACES_NATIVE_INNERKITS_HISYSEVENT_EASY_DEF_H

#define EVENT_BUFF_LEN                   (1120)

#define SUCCESS                          (0)
#define ERR_DOMAIN_INVALID               (-1)
#define ERR_NAME_INVALID                 (-2)
#define ERR_TYPE_INVALID                 (-3)
#define ERR_PARAM_VALUE_INVALID          (-4)
#define ERR_EVENT_BUF_INVALID            (-5)
#define ERR_ENCODE_STR_FAILED            (-6)
#define ERR_ENCODE_VALUE_TYPE_FAILED     (-7)
#define ERR_MEM_OPT_FAILED               (-8)
#define ERR_INIT_SOCKET_FAILED           (-9)
#define ERR_SET_SOCKET_OPT_FAILED        (-10)
#define ERR_SOCKET_ADDR_INVALID          (-11)

#define ERR_SOCKET_SEND_ERROR_BASE       (-1000)

#define MAX_DOMAIN_LENGTH                (16)
#define MAX_EVENT_NAME_LENGTH            (32)

#endif // HISYSEVENT_INTERFACES_NATIVE_INNERKITS_HISYSEVENT_EASY_DEF_H

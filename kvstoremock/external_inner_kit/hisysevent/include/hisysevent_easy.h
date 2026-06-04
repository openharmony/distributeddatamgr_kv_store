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

#ifndef HISYSEVENT_INTERFACES_NATIVE_INNERKITS_HISYSEVENT_EASY_H
#define HISYSEVENT_INTERFACES_NATIVE_INNERKITS_HISYSEVENT_EASY_H

#ifdef __cplusplus
extern "C" {
#endif

enum HiSysEventEasyType {
    EASY_EVENT_TYPE_FAULT = 1,
    EASY_EVENT_TYPE_STATISTIC,
    EASY_EVENT_TYPE_SECURITY,
    EASY_EVENT_TYPE_BEHAVIOR,
};

/**
 * @brief Easy writing sys event
 *
 * @param domain  event domain
 * @param name    event name
 * @param eventType event type of the event
 * @param data customized param data to write
 * @return 0 means success, others means failure.
 */
int HiSysEventEasyWrite(const char* domain, const char* name, enum HiSysEventEasyType eventType, const char* data);

#define OH_HiSysEvent_Easy_Write(domain, name, eventType, data) \
({ \
    int hiSysEventEsayWriteRet2024___ = HiSysEventEasyWrite(domain, name, eventType, data); \
    hiSysEventEsayWriteRet2024___; \
})

#ifdef __cplusplus
}
#endif
#endif // HISYSEVENT_INTERFACES_NATIVE_INNERKITS_HISYSEVENT_EASY_H

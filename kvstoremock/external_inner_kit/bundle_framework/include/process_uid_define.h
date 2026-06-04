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

#ifndef PROCESS_UID_DEFINE_H
#define PROCESS_UID_DEFINE_H

// UID/ProcessID separation threshold (200000) for seccomp policy parsing, avoiding name
// collisions with system macros.
#define BASE_USER_RANGE_FOR_NWEB (200000)

// Render Process
// Corresponding ProcessID range: RENDER_PROCESS_ISOLATION_START_ID (100000) ~
// RENDER_PROCESS_ISOLATION_END_ID (109999)
// Note: This ProcessID range is strictly within [0, BASE_USER_RANGE_FOR_NWEB-1] (0~199999)
// to comply with the UID composition rule. Formula application example (UserID=100):
// - Start UID = UserID × BASE_USER_RANGE_FOR_NWEB + RENDER_PROCESS_ISOLATION_START_ID
//               = 100 × 200000 + 100000 = 20100000
// - End UID = UserID × BASE_USER_RANGE_FOR_NWEB + RENDER_PROCESS_ISOLATION_END_ID
//             = 100 × 200000 + 109999 = 20109999
// Explanation: The 200000 (BASE_USER_RANGE_FOR_NWEB) in the formula is the core separator—its
// value is chosen to reserve sufficient space for ProcessIDs (up to 109999 unique ProcessIDs
// per user) while keeping UID values manageable.
#define START_ID_FOR_RENDER_PROCESS_ISOLATION (100000)
#define END_ID_FOR_RENDER_PROCESS_ISOLATION (109999)

#endif  // PROCESS_UID_DEFINE_H
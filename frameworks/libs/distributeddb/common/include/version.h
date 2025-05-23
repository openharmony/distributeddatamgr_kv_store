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

#ifndef VERSION_H
#define VERSION_H

#include <string>
#include <cstdint>

namespace DistributedDB {
// Version Regulation:
// Module version is always equal to the max version of its submodule.
// If a module or submodule upgrade to higher version, DO NOT simply increase current version by 1.
//      First: you have to preserve current version by renaming it as a historical version.
//      Second: Update the current version to the version to be release.
//      Finally: Update its parent module's version if exist.
// Why we update the current version to the version to be release? For example, if module A has submodule B and C,
// if now version of B is 105, and C is 101, thus version of A is 105; if now release version is 106 and we upgrade
// submodule C, if we simply change version of C to 102 then version of A is still 105, but if we change version of C
// to 106 then version of A is now 106, so we can know that something had changed for module A.
constexpr const char *SOFTWARE_VERSION_STRING = "1.1.5"; // DistributedDB current version string.
constexpr const uint32_t SOFTWARE_VERSION_BASE = 100; // Software version base value, do not change it
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_1_0 = SOFTWARE_VERSION_BASE + 1; // 1 for first released version
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_2_0 = SOFTWARE_VERSION_BASE + 2; // 2 for second released version
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_3_0 = SOFTWARE_VERSION_BASE + 3; // 3 for third released version
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_4_0 = SOFTWARE_VERSION_BASE + 4; // 4 for fourth released version
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_5_0 = SOFTWARE_VERSION_BASE + 5; // 5 for fifth released version
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_6_0 = SOFTWARE_VERSION_BASE + 6; // 6 for sixth released version
// check both device has security label at 107
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_7_0 = SOFTWARE_VERSION_BASE + 7; // 7 for seventh released version
// kv ability sync with 2 packet at 108
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_8_0 = SOFTWARE_VERSION_BASE + 8; // 8 for eighth released version
// 109 version add field in request packet
// add schema version in ability request
// add schema version, system time offset, sender local time offset in data request
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_9_0 = SOFTWARE_VERSION_BASE + 9; // 9 for ninth released version
// 110 version add totalDataCount field in DataRequestPkt
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_10_0 = SOFTWARE_VERSION_BASE + 10; // 10 for tenth released version
// 111 version record remote version and rdb sync with config column
constexpr const uint32_t SOFTWARE_VERSION_RELEASE_11_0 = SOFTWARE_VERSION_BASE + 11; // 11 for tenth released version
constexpr const uint32_t SOFTWARE_VERSION_EARLIEST = SOFTWARE_VERSION_RELEASE_1_0;
constexpr const uint32_t SOFTWARE_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_11_0;
constexpr const int VERSION_INVALID = INT32_MAX;

// Storage Related Version
// LocalNaturalStore Related Version
constexpr const int LOCAL_STORE_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_1_0;
// SingleVerNaturalStore Related Version
constexpr const int SINGLE_VER_STORE_VERSION_V1 = SOFTWARE_VERSION_RELEASE_1_0;
constexpr const int SINGLE_VER_STORE_VERSION_V2 = SOFTWARE_VERSION_RELEASE_2_0;
constexpr const int SINGLE_VER_STORE_VERSION_V3 = SOFTWARE_VERSION_RELEASE_3_0;
// 104 add modify time and create time into sync_data
constexpr const int SINGLE_VER_STORE_VERSION_V4 = SOFTWARE_VERSION_RELEASE_4_0;
constexpr const int SINGLE_VER_STORE_VERSION_CURRENT = SINGLE_VER_STORE_VERSION_V4;
// MultiVerNaturalStore Related Version
constexpr const uint32_t VERSION_FILE_VERSION_CURRENT = 1;
constexpr const uint32_t MULTI_VER_STORE_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_1_0;
constexpr const int MULTI_VER_COMMIT_STORAGE_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_1_0;
constexpr const int MULTI_VER_DATA_STORAGE_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_1_0;
constexpr const int MULTI_VER_METADATA_STORAGE_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_1_0;
constexpr const int MULTI_VER_VALUESLICE_STORAGE_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_1_0;

// Syncer Related Version
constexpr const int TIME_SYNC_VERSION_V1 = SOFTWARE_VERSION_RELEASE_1_0; // time sync proctol added in version 101.
// Ability sync proctol added in version 102.
constexpr const int ABILITY_SYNC_VERSION_V1 = SOFTWARE_VERSION_RELEASE_2_0;
constexpr const uint32_t SINGLE_VER_SYNC_PROCTOL_V1 = SOFTWARE_VERSION_RELEASE_1_0; // The 1st version num
constexpr const uint32_t SINGLE_VER_SYNC_PROCTOL_V2 = SOFTWARE_VERSION_RELEASE_2_0; // The 2nd version num
constexpr const uint32_t SINGLE_VER_SYNC_PROCTOL_V3 = SOFTWARE_VERSION_RELEASE_3_0; // The third version num

// Local Meta Data Version
constexpr const uint32_t LOCAL_META_DATA_VERSION_V1 = SOFTWARE_VERSION_RELEASE_8_0;
constexpr const uint32_t LOCAL_META_DATA_VERSION_V2 = SOFTWARE_VERSION_RELEASE_9_0;
} // namespace DistributedDB

#endif // VERSION_H
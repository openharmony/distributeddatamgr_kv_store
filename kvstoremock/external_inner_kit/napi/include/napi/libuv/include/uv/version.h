/**
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

#ifndef UV_VERSION_H
#define UV_VERSION_H

 /*
 * Versions with the same major number are ABI stable. API is allowed to
 * evolve between minor releases, but only in a backwards compatible way.
 * Make sure you update the -soname directives in configure.ac
 * whenever you bump UV_VERSION_MAJOR or UV_VERSION_MINOR (but
 * not UV_VERSION_PATCH.)
 */

#define UV_VERSION_MAJOR 1
#define UV_VERSION_MINOR 52
#define UV_VERSION_PATCH 0
#define UV_VERSION_IS_RELEASE 1
#define UV_VERSION_SUFFIX ""

#define UV_VERSION_HEX  ((UV_VERSION_MAJOR << 16) | \
                         (UV_VERSION_MINOR <<  8) | \
                         (UV_VERSION_PATCH))

#endif /* UV_VERSION_H */

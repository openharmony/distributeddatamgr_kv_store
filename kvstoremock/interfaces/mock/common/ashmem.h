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

/**
 * @file ashmem.h
 *
 * @brief Provides the <b>Ashmem</b> class implemented in c_utils to operate the
 * Anonymous Shared Memory (Ashmem).
 */

#ifndef DISTRIBUTED_KVSTORE_ASHMEM_H
#define DISTRIBUTED_KVSTORE_ASHMEM_H

#include <cstddef>
#ifndef IOS_PLATFORM
#include <linux/ashmem.h>
#endif
#ifdef UTILS_CXX_RUST
#include <memory>
#endif
#include "refbase.h"

namespace OHOS {
/**
 * @brief Provides the <b>Ashmem</b> class implemented in c_utils to
 * operate the Anonymous Shared Memory (Ashmem).
 *
 * You can use the interfaces in this class to create <b>Ashmem</b> regions
 * and map them to implement write and read operations.
 *
 * @note <b>Ashmem</b> regions should be unmapped and closed manually,
 * though managed by a smart pointer.
 */
class Ashmem : public virtual RefBase {
public:
    /**
     * @brief Construct a new Ashmem object.
     *
     * @param fd File descriptor of an ashmem in kenrel.
     * @param size Size of the corresponding ashmem region in kernel.
     */
    Ashmem(int fd, int32_t size) {}
    ~Ashmem() override {}
};
} // namespace OHOS
#endif // DISTRIBUTED_KVSTORE_ASHMEM_H

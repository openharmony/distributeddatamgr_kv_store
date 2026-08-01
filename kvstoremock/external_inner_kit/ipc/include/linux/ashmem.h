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
 * @file linux/ashmem.h
 *
 * @brief Compile-time placeholder for the ashmem ioctl command definitions,
 * co-located with the mock IPC headers. <ashmem.h> from c_utils (included by the
 * mock message_parcel.h) unconditionally includes <linux/ashmem.h>, which is not
 * provided by the host kernel. Any target that puts ipc/include on its include
 * path (the mock IPC and, on host, the real kv_store code that uses the mock
 * IPC) therefore resolves <linux/ashmem.h> to this shim automatically.
 *
 * The mock layer never invokes the ashmem ioctls (WriteAshmem/ReadAshmem are
 * stubbed), so the macro values are never used at runtime; they exist only so
 * the code compiles. This directory is on the include path solely for non-ohos
 * (host/cross-platform) builds, so device builds keep using the real kernel
 * header.
 */

#ifndef KV_STORE_KVSTOREMOCK_LINUX_ASHMEM_H
#define KV_STORE_KVSTOREMOCK_LINUX_ASHMEM_H

#define ASHMEM_SET_NAME 0
#define ASHMEM_SET_SIZE 1
#define ASHMEM_SET_PROT_MASK 2
#define ASHMEM_GET_SIZE 3
#define ASHMEM_GET_PROT_MASK 4
#define ASHMEM_NAME_LEN 32

#endif // KV_STORE_RDBMOCK_LINUX_ASHMEM_H

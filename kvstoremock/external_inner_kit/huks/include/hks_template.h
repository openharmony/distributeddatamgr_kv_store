/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HKS_TEMPLATE_H
#define HKS_TEMPLATE_H

#include <errno.h>
#include <string.h>

#include "hks_log.h"

#undef HKS_NULL_POINTER

#ifdef __cplusplus
#define HKS_NULL_POINTER nullptr
#else
#define HKS_NULL_POINTER NULL
#endif

#define HKS_IF_NOT_SUCC_LOGE_RETURN(RESULT, ERROR_CODE, LOG_MESSAGE, ...) \
if ((RESULT) != HKS_SUCCESS) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    return (ERROR_CODE); \
}

#define HKS_IF_NOT_SUCC_LOGI_RETURN(RESULT, ERROR_CODE, LOG_MESSAGE, ...) \
if ((RESULT) != HKS_SUCCESS) { \
    HKS_LOG_I(LOG_MESSAGE, ##__VA_ARGS__); \
    return (ERROR_CODE); \
}

#define HKS_IF_NOT_SUCC_LOGE_BREAK(RESULT, LOG_MESSAGE, ...) \
if ((RESULT) != HKS_SUCCESS) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    break; \
}

#define HKS_IF_NOT_SUCC_LOGI_RETURN(RESULT, ERROR_CODE, LOG_MESSAGE, ...) \
if ((RESULT) != HKS_SUCCESS) { \
    HKS_LOG_I(LOG_MESSAGE, ##__VA_ARGS__); \
    return (ERROR_CODE); \
}

#define HKS_IF_NOT_SUCC_LOGI_BREAK(RESULT, LOG_MESSAGE, ...) \
if ((RESULT) != HKS_SUCCESS) { \
    HKS_LOG_I(LOG_MESSAGE, ##__VA_ARGS__); \
    break; \
}

#define HKS_IF_NOT_SUCC_BREAK(RESULT, ...) \
if ((RESULT) != HKS_SUCCESS) { \
    break; \
}

#define HKS_IF_NOT_SUCC_LOGE(RESULT, LOG_MESSAGE, ...) \
if ((RESULT) != HKS_SUCCESS) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
}

#define HKS_IF_NOT_SUCC_LOGI(RESULT, LOG_MESSAGE, ...) \
if ((RESULT) != HKS_SUCCESS) { \
    HKS_LOG_I(LOG_MESSAGE, ##__VA_ARGS__); \
}

#define HKS_IF_NOT_SUCC_RETURN(RESULT, ERROR_CODE) \
if ((RESULT) != HKS_SUCCESS) { \
    return (ERROR_CODE); \
}

#define HKS_IF_NOT_SUCC_RETURN_VOID(RESULT) \
if ((RESULT) != HKS_SUCCESS) { \
    return; \
}

#define HKS_IF_NOT_SUCC_LOGE_RETURN_VOID(RESULT, LOG_MESSAGE, ...) \
if ((RESULT) != HKS_SUCCESS) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    return; \
}

#define HKS_IF_NULL_LOGE_RETURN(OBJECT, ERROR_CODE, LOG_MESSAGE, ...) \
if ((OBJECT) == HKS_NULL_POINTER) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    return (ERROR_CODE); \
}

#define HKS_IF_NULL_LOGI_RETURN(OBJECT, ERROR_CODE, LOG_MESSAGE, ...) \
if ((OBJECT) == HKS_NULL_POINTER) { \
    HKS_LOG_I(LOG_MESSAGE, ##__VA_ARGS__); \
    return (ERROR_CODE); \
}

#define HKS_IF_NULL_LOGE_RETURN_VOID(OBJECT, LOG_MESSAGE, ...) \
if ((OBJECT) == HKS_NULL_POINTER) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    return; \
}

#define HKS_IF_NULL_LOGE_BREAK(OBJECT, LOG_MESSAGE, ...) \
if ((OBJECT) == HKS_NULL_POINTER) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    break; \
}

#define HKS_IF_NULL_RETURN(OBJECT, ERROR_CODE) \
if ((OBJECT) == HKS_NULL_POINTER) { \
    return (ERROR_CODE); \
}

#define HKS_IF_NULL_RETURN_VOID(OBJECT) \
if ((OBJECT) == HKS_NULL_POINTER) { \
    return; \
}

#define HKS_IF_NULL_BREAK(OBJECT) \
if ((OBJECT) == HKS_NULL_POINTER) { \
    break; \
}

#define HKS_IF_TRUE_RETURN(BOOL_FUNC, ERROR_CODE) \
if (BOOL_FUNC) { \
    return (ERROR_CODE); \
}

#define HKS_IF_TRUE_CONTINUE(BOOL_FUNC) \
if (BOOL_FUNC) { \
    continue; \
}

#define HKS_IF_TRUE_EXCU_CONTINUE(BOOL_FUNC, EXCU_FUNC) \
({ if (BOOL_FUNC) { \
    (EXCU_FUNC); \
    continue; \
} })

#define HKS_IF_TRUE_RETURN_VOID(BOOL_FUNC) \
if ((BOOL_FUNC)) { \
    return; \
}

#define HKS_IF_TRUE_LOGE_RETURN(BOOL_FUNC, ERROR_CODE, LOG_MESSAGE, ...) \
if (BOOL_FUNC) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    return (ERROR_CODE); \
}

#define HKS_IF_TRUE_LOGE_CONTINUE(BOOL_FUNC, LOG_MESSAGE, ...) \
if ((BOOL_FUNC)) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    continue; \
}

#define HKS_IF_TRUE_LOGE_RETURN_VOID(BOOL_FUNC, LOG_MESSAGE, ...) \
if ((BOOL_FUNC)) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    return; \
}

#define HKS_IF_TRUE_LOGI_RETURN_VOID(BOOL_FUNC, LOG_MESSAGE, ...) \
if ((BOOL_FUNC)) { \
    HKS_LOG_I(LOG_MESSAGE, ##__VA_ARGS__); \
    return; \
}

#define HKS_IF_TRUE_LOGI_RETURN(BOOL_FUNC, ERROR_CODE, LOG_MESSAGE, ...) \
if (BOOL_FUNC) { \
    HKS_LOG_I(LOG_MESSAGE, ##__VA_ARGS__); \
    return (ERROR_CODE); \
}

#define HKS_IF_TRUE_LOGI_BREAK(BOOL_FUNC, LOG_MESSAGE, ...) \
if ((BOOL_FUNC)) { \
    HKS_LOG_I(LOG_MESSAGE, ##__VA_ARGS__); \
    break; \
}

#define HKS_IF_TRUE_LOGE(BOOL_FUNC, LOG_MESSAGE, ...) \
if (BOOL_FUNC) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
}

#define HKS_IF_NOT_TRUE_LOGE(BOOL_FUNC, LOG_MESSAGE, ...) \
if (!(BOOL_FUNC)) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
}

#define HKS_IF_NOT_TRUE_RETURN(BOOL_FUNC, ERROR_CODE) \
if (!(BOOL_FUNC)) { \
    return (ERROR_CODE); \
}

#define HKS_IF_NOT_TRUE_LOGE_BREAK(BOOL_FUNC, LOG_MESSAGE, ...) \
if (!(BOOL_FUNC)) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    break; \
}

#define HKS_IF_NOT_TRUE_LOGE_RETURN(BOOL_FUNC, ERROR_CODE, LOG_MESSAGE, ...) \
if (!(BOOL_FUNC)) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    return (ERROR_CODE); \
}

#define HKS_IF_NOT_EOK_LOGE_RETURN(BOOL_FUNC, ERROR_CODE, LOG_MESSAGE, ...) \
if ((BOOL_FUNC) != EOK) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    return (ERROR_CODE); \
}

#define HKS_IF_NOT_EOK_LOGE_BREAK(BOOL_FUNC, LOG_MESSAGE, ...) \
if ((BOOL_FUNC) != EOK) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    break; \
}

#define HKS_IF_NOT_EOK_BREAK(BOOL_FUNC) \
if ((BOOL_FUNC) != EOK) { \
    break; \
}

#define HKS_IF_TRUE_LOGE_BREAK(BOOL_FUNC, LOG_MESSAGE, ...) \
if (BOOL_FUNC) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    break; \
}

#define HKS_IF_TRUE_BREAK(BOOL_FUNC) \
if (BOOL_FUNC) { \
    break; \
}

#define HKS_IF_NOT_TRUE_RETURN_VOID(BOOL_FUNC) \
if (!(BOOL_FUNC)) { \
    return; \
} \

#define HKS_IF_NOT_TRUE_LOGE_RETURN_VOID(BOOL_FUNC, LOG_MESSAGE, ...) \
if (!(BOOL_FUNC)) { \
    HKS_LOG_E(LOG_MESSAGE, ##__VA_ARGS__); \
    return; \
}

#define HKS_LOG_ERRNO(msg, ret) ({ int currentErrno = errno; \
    HKS_LOG_E_IMPORTANT(msg " ret: %" LOG_PUBLIC "d, errno: %" LOG_PUBLIC "d, strerror: %" LOG_PUBLIC "s", \
        (ret), currentErrno, strerror(currentErrno)); })

#define HKS_LOG_FILE_OP_ERRNO(msg, type) ({ int currentErrno = errno; \
    HKS_LOG_E_IMPORTANT(msg " pathType: %" LOG_PUBLIC "d, errno: %" LOG_PUBLIC "d, strerror: %" LOG_PUBLIC "s", \
        (type), currentErrno, strerror(currentErrno)); })

#define HKS_IF_NOT_SUCC_LOG_ERRNO_RETURN(msg, ret) ({ int currentErrno = errno; \
    if ((ret) != 0) { \
        HKS_LOG_E(msg " %" LOG_PUBLIC "d, errno: %" LOG_PUBLIC "d, strerror: %" LOG_PUBLIC "s", \
            (ret), currentErrno, strerror(currentErrno)); \
        return HKS_FAILURE; \
    }})

#define HKS_IF_NOT_TRUE_LOGI(RESULT, LOG_MESSAGE, ...) \
do { \
    if (!(RESULT)) { \
        HKS_LOG_I(LOG_MESSAGE, ##__VA_ARGS__); \
    } \
} while (0)

#define HKS_IF_NOT_TRUE_LOGI_RETURN(RESULT, ERROR_CODE, LOG_MESSAGE, ...) \
if (!(RESULT)) { \
    HKS_LOG_I(LOG_MESSAGE, ##__VA_ARGS__); \
    return (ERROR_CODE); \
}

#define HKS_IF_NOT_TRUE_EXCU(BOOL_FUNC, EXCU_FUNC) \
({ if (!(BOOL_FUNC)) { \
    (EXCU_FUNC); \
} })

#define HKS_IF_TRUE_EXCU(BOOL_FUNC, EXCU_FUNC) \
({ if (BOOL_FUNC) { \
    (EXCU_FUNC); \
} })

#endif /* HKS_TEMPLATE_H */
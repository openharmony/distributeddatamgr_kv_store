/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef GRD_TOKENIZER_SQLITE_H
#define GRD_TOKENIZER_SQLITE_H

#ifndef GRD_API
#ifndef _WIN32
#define GRD_API __attribute__((visibility("default")))
#else
#define GRD_API
#endif  // _WIN32
#endif  // GRD_API

#include "sqlite3ext.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef int (*xTokenFn)(void *, int, const char *, int, int, int);

GRD_API int fts5_customtokenizer_xCreate(void *sqlite3, const char **azArg, int nArg, Fts5Tokenizer **ppOut);
GRD_API int fts5_customtokenizer_xTokenize(
    Fts5Tokenizer *tokenizer_ptr, void *pCtx, int flags, const char *pText, int nText, xTokenFn xToken);
GRD_API void fts5_customtokenizer_xDelete(Fts5Tokenizer *tokenizer_ptr);

GRD_API int sqlite3_customtokenizer_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // GRD_TOKENIZER_SQLITE_H

/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef GRD_DOCUMENT_API_INNER_H
#define GRD_DOCUMENT_API_INNER_H

#include <cstdint>

#include "grd_base/grd_resultset_api.h"
#include "grd_base/grd_type_export.h"

namespace DocumentDB {
int32_t GRD_CreateCollectionInner(GRD_DB *db, const char *collectionName, const char *optionStr, uint32_t flags);

int32_t GRD_DropCollectionInner(GRD_DB *db, const char *collectionName, uint32_t flags);

int32_t GRD_InsertDocInner(GRD_DB *db, const char *collectionName, const char *document, uint32_t flags);

int32_t GRD_FindDocInner(GRD_DB *db, const char *collectionName, Query query, uint32_t flags,
    GRD_ResultSet **resultSet);

int32_t GRD_UpdateDocInner(GRD_DB *db, const char *collectionName, const char *filter, const char *update,
    uint32_t flags);

int32_t GRD_UpsertDocInner(GRD_DB *db, const char *collectionName, const char *filter, const char *document,
    uint32_t flags);

int32_t GRD_DeleteDocInner(GRD_DB *db, const char *collectionName, const char *filter, uint32_t flags);
} // namespace DocumentDB
#endif // GRD_DOCUMENT_API_INNER_H
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

#ifndef CJSON_IOS_H
#define CJSON_IOS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    void *obj;
    int valueint;
    double valuedouble;
    char *string;
    char *valuestring;
} cJSON;

static constexpr int CJSON_SHIFT_NULL = 2;
static constexpr int CJSON_SHIFT_NUMBER = 3;
static constexpr int CJSON_SHIFT_STRING = 4;
static constexpr int CJSON_SHIFT_ARRAY = 5;
static constexpr int CJSON_SHIFT_OBJECT = 6;
static constexpr int CJSON_SHIFT_RAW = 7;

#define cJSON_Invalid (0)
#define cJSON_False  (1 << 0)
#define cJSON_True   (1 << 1)
#define cJSON_NULL   (1 << CJSON_SHIFT_NULL)
#define cJSON_Number (1 << CJSON_SHIFT_NUMBER)
#define cJSON_String (1 << CJSON_SHIFT_STRING)
#define cJSON_Array  (1 << CJSON_SHIFT_ARRAY)
#define cJSON_Object (1 << CJSON_SHIFT_OBJECT)
#define cJSON_Raw    (1 << CJSON_SHIFT_RAW)

cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated);
void cJSON_Delete(cJSON *item);
void cJSON_free(void *ptr);
int cJSON_Compare(const cJSON *a, const cJSON *b, int case_sensitive);
char *cJSON_PrintUnformatted(const cJSON *item);
char *cJSON_Print(const cJSON *item);

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);
cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string);
cJSON *cJSON_GetArrayItem(const cJSON *array, int index);
double cJSON_GetNumberValue(const cJSON *item);
int cJSON_IsNumber(const cJSON *item);

cJSON *cJSON_CreateObject(void);

int cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
int cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem);
int cJSON_ReplaceItemInObjectCaseSensitive(cJSON *object, const char *string, cJSON *newitem);
int cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem);

void cJSON_DeleteItemFromObject(cJSON *object, const char *string);
void cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, const char *string);
void cJSON_DeleteItemFromArray(cJSON *array, int which);

cJSON *cJSON_Duplicate(const cJSON *item, int recurse);

#ifdef __cplusplus
}
#endif
#endif // CJSON_IOS_H

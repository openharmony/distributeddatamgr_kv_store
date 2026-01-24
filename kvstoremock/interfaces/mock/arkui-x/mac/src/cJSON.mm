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

#include "cJSON.h"

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#include <cstdlib>
#include <cstring>

static inline id RdGetObj(const cJSON *node)
{
    if (node == nullptr || node->obj == nullptr) {
        return nil;
    }
    return (__bridge id)(node->obj);
}

static inline int RdInferType(id obj)
{
    if (obj == nil) {
        return cJSON_Invalid;
    }
    if (obj == [NSNull null]) {
        return cJSON_NULL;
    }
    if ([obj isKindOfClass:[NSDictionary class]] || [obj isKindOfClass:[NSMutableDictionary class]]) {
        return cJSON_Object;
    }
    if ([obj isKindOfClass:[NSArray class]] || [obj isKindOfClass:[NSMutableArray class]]) {
        return cJSON_Array;
    }
    if ([obj isKindOfClass:[NSString class]]) {
        return cJSON_String;
    }
    if ([obj isKindOfClass:[NSNumber class]]) {
        return cJSON_Number;
    }
    return cJSON_Raw;
}

static inline cJSON *RdWrapObj(id obj)
{
    if (obj == nil) {
        return nullptr;
    }
    cJSON *node = static_cast<cJSON *>(calloc(1, sizeof(cJSON)));
    if (node == nullptr) {
        return nullptr;
    }
    node->type = RdInferType(obj);
    node->obj = (void *)CFBridgingRetain(obj);
    return node;
}

static inline void RdFreeNode(cJSON *node)
{
    if (node == nullptr) {
        return;
    }
    if (node->obj != nullptr) {
        CFBridgingRelease(node->obj);
        node->obj = nullptr;
    }
    if (node->string != nullptr) {
        free(node->string);
        node->string = nullptr;
    }
    if (node->valuestring != nullptr) {
        free(node->valuestring);
        node->valuestring = nullptr;
    }
    free(node);
}

static inline NSData *RdSerializeJson(id obj, NSJSONWritingOptions options, NSError **error)
{
    if (obj == nil) {
        return nil;
    }
    return [NSJSONSerialization dataWithJSONObject:obj
        options:(options | NSJSONWritingFragmentsAllowed) error:error];
}

extern "C" {
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated)
{
    @autoreleasepool {
        if (value == nullptr) {
            if (return_parse_end) {
                *return_parse_end = value;
            }
            return nullptr;
        }

        NSString *jsonStr = [NSString stringWithUTF8String:value];
        if (jsonStr == nil) {
            if (return_parse_end) {
                *return_parse_end = value;
            }
            return nullptr;
        }

        NSData *data = [jsonStr dataUsingEncoding:NSUTF8StringEncoding];
        if (data == nil) {
            if (return_parse_end) {
                *return_parse_end = value;
            }
            return nullptr;
        }

        NSError *error = nil;
        id obj = [NSJSONSerialization JSONObjectWithData:data
            options:(NSJSONReadingMutableContainers | NSJSONReadingMutableLeaves | NSJSONReadingFragmentsAllowed)
            error:&error];
        if (error != nil || obj == nil) {
            if (return_parse_end) {
                *return_parse_end = value;
            }
            return nullptr;
        }

        if (return_parse_end) {
            *return_parse_end = value + strlen(value);
        }

        return RdWrapObj(obj);
    }
}

void cJSON_Delete(cJSON *item)
{
    RdFreeNode(item);
}

void cJSON_free(void *ptr)
{
    if (ptr != nullptr) {
        free(ptr);
    }
}

int cJSON_Compare(const cJSON *a, const cJSON *b, int case_sensitive)
{
    @autoreleasepool {
        id av = RdGetObj(a);
        id bv = RdGetObj(b);

        if (av == nil && bv == nil) {
            return 1;
        }
        if (av == nil || bv == nil) {
            return 0;
        }
        return [av isEqual:bv] ? 1 : 0;
    }
}

char *cJSON_PrintUnformatted(const cJSON *item)
{
    @autoreleasepool {
        id obj = RdGetObj(item);
        if (obj == nil) {
            return nullptr;
        }

        NSError *error = nil;
        NSData *data = RdSerializeJson(obj, 0, &error);
        if (data == nil || error != nil) {
            return nullptr;
        }

        const NSUInteger len = [data length];
        char *result = static_cast<char *>(malloc(len + 1));
        if (result == nullptr) {
            return nullptr;
        }
        [data getBytes:result length:len];
        result[len] = '\0';
        return result;
    }
}

char *cJSON_Print(const cJSON *item)
{
    @autoreleasepool {
        id obj = RdGetObj(item);
        if (obj == nil) {
            return nullptr;
        }

        NSError *error = nil;
        NSData *data = RdSerializeJson(obj, NSJSONWritingPrettyPrinted, &error);
        if (data == nil || error != nil) {
            return nullptr;
        }

        const NSUInteger len = [data length];
        char *result = static_cast<char *>(malloc(len + 1));
        if (result == nullptr) {
            return nullptr;
        }
        [data getBytes:result length:len];
        result[len] = '\0';
        return result;
    }
}

cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string)
{
    @autoreleasepool {
        id obj = RdGetObj(object);
        if (![obj isKindOfClass:[NSDictionary class]] || string == nullptr) {
            return nullptr;
        }

        NSDictionary *dict = (NSDictionary *)obj;
        NSString *key = [NSString stringWithUTF8String:string];
        if (key == nil) {
            return nullptr;
        }

        id value = dict[key];
        return RdWrapObj(value);
    }
}

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string)
{
    @autoreleasepool {
        id obj = RdGetObj(object);
        if (![obj isKindOfClass:[NSDictionary class]] || string == nullptr) {
            return nullptr;
        }

        NSDictionary *dict = (NSDictionary *)obj;
        NSString *key = [NSString stringWithUTF8String:string];
        if (key == nil) {
            return nullptr;
        }

        for (NSString *k in dict) {
            if ([k compare:key options:NSCaseInsensitiveSearch] == NSOrderedSame) {
                id value = dict[k];
                return RdWrapObj(value);
            }
        }
        return nullptr;
    }
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int index)
{
    @autoreleasepool {
        id obj = RdGetObj(array);
        if (![obj isKindOfClass:[NSArray class]] || index < 0) {
            return nullptr;
        }

        NSArray *arr = (NSArray *)obj;
        if ((NSUInteger)index >= [arr count]) {
            return nullptr;
        }

        id value = arr[(NSUInteger)index];
        return RdWrapObj(value);
    }
}

double cJSON_GetNumberValue(const cJSON *item)
{
    @autoreleasepool {
        id obj = RdGetObj(item);
        if (![obj isKindOfClass:[NSNumber class]]) {
            return 0.0;
        }
        return [(NSNumber *)obj doubleValue];
    }
}

int cJSON_IsNumber(const cJSON *item)
{
    @autoreleasepool {
        id obj = RdGetObj(item);
        return [obj isKindOfClass:[NSNumber class]] ? 1 : 0;
    }
}

void cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, const char *string)
{
    @autoreleasepool {
        id obj = RdGetObj(object);
        if (![obj isKindOfClass:[NSMutableDictionary class]] || string == nullptr) {
            return;
        }

        NSMutableDictionary *dict = (NSMutableDictionary *)obj;
        NSString *key = [NSString stringWithUTF8String:string];
        if (key != nil) {
            [dict removeObjectForKey:key];
        }
    }
}

void cJSON_DeleteItemFromObject(cJSON *object, const char *string)
{
    @autoreleasepool {
        id obj = RdGetObj(object);
        if (![obj isKindOfClass:[NSMutableDictionary class]] || string == nullptr) {
            return;
        }

        NSMutableDictionary *dict = (NSMutableDictionary *)obj;
        NSString *key = [NSString stringWithUTF8String:string];
        if (key == nil) {
            return;
        }

        for (NSString *k in [[dict allKeys] copy]) {
            if ([k compare:key options:NSCaseInsensitiveSearch] == NSOrderedSame) {
                [dict removeObjectForKey:k];
                return;
            }
        }
    }
}

void cJSON_DeleteItemFromArray(cJSON *array, int which)
{
    @autoreleasepool {
        id obj = RdGetObj(array);
        if (![obj isKindOfClass:[NSMutableArray class]] || which < 0) {
            return;
        }

        NSMutableArray *arr = (NSMutableArray *)obj;
        if ((NSUInteger)which < [arr count]) {
            [arr removeObjectAtIndex:(NSUInteger)which];
        }
    }
}

cJSON *cJSON_Duplicate(const cJSON *item, int recurse)
{
    @autoreleasepool {
        if (item == nullptr) {
            return nullptr;
        }

        id obj = RdGetObj(item);
        if (obj == nil) {
            return nullptr;
        }

        if (!recurse) {
            return RdWrapObj(obj);
        }

        NSError *error = nil;
        NSData *data = RdSerializeJson(obj, 0, &error);
        if (data == nil || error != nil) {
            return RdWrapObj(obj);
        }

        id copied = [NSJSONSerialization JSONObjectWithData:data
            options:(NSJSONReadingMutableContainers | NSJSONReadingMutableLeaves | NSJSONReadingFragmentsAllowed)
            error:&error];
        return RdWrapObj(copied ?: obj);
    }
}

int cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item)
{
    @autoreleasepool {
        id obj = RdGetObj(object);
        if (![obj isKindOfClass:[NSMutableDictionary class]] || string == nullptr) {
            return 0;
        }

        NSMutableDictionary *dict = (NSMutableDictionary *)obj;
        NSString *key = [NSString stringWithUTF8String:string];
        id value = RdGetObj(item);

        if (key != nil && value != nil) {
            dict[key] = value;
            cJSON_Delete(item);
            return 1;
        }
        return 0;
    }
}

int cJSON_ReplaceItemInObjectCaseSensitive(cJSON *object, const char *string, cJSON *newitem)
{
    cJSON_DeleteItemFromObjectCaseSensitive(object, string);
    return cJSON_AddItemToObject(object, string, newitem);
}

int cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem)
{
    @autoreleasepool {
        id obj = RdGetObj(array);
        if (![obj isKindOfClass:[NSMutableArray class]] || which < 0) {
            return 0;
        }

        NSMutableArray *arr = (NSMutableArray *)obj;
        if ((NSUInteger)which < [arr count]) {
            id value = RdGetObj(newitem);
            if (value != nil) {
                arr[(NSUInteger)which] = value;
                cJSON_Delete(newitem);
                return 1;
            }
        }
        return 0;
    }
}

int cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem)
{
    @autoreleasepool {
        id obj = RdGetObj(array);
        if (![obj isKindOfClass:[NSMutableArray class]] || which < 0) {
            return 0;
        }

        NSMutableArray *arr = (NSMutableArray *)obj;
        if ((NSUInteger)which <= [arr count]) {
            id value = RdGetObj(newitem);
            if (value != nil) {
                [arr insertObject:value atIndex:(NSUInteger)which];
                cJSON_Delete(newitem);
                return 1;
            }
        }
        return 0;
    }
}

cJSON *cJSON_CreateObject(void)
{
    @autoreleasepool {
        return RdWrapObj([NSMutableDictionary dictionary]);
    }
}
} // extern "C"

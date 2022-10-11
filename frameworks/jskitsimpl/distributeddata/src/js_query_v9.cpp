/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "JS_QueryV9"
#include "js_query.h"
#include "js_util.h"
#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"

using namespace OHOS::DistributedKv;

namespace OHOS::DistributedData {
DataQuery& JsQueryV9::GetNative()
{
    return query_;
}

napi_value JsQueryV9::Constructor(napi_env env)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("reset", JsQueryV9::Reset),
        DECLARE_NAPI_FUNCTION("equalTo", JsQueryV9::EqualTo),
        DECLARE_NAPI_FUNCTION("notEqualTo", JsQueryV9::NotEqualTo),
        DECLARE_NAPI_FUNCTION("greaterThan", JsQueryV9::GreaterThan),
        DECLARE_NAPI_FUNCTION("lessThan", JsQueryV9::LessThan),
        DECLARE_NAPI_FUNCTION("greaterThanOrEqualTo", JsQueryV9::GreaterThanOrEqualTo),
        DECLARE_NAPI_FUNCTION("lessThanOrEqualTo", JsQueryV9::LessThanOrEqualTo),
        DECLARE_NAPI_FUNCTION("isNull", JsQueryV9::IsNull),
        DECLARE_NAPI_FUNCTION("inNumber", JsQueryV9::InNumber),
        DECLARE_NAPI_FUNCTION("inString", JsQueryV9::InString),
        DECLARE_NAPI_FUNCTION("notInNumber", JsQueryV9::NotInNumber),
        DECLARE_NAPI_FUNCTION("notInString", JsQueryV9::NotInString),
        DECLARE_NAPI_FUNCTION("like", JsQueryV9::Like),
        DECLARE_NAPI_FUNCTION("unlike", JsQueryV9::Unlike),
        DECLARE_NAPI_FUNCTION("and", JsQueryV9::And),
        DECLARE_NAPI_FUNCTION("or", JsQueryV9::Or),
        DECLARE_NAPI_FUNCTION("orderByAsc", JsQueryV9::OrderByAsc),
        DECLARE_NAPI_FUNCTION("orderByDesc", JsQueryV9::OrderByDesc),
        DECLARE_NAPI_FUNCTION("limit", JsQueryV9::Limit),
        DECLARE_NAPI_FUNCTION("isNotNull", JsQueryV9::IsNotNull),
        DECLARE_NAPI_FUNCTION("beginGroup", JsQueryV9::BeginGroup),
        DECLARE_NAPI_FUNCTION("endGroup", JsQueryV9::EndGroup),
        DECLARE_NAPI_FUNCTION("prefixKey", JsQueryV9::PrefixKey),
        DECLARE_NAPI_FUNCTION("setSuggestIndex", JsQueryV9::SetSuggestIndex),
        DECLARE_NAPI_FUNCTION("deviceId", JsQueryV9::DeviceId),
        DECLARE_NAPI_FUNCTION("getSqlLike", JsQueryV9::GetSqlLike)
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);
    return JSUtil::DefineClass(env, "QueryV9", properties, count, JsQueryV9::New);
}

/*
 * [JS API Prototype]
 *      var query = new ddm.JsQueryV9();
 */
napi_value JsQueryV9::New(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The parameter is incorrect.");

    JsQueryV9* query = new (std::nothrow) JsQueryV9();
    CHECK_IF_ASSERT(env, query != nullptr, PARAM_ERROR, "no memory for query.");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("query finalize.");
        auto* query = reinterpret_cast<JsQueryV9*>(data);
        CHECK_RETURN_VOID(query != nullptr, "finalize null!");
        delete query;
    };
    NAPI_CALL(env, napi_wrap(env, ctxt->self, query, finalize, nullptr, nullptr));
    return ctxt->self;
}

napi_value JsQueryV9::Reset(napi_env env, napi_callback_info info)
{
    ZLOGE("Query::pppppppppReset()");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function Reset parameter is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.Reset();
    return ctxt->self;
}

struct ValueContextV9 : public ContextBase {
    std::string field;
    JSUtil::QueryVariant vv;

    void GetValueSync(napi_env env, napi_callback_info info)
    {
        auto input = [this, env](size_t argc, napi_value* argv) {
            // required 2 arguments :: <field> <value>
            CHECK_THROW_BUSINESS_ERR(this, argc >= 2, PARAM_ERROR, "The number of parameters is incorrect.");
            status = JSUtil::GetValue(env, argv[0], field);
            CHECK_THROW_BUSINESS_ERR(this, status == napi_ok, PARAM_ERROR, "The parameters field is incorrect.");
            status = JSUtil::GetValue(env, argv[1], vv);
            CHECK_THROW_BUSINESS_ERR(this, status == napi_ok, PARAM_ERROR, "The parameters value is incorrect.");
        };
        GetCbInfoSync(env, info, input);
    }
};

/* [js] equalTo(field:string, value:number|string|boolean):JsQueryV9 */
napi_value JsQueryV9::EqualTo(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::EqualTo()");
    auto ctxt = std::make_shared<ValueContextV9>();
    ctxt->GetValueSync(env, info);
    if (ctxt->isThrowError) {
        ZLOGE("EqualToV9 exit");
        return nullptr;
    }
    CHECK_IF_RETURN("EqualToV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function EqualTo parameter is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.EqualTo(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.EqualTo(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.EqualTo(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQueryV9::NotEqualTo(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::NotEqualTo()");
    auto ctxt = std::make_shared<ValueContextV9>();
    ctxt->GetValueSync(env, info);
    CHECK_IF_RETURN("NotEqualToV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function NotEqualTo parameter is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.NotEqualTo(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.NotEqualTo(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.NotEqualTo(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQueryV9::GreaterThan(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::GreaterThan()");
    auto ctxt = std::make_shared<ValueContextV9>();
    ctxt->GetValueSync(env, info);
    CHECK_IF_RETURN("GreaterThanV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function GreaterThan parameter is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.GreaterThan(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.GreaterThan(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.GreaterThan(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQueryV9::LessThan(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::LessThan()");
    auto ctxt = std::make_shared<ValueContextV9>();
    ctxt->GetValueSync(env, info);
    CHECK_IF_RETURN("LessThanV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function LessThan parameter is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.LessThan(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.LessThan(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.LessThan(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQueryV9::GreaterThanOrEqualTo(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::GreaterThanOrEqualTo()");
    auto ctxt = std::make_shared<ValueContextV9>();
    ctxt->GetValueSync(env, info);
    CHECK_IF_RETURN("GreaterThanOrEqualToV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function GreaterThanOrEqualTo parameter is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.GreaterThanOrEqualTo(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.GreaterThanOrEqualTo(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.GreaterThanOrEqualTo(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQueryV9::LessThanOrEqualTo(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::LessThanOrEqualTo()");
    auto ctxt = std::make_shared<ValueContextV9>();
    ctxt->GetValueSync(env, info);
    CHECK_IF_RETURN("LessThanOrEqualToV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function LessThanOrEqualTo parameter is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.LessThanOrEqualTo(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.LessThanOrEqualTo(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.LessThanOrEqualTo(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQueryV9::IsNull(napi_env env, napi_callback_info info)
{
    std::string field;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &field](size_t argc, napi_value* argv) {
        // required 1 arguments :: <field>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 1, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], field);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The parameters field is incorrect.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("IsNullV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function IsNull parameter is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.IsNull(field);
    return ctxt->self;
}

/*
 * InNumber / NotInNumber
 * [NOTES] Recommended to use the napi_typedarray_type
 */
enum class NumberType : uint8_t {
    NUMBER_INT,
    NUMBER_LONG,
    NUMBER_DOUBLE,
    NUMBER_INVALID = 255
};
struct NumbersContextV9 : public ContextBase {
    std::string field;
    std::vector<int> intList;
    std::vector<int64_t> longList;
    std::vector<double> doubleList;
    NumberType innerType = NumberType::NUMBER_INVALID;

    void GetNumberSync(napi_env env, napi_callback_info info)
    {
        auto input = [this, env](size_t argc, napi_value* argv) {
            // required 2 arguments :: <field> <value-list>
            CHECK_THROW_BUSINESS_ERR(this, argc >= 2, PARAM_ERROR, "The number of parameters is incorrect.");
            status = JSUtil::GetValue(env, argv[0], field);
            CHECK_THROW_BUSINESS_ERR(this, status == napi_ok, PARAM_ERROR, "The parameters field is incorrect.");
            bool isTypedArray = false;
            status = napi_is_typedarray(env, argv[1], &isTypedArray);
            ZLOGD("arg[1] %{public}s a TypedArray", isTypedArray ? "is" : "is not");
            if (isTypedArray && (status == napi_ok)) {
                napi_typedarray_type type = napi_biguint64_array;
                size_t length = 0;
                napi_value buffer = nullptr;
                size_t offset = 0;
                void* data = nullptr;
                status = napi_get_typedarray_info(env, argv[1], &type, &length, &data, &buffer, &offset);
                CHECK_THROW_BUSINESS_ERR(this, status == napi_ok, PARAM_ERROR, "The parameters number array is incorrect.");
                if (type < napi_uint32_array) {
                    status = JSUtil::GetValue(env, argv[1], intList);
                    innerType = NumberType::NUMBER_INT;
                } else if (type == napi_bigint64_array || type == napi_uint32_array) {
                    status = JSUtil::GetValue(env, argv[1], longList);
                    innerType = NumberType::NUMBER_LONG;
                } else {
                    status = JSUtil::GetValue(env, argv[1], doubleList);
                    innerType = NumberType::NUMBER_DOUBLE;
                }
            } else {
                bool isArray = false;
                status = napi_is_array(env, argv[1], &isArray);
                CHECK_THROW_BUSINESS_ERR(this, isArray, PARAM_ERROR, "The type of parameters number array is incorrect.");
                ZLOGD("arg[1] %{public}s a Array, treat as array of double.", isTypedArray ? "is" : "is not");
                status = JSUtil::GetValue(env, argv[1], doubleList);
                CHECK_THROW_BUSINESS_ERR(this, status == napi_ok, PARAM_ERROR, "The parameters number array is incorrect.");
                innerType = NumberType::NUMBER_DOUBLE;
            }
        };
        GetCbInfoSync(env, info, input);
    }
};

napi_value JsQueryV9::InNumber(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::InNumber()");
    auto ctxt = std::make_shared<NumbersContextV9>();
    ctxt->GetNumberSync(env, info);
    CHECK_IF_RETURN("InNumberV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function InNumber parameter is incorrect.");
    
    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    if (ctxt->innerType == NumberType::NUMBER_INT) {
        query.In(ctxt->field, ctxt->intList);
    } else if (ctxt->innerType == NumberType::NUMBER_LONG) {
        query.In(ctxt->field, ctxt->longList);
    } else if (ctxt->innerType == NumberType::NUMBER_DOUBLE) {
        query.In(ctxt->field, ctxt->doubleList);
    }
    return ctxt->self;
}

napi_value JsQueryV9::InString(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::InString()");
    struct StringsContext : public ContextBase {
        std::string field;
        std::vector<std::string> valueList;
    };
    auto ctxt = std::make_shared<StringsContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <field> <valueList>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 2, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->field);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of field must be string.");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->valueList);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of valueList must be array.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("InNumberV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function InString parameter is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.In(ctxt->field, ctxt->valueList);
    return ctxt->self;
}

napi_value JsQueryV9::NotInNumber(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::NotInNumber()");
    auto ctxt = std::make_shared<NumbersContextV9>();
    ctxt->GetNumberSync(env, info);
    CHECK_IF_RETURN("NotInNumber exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function InString NotInNumber is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    if (ctxt->innerType == NumberType::NUMBER_INT) {
        query.NotIn(ctxt->field, ctxt->intList);
    } else if (ctxt->innerType == NumberType::NUMBER_LONG) {
        query.NotIn(ctxt->field, ctxt->longList);
    } else if (ctxt->innerType == NumberType::NUMBER_DOUBLE) {
        query.NotIn(ctxt->field, ctxt->doubleList);
    }
    return ctxt->self;
}

napi_value JsQueryV9::NotInString(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::NotInString()");
    struct StringsContext : public ContextBase {
        std::string field;
        std::vector<std::string> valueList;
    };
    auto ctxt = std::make_shared<StringsContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <field> <valueList>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 2, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->field);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of field must be string.");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->valueList);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of valueList must be array.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("NotInStringV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function NotInString is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.NotIn(ctxt->field, ctxt->valueList);
    return ctxt->self;
}

napi_value JsQueryV9::Like(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::Like()");
    struct LikeContext : public ContextBase {
        std::string field;
        std::string value;
    };
    auto ctxt = std::make_shared<LikeContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <field> <value>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 2, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->field);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of field must be string.");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->value);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of value must be string.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("LikeV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function Like is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.Like(ctxt->field, ctxt->value);
    return ctxt->self;
}

napi_value JsQueryV9::Unlike(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::Unlike()");
    struct UnlikeContext : public ContextBase {
        std::string field;
        std::string value;
    };
    auto ctxt = std::make_shared<UnlikeContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <field> <value>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 2, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->field);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of field must be string.");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->value);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of value must be string.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("UnlikeV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function Unlike is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.Unlike(ctxt->field, ctxt->value);
    return ctxt->self;
}

napi_value JsQueryV9::And(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::And()");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function And is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.And();
    return ctxt->self;
}

napi_value JsQueryV9::Or(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::Or()");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function Or is incorrect.");
    
    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.Or();
    return ctxt->self;
}

napi_value JsQueryV9::OrderByAsc(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::OrderByAsc()");
    std::string field;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &field](size_t argc, napi_value* argv) {
        // required 1 arguments :: <field>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 1, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], field);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of field must be string.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("OrderByAscV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function OrderByAsc is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.OrderByAsc(field);
    return ctxt->self;
}

napi_value JsQueryV9::OrderByDesc(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::OrderByDesc()");
    std::string field;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &field](size_t argc, napi_value* argv) {
        // required 1 arguments :: <field>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 1, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], field);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of field must be string.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("OrderByDescV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function OrderByDesc is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.OrderByDesc(field);
    return ctxt->self;
}

napi_value JsQueryV9::Limit(napi_env env, napi_callback_info info)
{
    struct LimitContext : public ContextBase {
        int number;
        int offset;
    };
    auto ctxt = std::make_shared<LimitContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <number> <offset>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 2, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = napi_get_value_int32(env, argv[0], &ctxt->number);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of number must be int.");
        ctxt->status = napi_get_value_int32(env, argv[1], &ctxt->offset);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of offset must be int.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("LimitV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function Limit is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.Limit(ctxt->number, ctxt->offset);
    return ctxt->self;
}

napi_value JsQueryV9::IsNotNull(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::IsNotNull()");
    std::string field;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &field](size_t argc, napi_value* argv) {
        // required 1 arguments :: <field>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 1, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], field);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of field must be string.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("IsNotNullV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function IsNotNull is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.IsNotNull(field);
    return ctxt->self;
}

napi_value JsQueryV9::BeginGroup(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::BeginGroup()");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    CHECK_IF_RETURN("BeginGroupV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function BeginGroup is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.BeginGroup();
    return ctxt->self;
}

napi_value JsQueryV9::EndGroup(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::EndGroup()");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    CHECK_IF_RETURN("EndGroupV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function EndGroup is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.EndGroup();
    return ctxt->self;
}

napi_value JsQueryV9::PrefixKey(napi_env env, napi_callback_info info)
{
    std::string prefix;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &prefix](size_t argc, napi_value* argv) {
        // required 1 arguments :: <prefix>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 1, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], prefix);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of prefix must be string.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("PrefixKeyV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function PrefixKey is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.KeyPrefix(prefix);
    return ctxt->self;
}

napi_value JsQueryV9::SetSuggestIndex(napi_env env, napi_callback_info info)
{
    std::string suggestIndex;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &suggestIndex](size_t argc, napi_value* argv) {
        // required 1 arguments :: <suggestIndex>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 1, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], suggestIndex);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of suggestIndex must be string.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("SetSuggestIndexV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function SetSuggestIndex is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.SetSuggestIndex(suggestIndex);
    return ctxt->self;
}

napi_value JsQueryV9::DeviceId(napi_env env, napi_callback_info info)
{
    std::string deviceId;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &deviceId](size_t argc, napi_value* argv) {
        // required 1 arguments :: <deviceId>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 1, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], deviceId);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of deviceId must be string.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("DeviceIdV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function DeviceId is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    query.DeviceId(deviceId);
    return ctxt->self;
}

// getSqlLike():string
napi_value JsQueryV9::GetSqlLike(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    CHECK_IF_RETURN("GetSqlLikeV9 exit", ctxt->isThrowError);
    CHECK_IF_ASSERT(env, ctxt->status == napi_ok, PARAM_ERROR, "The function GetSqlLike is incorrect.");

    auto& query = reinterpret_cast<JsQueryV9*>(ctxt->native)->query_;
    JSUtil::SetValue(env, query.ToString(), ctxt->output);
    return ctxt->output;
}
} // namespace OHOS::DistributedData

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
#define LOG_TAG "JS_FieldNode"
#include "js_field_node.h"
#include "js_util.h"
#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"

using namespace OHOS::DistributedKv;

namespace OHOS::DistributedKVStore {
static std::string FIELD_NAME = "FIELD_NAME";
static std::string VALUE_TYPE = "VALUE_TYPE";
static std::string DEFAULT_VALUE = "DEFAULT_VALUE";
static std::string IS_DEFAULT_VALUE = "IS_DEFAULT_VALUE";
static std::string IS_NULLABLE = "IS_NULLABLE";
static std::string CHILDREN = "CHILDREN";

std::map<uint32_t, std::string> JsFieldNode::valueTypeToString_ = {
    { JSUtil::STRING, std::string("STRING") },
    { JSUtil::INTEGER, std::string("INTEGER") },
    { JSUtil::FLOAT, std::string("FLOAT") },
    { JSUtil::BYTE_ARRAY, std::string("BYTE_ARRAY") },
    { JSUtil::BOOLEAN, std::string("BOOLEAN") },
    { JSUtil::DOUBLE, std::string("DOUBLE") }
};

JsFieldNode::JsFieldNode(const std::string& fName)
    : fieldName(fName)
{
}

std::string JsFieldNode::GetFieldName()
{
    return fieldName;
}

JsFieldNode::json JsFieldNode::GetValueForJson()
{
    if (fields.empty()) {
        return ValueTypeToString(valueType) + "," + (isNullable ? "NULL" : "NOT NULL");
    }

    json jsFields;
    for (auto fld : fields) {
        jsFields[fld->fieldName] = fld->GetValueForJson();
    }
    return jsFields;
}

napi_value JsFieldNode::Constructor(napi_env env)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("appendChild", JsFieldNode::AppendChild),
        DECLARE_NAPI_GETTER_SETTER("default", JsFieldNode::GetDefaultValue, JsFieldNode::SetDefaultValue),
        DECLARE_NAPI_GETTER_SETTER("nullable", JsFieldNode::GetNullable, JsFieldNode::SetNullable),
        DECLARE_NAPI_GETTER_SETTER("type", JsFieldNode::GetValueType, JsFieldNode::SetValueType)
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);
    return JSUtil::DefineClass(env, "FieldNode", properties, count, JsFieldNode::New);
}

napi_value JsFieldNode::New(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::New");
    std::string fieldName;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &fieldName](size_t argc, napi_value* argv) {
        // required 1 arguments :: <fieldName>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], fieldName);
        ASSERT_BUSINESS_ERR(ctxt, ((ctxt->status == napi_ok) && !fieldName.empty()), Status::INVALID_ARGUMENT,
            "The parameters of fieldName is incorrect.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "JsFieldNode New exit");

    JsFieldNode* fieldNode = new (std::nothrow) JsFieldNode(fieldName);
    ASSERT_ERR(env, fieldNode != nullptr, Status::INVALID_ARGUMENT, "no memory for fieldNode.");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("fieldNode finalize.");
        auto* field = reinterpret_cast<JsFieldNode*>(data);
        ASSERT_VOID(field != nullptr, "finalize null!");
        delete field;
    };
    ASSERT_CALL(env, napi_wrap(env, ctxt->self, fieldNode, finalize, nullptr, nullptr), fieldNode);
    return ctxt->self;
}

napi_value JsFieldNode::AppendChild(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::AppendChild");
    JsFieldNode* child = nullptr;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &child](size_t argc, napi_value* argv) {
        // required 1 arguments :: <child>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::Unwrap(env, argv[0], reinterpret_cast<void**>(&child), JsFieldNode::Constructor(env));
        ASSERT_BUSINESS_ERR(ctxt, ((ctxt->status == napi_ok) && (child != nullptr)), Status::INVALID_ARGUMENT,
            "The parameters JsFieldNode is incorrect.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "AppendChild exit");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    fieldNode->fields.push_back(child);

    napi_get_boolean(env, true, &ctxt->output);
    return ctxt->output;
}

JsFieldNode* JsFieldNode::GetFieldNode(napi_env env, napi_callback_info info, std::shared_ptr<ContextBase>& ctxt)
{
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");
    return reinterpret_cast<JsFieldNode*>(ctxt->native);
}

template <typename T>
napi_value JsFieldNode::GetContextValue(napi_env env, std::shared_ptr<ContextBase> &ctxt, T &value)
{
    JSUtil::SetValue(env, value, ctxt->output);
    return ctxt->output;
}

napi_value JsFieldNode::GetDefaultValue(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::GetDefaultValue");
    auto ctxt = std::make_shared<ContextBase>();
    auto fieldNode = GetFieldNode(env, info, ctxt);
    ASSERT(fieldNode != nullptr, "getFieldNode nullptr!", nullptr);
    return GetContextValue(env, ctxt, fieldNode->defaultValue);
}

napi_value JsFieldNode::SetDefaultValue(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::SetDefaultValue");
    auto ctxt = std::make_shared<ContextBase>();
    JSUtil::KvStoreVariant vv;
    auto input = [env, ctxt, &vv](size_t argc, napi_value* argv) {
        // required 1 arguments :: <defaultValue>
        ASSERT_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], vv);
        ASSERT_STATUS(ctxt, "invalid arg[0], i.e. invalid defaultValue!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    fieldNode->defaultValue = vv;
    return nullptr;
}

napi_value JsFieldNode::GetNullable(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::GetNullable");
    auto ctxt = std::make_shared<ContextBase>();
    auto fieldNode = GetFieldNode(env, info, ctxt);
    ASSERT(fieldNode != nullptr, "getFieldNode nullptr!", nullptr);
    return GetContextValue(env, ctxt, fieldNode->isNullable);
}

napi_value JsFieldNode::SetNullable(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::SetNullable");
    auto ctxt = std::make_shared<ContextBase>();
    bool isNullable = false;
    auto input = [env, ctxt, &isNullable](size_t argc, napi_value* argv) {
        // required 1 arguments :: <isNullable>
        ASSERT_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], isNullable);
        ASSERT_STATUS(ctxt, "invalid arg[0], i.e. invalid isNullable!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    fieldNode->isNullable = isNullable;
    return nullptr;
}

napi_value JsFieldNode::GetValueType(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::GetValueType");
    auto ctxt = std::make_shared<ContextBase>();
    auto fieldNode = GetFieldNode(env, info, ctxt);
    ASSERT(fieldNode != nullptr, "getFieldNode nullptr!", nullptr);
    return GetContextValue(env, ctxt, fieldNode->valueType);
}

napi_value JsFieldNode::SetValueType(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::SetValueType");
    auto ctxt = std::make_shared<ContextBase>();
    uint32_t type = 0;
    auto input = [env, ctxt, &type](size_t argc, napi_value* argv) {
        // required 1 arguments :: <valueType>
        ASSERT_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], type);
        ASSERT_STATUS(ctxt, "invalid arg[0], i.e. invalid valueType!");
        ASSERT_ARGS(ctxt, (JSUtil::STRING <= type) && (type <= JSUtil::DOUBLE),
            "invalid arg[0], i.e. invalid valueType!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    fieldNode->valueType = type;
    return nullptr;
}

std::string JsFieldNode::ValueToString(JSUtil::KvStoreVariant value)
{
    auto strValue = std::get_if<std::string>(&value);
    if (strValue != nullptr) {
        return (*strValue);
    }
    auto intValue = std::get_if<int32_t>(&value);
    if (intValue != nullptr) {
        return std::to_string(*intValue);
    }
    auto fltValue = std::get_if<float>(&value);
    if (fltValue != nullptr) {
        return std::to_string(*fltValue);
    }
    auto boolValue = std::get_if<bool>(&value);
    if (boolValue != nullptr) {
        return std::to_string(*boolValue);
    }
    auto dblValue = std::get_if<double>(&value);
    if (dblValue != nullptr) {
        return std::to_string(*dblValue);
    }
    ZLOGE("ValueType is INVALID");
    return std::string();
}

std::string JsFieldNode::ValueTypeToString(uint32_t type)
{
    // DistributedDB::FieldType
    auto it = valueTypeToString_.find(type);
    if (valueTypeToString_.find(type) != valueTypeToString_.end()) {
        return it->second;
    } else {
        return std::string();
    }
}

std::string JsFieldNode::Dump()
{
    json jsFields;
    for (auto fld : fields) {
        jsFields.push_back(fld->Dump());
    }

    json jsNode = {
        { FIELD_NAME, fieldName },
        { VALUE_TYPE, ValueTypeToString(valueType) },
        { DEFAULT_VALUE, ValueToString(defaultValue) },
        { IS_DEFAULT_VALUE, isWithDefaultValue },
        { IS_NULLABLE, isNullable },
        { CHILDREN, jsFields.dump() }
    };
    return jsNode.dump();
}
} // namespace OHOS::DistributedKVStore

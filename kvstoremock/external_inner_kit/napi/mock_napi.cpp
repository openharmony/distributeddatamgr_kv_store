/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "napi/libuv/include/uv.h"
#include "js_native_api.h"
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "node_api.h"
#include "napi_base_context.h"
#include "ability.h"

extern "C" {
napi_status napi_create_threadsafe_function(napi_env env, napi_value func, napi_value async_resource,
    napi_value async_resource_name, size_t max_queue_size, size_t initial_thread_count, void *thread_finalize_data,
    napi_finalize thread_finalize_cb, void *context, napi_threadsafe_function_call_js call_js_cb,
    napi_threadsafe_function *result)
{
    return napi_ok;
}

napi_status napi_call_threadsafe_function(napi_threadsafe_function func, void *data,
    napi_threadsafe_function_call_mode is_blocking)
{
    return napi_ok;
}

napi_status napi_acquire_threadsafe_function(napi_threadsafe_function func)
{
    return napi_ok;
}

napi_status napi_release_threadsafe_function(napi_threadsafe_function func, napi_threadsafe_function_release_mode mode)
{
    return napi_ok;
}

napi_status napi_create_object_with_properties(napi_env env, napi_value *result, size_t property_count,
    const napi_property_descriptor *properties)
{
    return napi_ok;
}

napi_status napi_get_named_property(napi_env env, napi_value object, const char *utf8name, napi_value *result)
{
    return napi_ok;
}

napi_status napi_call_function(napi_env env, napi_value recv, napi_value func, size_t argc, const napi_value *argv,
    napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_cb_info(napi_env env, napi_callback_info cbinfo, size_t *argc, napi_value *argv,
    napi_value *this_arg, void **data)
{
    return napi_ok;
}

napi_status napi_get_global(napi_env env, napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_value_string_utf8(napi_env env, napi_value value, char *buf, size_t bufsize, size_t *result)
{
    return napi_ok;
}

napi_status napi_get_array_length(napi_env env, napi_value value, uint32_t *result)
{
    return napi_ok;
}

napi_status napi_is_typedarray(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_get_typedarray_info(napi_env env, napi_value typedarray, napi_typedarray_type *type, size_t *length,
    void **data, napi_value *arraybuffer, size_t *byte_offset)
{
    return napi_ok;
}

napi_status napi_get_element(napi_env env, napi_value object, uint32_t index, napi_value *result)
{
    return napi_ok;
}

napi_status napi_typeof(napi_env env, napi_value value, napi_valuetype *result)
{
    return napi_ok;
}

napi_status napi_get_value_bool(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_set_element(napi_env env, napi_value object, uint32_t index, napi_value value)
{
    return napi_ok;
}

napi_status napi_get_last_error_info(napi_env env, const napi_extended_error_info **result)
{
    return napi_ok;
}

napi_status napi_is_exception_pending(napi_env env, bool *result)
{
    return napi_ok;
}

napi_status napi_throw_error(napi_env env, const char *code, const char *msg)
{
    return napi_ok;
}

napi_status napi_get_value_double(napi_env env, napi_value value, double *result)
{
    return napi_ok;
}

napi_status napi_create_array_with_length(napi_env env, size_t length, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_string_utf8(napi_env env, const char *str, size_t length, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_arraybuffer(napi_env env, size_t byte_length, void **data, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_typedarray(napi_env env, napi_typedarray_type type, size_t length, napi_value arraybuffer,
    size_t byte_offset, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_int32(napi_env env, int32_t value, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_int64(napi_env env, int64_t value, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_double(napi_env env, double value, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_date(napi_env env, double value, napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_boolean(napi_env env, bool value, napi_value *result)
{
    return napi_ok;
}

napi_status napi_define_class(napi_env env, const char *utf8name, size_t length, napi_callback constructor,
    void *callback_data, size_t property_count, const napi_property_descriptor *properties, napi_value *result)
{
    return napi_ok;
}

napi_status napi_unwrap(napi_env env, napi_value obj, void **result)
{
    return napi_ok;
}

napi_status napi_delete_reference(napi_env env, napi_ref ref)
{
    return napi_ok;
}

napi_status napi_create_reference(napi_env env, napi_value value, uint32_t initial_refcount, napi_ref *result)
{
    return napi_ok;
}

napi_status napi_wrap(napi_env env, napi_value js_object, void *native_object, napi_finalize finalize_cb,
    void *finalize_hint, napi_ref *result)
{
    return napi_ok;
}

napi_status napi_get_undefined(napi_env env, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_async_work(napi_env env, napi_value async_resource, napi_value async_resource_name,
    napi_async_execute_callback execute, napi_async_complete_callback complete, void *data, napi_async_work *result)
{
    return napi_ok;
}

napi_status napi_set_named_property(napi_env env, napi_value object, const char *utf8name, napi_value value)
{
    return napi_ok;
}

napi_status napi_get_new_target(napi_env env, napi_callback_info cbinfo, napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_reference_value(napi_env env, napi_ref ref, napi_value *result)
{
    return napi_ok;
}

napi_status napi_new_instance(napi_env env, napi_value constructor, size_t argc, const napi_value *argv,
    napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_value_int32(napi_env env, napi_value value, int32_t *result)
{
    return napi_ok;
}

napi_status napi_reference_ref(napi_env env, napi_ref ref, uint32_t *result)
{
    return napi_ok;
}

napi_status napi_reference_unref(napi_env env, napi_ref ref, uint32_t *result)
{
    return napi_ok;
}

napi_status napi_get_property_names(napi_env env, napi_value object, napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_property(napi_env env, napi_value object, napi_value key, napi_value *result)
{
    return napi_ok;
}

napi_status napi_delete_async_work(napi_env env, napi_async_work work)
{
    return napi_ok;
}

napi_status napi_create_error(napi_env env, napi_value code, napi_value msg, napi_value *result)
{
    return napi_ok;
}

napi_status napi_resolve_deferred(napi_env env, napi_deferred deferred, napi_value resolution)
{
    return napi_ok;
}

napi_status napi_reject_deferred(napi_env env, napi_deferred deferred, napi_value resolution)
{
    return napi_ok;
}

napi_status napi_queue_async_work(napi_env env, napi_async_work work)
{
    return napi_ok;
}

napi_status napi_queue_async_work_with_qos(napi_env env, napi_async_work work, napi_qos_t qos)
{
    return napi_ok;
}

napi_status napi_create_promise(napi_env env, napi_deferred *deferred, napi_value *promise)
{
    return napi_ok;
}

napi_status napi_create_object(napi_env env, napi_value *result)
{
    return napi_ok;
}

napi_status napi_define_properties(napi_env env, napi_value object, size_t property_count,
    const napi_property_descriptor *properties)
{
    return napi_ok;
}

void napi_module_register(napi_module *mod) {}

napi_status napi_async_init(napi_env env, napi_value async_resource, napi_value async_resource_name,
    napi_async_context *result)
{
    return napi_ok;
}

napi_status napi_async_destroy(napi_env env, napi_async_context async_context)
{
    return napi_ok;
}

napi_status napi_make_callback(napi_env env, napi_async_context async_context, napi_value recv, napi_value func,
    size_t argc, const napi_value *argv, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_buffer(napi_env env, size_t length, void **data, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_external_buffer(napi_env env, size_t length, void *data, napi_finalize finalize_cb,
    void *finalize_hint, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_buffer_copy(napi_env env, size_t length, const void *data, void **result_data,
    napi_value *result)
{
    return napi_ok;
}

napi_status napi_is_buffer(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_get_buffer_info(napi_env env, napi_value value, void **data, size_t *length)
{
    return napi_ok;
}

napi_status napi_cancel_async_work(napi_env env, napi_async_work work)
{
    return napi_ok;
}

napi_status napi_get_node_version(napi_env env, const napi_node_version **version)
{
    return napi_ok;
}

napi_status napi_get_uv_event_loop(napi_env env, struct uv_loop_s **loop)
{
    return napi_ok;
}

napi_status napi_get_null(napi_env env, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_array(napi_env env, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_uint32(napi_env env, uint32_t value, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_string_latin1(napi_env env, const char *str, size_t length, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_string_utf16(napi_env env, const char16_t *str, size_t length, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_symbol(napi_env env, napi_value description, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_function(napi_env env, const char *utf8name, size_t length, napi_callback cb, void *data,
    napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_type_error(napi_env env, napi_value code, napi_value msg, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_range_error(napi_env env, napi_value code, napi_value msg, napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_value_uint32(napi_env env, napi_value value, uint32_t *result)
{
    return napi_ok;
}

napi_status napi_get_value_int64(napi_env env, napi_value value, int64_t *result)
{
    return napi_ok;
}

napi_status napi_get_value_string_latin1(napi_env env, napi_value value, char *buf, size_t bufsize, size_t *result)
{
    return napi_ok;
}

napi_status napi_get_value_string_utf16(napi_env env, napi_value value, char16_t *buf, size_t bufsize, size_t *result)
{
    return napi_ok;
}

napi_status napi_coerce_to_bool(napi_env env, napi_value value, napi_value *result)
{
    return napi_ok;
}

napi_status napi_coerce_to_number(napi_env env, napi_value value, napi_value *result)
{
    return napi_ok;
}

napi_status napi_coerce_to_object(napi_env env, napi_value value, napi_value *result)
{
    return napi_ok;
}

napi_status napi_coerce_to_string(napi_env env, napi_value value, napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_prototype(napi_env env, napi_value object, napi_value *result)
{
    return napi_ok;
}

napi_status napi_set_property(napi_env env, napi_value object, napi_value key, napi_value value)
{
    return napi_ok;
}

napi_status napi_has_property(napi_env env, napi_value object, napi_value key, bool *result)
{
    return napi_ok;
}

napi_status napi_delete_property(napi_env env, napi_value object, napi_value key, bool *result)
{
    return napi_ok;
}

napi_status napi_has_own_property(napi_env env, napi_value object, napi_value key, bool *result)
{
    return napi_ok;
}

napi_status napi_has_named_property(napi_env env, napi_value object, const char *utf8name, bool *result)
{
    return napi_ok;
}

napi_status napi_has_element(napi_env env, napi_value object, uint32_t index, bool *result)
{
    return napi_ok;
}

napi_status napi_delete_element(napi_env env, napi_value object, uint32_t index, bool *result)
{
    return napi_ok;
}

napi_status napi_is_array(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_strict_equals(napi_env env, napi_value lhs, napi_value rhs, bool *result)
{
    return napi_ok;
}

napi_status napi_instanceof(napi_env env, napi_value object, napi_value constructor, bool *result)
{
    return napi_ok;
}

napi_status napi_remove_wrap(napi_env env, napi_value js_object, void **result)
{
    return napi_ok;
}

napi_status napi_create_external(napi_env env, void *data, napi_finalize finalize_cb, void *finalize_hint,
    napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_value_external(napi_env env, napi_value value, void **result)
{
    return napi_ok;
}

napi_status napi_open_handle_scope(napi_env env, napi_handle_scope *result)
{
    return napi_ok;
}

napi_status napi_close_handle_scope(napi_env env, napi_handle_scope scope)
{
    return napi_ok;
}

napi_status napi_open_escapable_handle_scope(napi_env env, napi_escapable_handle_scope *result)
{
    return napi_ok;
}

napi_status napi_close_escapable_handle_scope(napi_env env, napi_escapable_handle_scope scope)
{
    return napi_ok;
}

napi_status napi_escape_handle(napi_env env, napi_escapable_handle_scope scope, napi_value escapee, napi_value *result)
{
    return napi_ok;
}

napi_status napi_throw(napi_env env, napi_value error)
{
    return napi_ok;
}

napi_status napi_throw_type_error(napi_env env, const char *code, const char *msg)
{
    return napi_ok;
}

napi_status napi_throw_range_error(napi_env env, const char *code, const char *msg)
{
    return napi_ok;
}

napi_status napi_is_error(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_get_and_clear_last_exception(napi_env env, napi_value *result)
{
    return napi_ok;
}

napi_status napi_is_arraybuffer(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_create_external_arraybuffer(napi_env env, void *external_data, size_t byte_length,
    napi_finalize finalize_cb, void *finalize_hint, napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_arraybuffer_info(napi_env env, napi_value arraybuffer, void **data, size_t *byte_length)
{
    return napi_ok;
}

napi_status napi_create_dataview(napi_env env, size_t length, napi_value arraybuffer, size_t byte_offset,
    napi_value *result)
{
    return napi_ok;
}

napi_status napi_is_dataview(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_get_dataview_info(napi_env env, napi_value dataview, size_t *bytelength, void **data,
    napi_value *arraybuffer, size_t *byte_offset)
{
    return napi_ok;
}

napi_status napi_get_version(napi_env env, uint32_t *result)
{
    return napi_ok;
}

napi_status napi_is_promise(napi_env env, napi_value value, bool *is_promise)
{
    return napi_ok;
}

napi_status napi_run_script(napi_env env, napi_value script, napi_value *result)
{
    return napi_ok;
}

napi_status napi_adjust_external_memory(napi_env env, int64_t change_in_bytes, int64_t *adjusted_value)
{
    return napi_ok;
}

napi_status napi_object_freeze(napi_env env, napi_value object)
{
    return napi_ok;
}

napi_status napi_get_all_property_names(napi_env env, napi_value object, napi_key_collection_mode key_mode,
    napi_key_filter key_filter, napi_key_conversion key_conversion, napi_value *result)
{
    return napi_ok;
}

napi_status napi_add_finalizer(napi_env env, napi_value js_object, void *native_object, napi_finalize finalize_cb,
    void *finalize_hint, napi_ref *result)
{
    return napi_ok;
}

napi_status napi_create_bigint_words(napi_env env, int sign_bit, size_t word_count, const uint64_t *words,
    napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_value_bigint_words(napi_env env, napi_value value, int *sign_bit, size_t *word_count,
    uint64_t *words)
{
    return napi_ok;
}
}

napi_status napi_is_sendable(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_map(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_callable(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_create_runtime(napi_env env, napi_env *result_env)
{
    return napi_ok;
}

napi_status napi_serialize(napi_env env, napi_value object, napi_value transfer_list, napi_value *result)
{
    return napi_ok;
}

napi_status napi_deserialize(napi_env env, napi_value recorder, napi_value *object)
{
    return napi_ok;
}

napi_status napi_delete_serialization_data(napi_env env, napi_value value)
{
    return napi_ok;
}

napi_status napi_get_exception_info_for_worker(napi_env env, napi_value obj)
{
    return napi_ok;
}

napi_status napi_run_buffer_script(napi_env env, std::vector<uint8_t> &buffer, napi_value *result)
{
    return napi_ok;
}

napi_status napi_run_actor(napi_env env, std::vector<uint8_t> &buffer, const char *descriptor, napi_value *result)
{
    return napi_ok;
}

napi_status napi_set_promise_rejection_callback(napi_env env, napi_ref ref, napi_ref checkRef)
{
    return napi_ok;
}

napi_status napi_is_arguments_object(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_async_function(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_boolean_object(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_generator_function(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_date(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_map_iterator(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_set_iterator(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_generator_object(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_module_namespace_object(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_proxy(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_reg_exp(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_number_object(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_set(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_string_object(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_symbol_object(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_weak_map(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_weak_set(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_send_event(napi_env env, const std::function<void()> &cb, napi_event_priority priority)
{
    return napi_ok;
}

napi_status napi_send_event(napi_env env, const std::function<void()> &cb, napi_event_priority priority,
    const char *name, napi_event_barrier_option barrierOption)
{
    return napi_ok;
}

napi_status napi_create_limit_runtime(napi_env env, napi_env *result_env)
{
    return napi_ok;
}

void napi_module_with_js_register(napi_module_with_js *mod) {}

napi_status napi_serialize_inner(napi_env env, napi_value object, napi_value transfer_list, napi_value clone_list,
    bool defaultTransfer, bool defaultCloneSendable, void **result)
{
    return napi_ok;
}

napi_status napi_run_actor(napi_env env, uint8_t *buffer, size_t bufferSize, const char *descriptor,
    napi_value *result, char *entryPoint)
{
    return napi_ok;
}

napi_status napi_wrap_with_size(napi_env env, napi_value js_object, void *native_object, napi_finalize finalize_cb,
    void *finalize_hint, napi_ref *result, size_t native_binding_size)
{
    return napi_ok;
}

napi_status napi_wrap_async_finalizer(napi_env env, napi_value js_object, void *native_object,
    napi_finalize finalize_cb, void *finalize_hint, napi_ref *result, size_t native_binding_size)
{
    return napi_ok;
}

napi_status napi_create_external_with_size(napi_env env, void *data, napi_finalize finalize_cb, void *finalize_hint,
    napi_value *result, size_t native_binding_size)
{
    return napi_ok;
}

napi_status napi_is_big_int64_array(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_big_uint64_array(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_is_shared_array_buffer(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_get_stack_trace(napi_env env, std::string &stack)
{
    return napi_ok;
}

napi_status napi_get_hybrid_stack_trace(napi_env env, std::string &stack)
{
    return napi_ok;
}

napi_status napi_get_own_property_descriptor(napi_env env, napi_value object, const char *utf8name, napi_value *result)
{
    return napi_ok;
}

napi_status napi_object_get_keys(napi_env env, napi_value data, napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_print_string(napi_env env, napi_value value, std::string &result)
{
    return napi_ok;
}

napi_status napi_send_cancelable_event(napi_env env, const std::function<void(void *)> &cb, void *data,
    napi_event_priority priority, uint64_t *handleId, const char *name)
{
    return napi_ok;
}

napi_status napi_cancel_event(napi_env env, uint64_t handleId, const char *name)
{
    return napi_ok;
}

napi_status napi_open_fast_native_scope(napi_env env, napi_fast_native_scope *scope)
{
    return napi_ok;
}

napi_status napi_close_fast_native_scope(napi_env env, napi_fast_native_scope scope)
{
    return napi_ok;
}

napi_status napi_get_shared_array_buffer_info(napi_env env, napi_value arraybuffer, void **data, size_t *byte_length)
{
    return napi_ok;
}

napi_status napi_encode(napi_env env, napi_value src, napi_value *result)
{
    return napi_ok;
}

napi_status napi_is_bitvector(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_create_sendable_typedarray(napi_env env, napi_typedarray_type type, size_t length,
    napi_value arraybuffer, size_t byte_offset, napi_value *result)
{
    return napi_ok;
}

napi_status napi_map_get_size(napi_env env, napi_value map, uint32_t *result)
{
    return napi_ok;
}

napi_status napi_create_sendable_array_with_length(napi_env env, size_t length, napi_value *result)
{
    return napi_ok;
}

napi_status napi_fatal_exception(napi_env env, napi_value err)
{
    return napi_ok;
}

napi_status napi_type_tag_object(napi_env env, napi_value value, const napi_type_tag *type_tag)
{
    return napi_ok;
}

napi_status napi_check_object_type_tag(napi_env env, napi_value value, const napi_type_tag *type_tag, bool *result)
{
    return napi_ok;
}

napi_status napi_run_script_path(napi_env env, const char *path, napi_value *result)
{
    return napi_ok;
}

napi_status napi_load_module(napi_env env, const char *path, napi_value *result)
{
    return napi_ok;
}

napi_status napi_load_module_with_info(napi_env env, const char *path, const char *module_info, napi_value *result)
{
    return napi_ok;
}

napi_status napi_get_instance_data(napi_env env, void **data)
{
    return napi_ok;
}

napi_status napi_set_instance_data(napi_env env, void *data, napi_finalize finalize_cb, void *finalize_hint)
{
    return napi_ok;
}

napi_status napi_remove_env_cleanup_hook(napi_env env, void (*fun)(void *), void *arg)
{
    return napi_ok;
}

napi_status napi_add_env_cleanup_hook(napi_env env, void (*fun)(void *), void *arg)
{
    return napi_ok;
}

napi_status napi_remove_async_cleanup_hook(napi_async_cleanup_hook_handle remove_handle)
{
    return napi_ok;
}

napi_status napi_add_async_cleanup_hook(napi_env env, napi_async_cleanup_hook hook, void *arg,
    napi_async_cleanup_hook_handle *remove_handle)
{
    return napi_ok;
}

napi_status napi_close_callback_scope(napi_env env, napi_callback_scope scope)
{
    return napi_ok;
}

napi_status napi_open_callback_scope(napi_env env, napi_value resource_object, napi_async_context context,
    napi_callback_scope *result)
{
    return napi_ok;
}

napi_status node_api_get_module_file_name(napi_env env, const char **result)
{
    return napi_ok;
}

napi_status napi_create_object_with_named_properties(napi_env env, napi_value *result, size_t property_count,
    const char **keys, const napi_value *values)
{
    return napi_ok;
}

napi_status napi_coerce_to_native_binding_object(napi_env env, napi_value js_object,
    napi_native_binding_detach_callback detach_cb, napi_native_binding_attach_callback attach_cb, void *native_object,
    void *hint)
{
    return napi_ok;
}

napi_status napi_run_event_loop(napi_env env, napi_event_mode mode)
{
    return napi_ok;
}

napi_status napi_stop_event_loop(napi_env env)
{
    return napi_ok;
}

napi_status napi_create_ark_runtime(napi_env *env)
{
    return napi_ok;
}

napi_status napi_destroy_ark_runtime(napi_env *env)
{
    return napi_ok;
}

napi_status napi_serialize(napi_env env, napi_value object, napi_value transfer_list, napi_value clone_list,
    void **result)
{
    return napi_ok;
}

napi_status napi_deserialize(napi_env env, void *buffer, napi_value *object)
{
    return napi_ok;
}

napi_status napi_delete_serialization_data(napi_env env, void *buffer)
{
    return napi_ok;
}

napi_status napi_is_concurrent_function(napi_env env, napi_value value, bool *result)
{
    return napi_ok;
}

napi_status napi_call_threadsafe_function_with_priority(napi_threadsafe_function func, void *data,
    napi_task_priority priority, bool isTail)
{
    return napi_ok;
}

napi_status napi_create_map(napi_env env, napi_value *result)
{
    return napi_ok;
}

napi_status napi_map_set_property(napi_env env, napi_value map, napi_value key, napi_value value)
{
    return napi_ok;
}

napi_status napi_map_set_named_property(napi_env env, napi_value map, const char *utf8name, napi_value value)
{
    return napi_ok;
}

napi_status napi_map_get_property(napi_env env, napi_value map, napi_value key, napi_value *result)
{
    return napi_ok;
}

napi_status napi_map_get_named_property(napi_env env, napi_value map, const char *utf8name, napi_value *result)
{
    return napi_ok;
}

napi_status napi_map_has_property(napi_env env, napi_value map, napi_value key, bool *result)
{
    return napi_ok;
}

napi_status napi_map_has_named_property(napi_env env, napi_value map, const char *utf8name, bool *result)
{
    return napi_ok;
}

napi_status napi_map_delete_property(napi_env env, napi_value map, napi_value key)
{
    return napi_ok;
}

napi_status napi_map_clear(napi_env env, napi_value map)
{
    return napi_ok;
}

napi_status napi_map_get_entries(napi_env env, napi_value map, napi_value *result)
{
    return napi_ok;
}

napi_status napi_map_get_keys(napi_env env, napi_value map, napi_value *result)
{
    return napi_ok;
}

napi_status napi_map_get_values(napi_env env, napi_value map, napi_value *result)
{
    return napi_ok;
}

napi_status napi_map_iterator_get_next(napi_env env, napi_value iterator, napi_value *result)
{
    return napi_ok;
}

napi_status napi_define_sendable_class(napi_env env, const char *utf8name, size_t length, napi_callback constructor,
    void *data, size_t property_count, const napi_property_descriptor *properties, napi_value parent,
    napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_sendable_object_with_properties(napi_env env, size_t property_count,
    const napi_property_descriptor *properties, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_sendable_array(napi_env env, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_sendable_arraybuffer(napi_env env, size_t byte_length, void **data, napi_value *result)
{
    return napi_ok;
}

napi_status napi_create_sendable_map(napi_env env, napi_value *result)
{
    return napi_ok;
}

napi_status napi_wrap_sendable(napi_env env, napi_value js_object, void *native_object, napi_finalize finalize_cb,
    void *finalize_hint)
{
    return napi_ok;
}

napi_status napi_wrap_sendable_with_size(napi_env env, napi_value js_object, void *native_object,
    napi_finalize finalize_cb, void *finalize_hint, size_t native_binding_size)
{
    return napi_ok;
}

napi_status napi_unwrap_sendable(napi_env env, napi_value js_object, void **result)
{
    return napi_ok;
}

napi_status napi_remove_wrap_sendable(napi_env env, napi_value js_object, void **result)
{
    return napi_ok;
}

namespace OHOS {
namespace AbilityRuntime {

napi_status IsStageContext(napi_env env, napi_value value, bool &mode)
{
    mode = true;
    return napi_ok;
}

std::shared_ptr<Context> GetStageModeContext(napi_env env, napi_value value)
{
    return std::make_shared<ExtensionContext>();
}

std::shared_ptr<AppExecFwk::Ability> GetCurrentAbility(napi_env env)
{
    return std::make_shared<AppExecFwk::Ability>();
}

}  // namespace AbilityRuntime

namespace AppExecFwk {

Ability* Ability::Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    return nullptr;
}

void Ability::OnStart() {}
void Ability::OnStop() {}
void Ability::OnActive() {}
void Ability::OnInactive() {}
void Ability::OnForeground() {}
void Ability::OnBackground() {}
void Ability::OnConfigurationUpdated() {}

}  // namespace AppExecFwk
}  // namespace OHOS

extern "C" {
int uv_queue_work_internal(uv_loop_t* loop, uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb, const char* task_name)
{
    return 0;
}
}
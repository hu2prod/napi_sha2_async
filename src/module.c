#include <node_api.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "sha2.c"

napi_value sha2_sync(napi_env env, napi_callback_info info) {
  napi_status status;
  
  napi_value ret_dummy;
  status = napi_create_int32(env, 0, &ret_dummy);
  
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to create return value ret_dummy");
    return ret_dummy;
  }
  
  size_t argc = 2;
  napi_value argv[2];
  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to parse arguments");
    return ret_dummy;
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  u8 *data_src;
  size_t data_src_len;
  status = napi_get_buffer_info(env, argv[0], (void**)&data_src, &data_src_len);
  
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid buffer was passed as argument of data_src");
    return ret_dummy;
  }
  
  u8 *data_dst;
  size_t data_dst_len;
  status = napi_get_buffer_info(env, argv[1], (void**)&data_dst, &data_dst_len);
  
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid buffer was passed as argument of data_dst");
    return ret_dummy;
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  if (data_dst_len != 32) {
    printf("data_dst_len %ld\n", data_dst_len);
    napi_throw_error(env, NULL, "Invalid buffer was passed as argument of data_dst; length != 32");
    return ret_dummy;
  }
  
  sph_sha256_context ctx;
  sph_sha256_init(&ctx);
  sph_sha256(&ctx, data_src, data_src_len);
  sph_sha256_close(&ctx, data_dst);
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  return ret_dummy;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
struct Work_data {
  u8* data_src;
  size_t data_src_len;
  u8* data_dst;
  
  const char* error;
  napi_ref data_dst_reference;
  napi_ref callback_reference;
};
void work_data_free(struct Work_data* data) {
  free(data->data_src);
  free(data->data_dst);
  free(data);
}

void napi_helper_error_cb(napi_env env, const char* error_str, napi_value callback) {
  napi_status status;
  napi_value global;
  status = napi_get_global(env, &global);
  if (status != napi_ok) {
    printf("status = %d\n", status);
    napi_throw_error(env, NULL, "Unable to create return value global (napi_get_global)");
    return;
  }
  
  napi_value call_argv[1];
  
  // napi_value error_code;
  // status = napi_create_int32(env, 1, &error_code);
  // if (status != napi_ok) {
    // printf("status = %d\n", status);
    // napi_throw_error(env, NULL, "!napi_create_int32");
    // return;
  // }
  
  napi_value error;
  status = napi_create_string_utf8(env, error_str, strlen(error_str), &error);
  if (status != napi_ok) {
    printf("status = %d\n", status);
    napi_throw_error(env, NULL, "!napi_create_string_utf8");
    return;
  }
  
  status = napi_create_error(env,
    NULL,
    error,
    &call_argv[0]);
  if (status != napi_ok) {
    printf("status = %d\n", status);
    printf("error  = %s\n", error_str);
    napi_throw_error(env, NULL, "!napi_create_error");
    return;
  }
  
  napi_value result;
  status = napi_call_function(env, global, callback, 1, call_argv, &result);
  if (status != napi_ok) {
    // это нормальная ошибка если основной поток падает
    napi_throw_error(env, NULL, "!napi_call_function");
    return;
  }
  return;
}


void execute_sha2(napi_env env, void* _data) {
  struct Work_data* data = (struct Work_data*)_data;
  sph_sha256_context ctx;
  sph_sha256_init(&ctx);
  sph_sha256(&ctx, data->data_src, data->data_src_len);
  sph_sha256_close(&ctx, data->data_dst);
}



void complete(napi_env env, napi_status execute_status, void* _data) {
  napi_status status;
  struct Work_data* worker_ctx = (struct Work_data*)_data;
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  //    prepare for callback (common parts)
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  napi_value callback;
  status = napi_get_reference_value(env, worker_ctx->callback_reference, &callback);
  if (status != napi_ok) {
    printf("status = %d\n", status);
    napi_throw_error(env, NULL, "Unable to get referenced callback (napi_get_reference_value)");
    work_data_free(worker_ctx);
    return;
  }
  napi_value data_dst_val;
  status = napi_get_reference_value(env, worker_ctx->data_dst_reference, &data_dst_val);
  if (status != napi_ok) {
    printf("status = %d\n", status);
    napi_throw_error(env, NULL, "Unable to get referenced callback (napi_get_reference_value)");
    work_data_free(worker_ctx);
    return;
  }
  u8 *data_dst;
  size_t data_dst_len;
  status = napi_get_buffer_info(env, data_dst_val, (void**)&data_dst, &data_dst_len);
  
  if (status != napi_ok) {
    printf("status = %d\n", status);
    napi_throw_error(env, NULL, "Invalid buffer was passed as argument of data_dst");
    work_data_free(worker_ctx);
    return;
  }
  
  
  napi_value global;
  status = napi_get_global(env, &global);
  if (status != napi_ok) {
    printf("status = %d\n", status);
    napi_throw_error(env, NULL, "Unable to create return value global (napi_get_global)");
    work_data_free(worker_ctx);
    return;
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  if (execute_status != napi_ok) {
    // чтобы не дублировать код
    if (!worker_ctx->error) {
      worker_ctx->error = "execute_status != napi_ok";
    }
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  if (worker_ctx->error) {
    napi_helper_error_cb(env, worker_ctx->error, callback);
    work_data_free(worker_ctx);
    return;
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  //    callback OK
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  napi_value result;
  napi_value call_argv[0];
  
  // worker_ctx->data_dst_len будет всегда 32
  memcpy(data_dst, worker_ctx->data_dst, 32);
  
  status = napi_call_function(env, global, callback, 0, call_argv, &result);
  if (status != napi_ok) {
    fprintf(stderr, "status = %d\n", status);
    napi_throw_error(env, NULL, "napi_call_function FAIL");
    work_data_free(worker_ctx);
    return;
  }
  work_data_free(worker_ctx);
}


napi_value sha2(napi_env env, napi_callback_info info) {
  napi_status status;
  
  napi_value ret_dummy;
  status = napi_create_int32(env, 0, &ret_dummy);
  
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to create return value ret_dummy");
    return ret_dummy;
  }
  
  size_t argc = 3;
  napi_value argv[3];
  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to parse arguments");
    return ret_dummy;
  }
  
  /*//////////////////////////////////////////////////////////////////////////////////////////////////*/
  
  u8 *data_src;
  size_t data_src_len;
  status = napi_get_buffer_info(env, argv[0], (void**)&data_src, &data_src_len);
  
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid buffer was passed as argument of data_src");
    return ret_dummy;
  }
  
  u8 *data_dst;
  size_t data_dst_len;
  status = napi_get_buffer_info(env, argv[1], (void**)&data_dst, &data_dst_len);
  
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid buffer was passed as argument of data_dst");
    return ret_dummy;
  }
  napi_value data_dst_val = argv[1];
  
  napi_value callback = argv[2];
  /*//////////////////////////////////////////////////////////////////////////////////////////////////*/
  if (data_dst_len != 32) {
    printf("data_dst_len %ld\n", data_dst_len);
    napi_helper_error_cb(env, "Invalid buffer was passed as argument of data_dst; length != 32", callback);
    return ret_dummy;
  }
  
  struct Work_data* worker_ctx = (struct Work_data*)malloc(sizeof(struct Work_data));
  worker_ctx->error = NULL;
  
  worker_ctx->data_src = (u8*)malloc(data_src_len);
  memcpy(worker_ctx->data_src, data_src, data_src_len);
  worker_ctx->data_src_len = data_src_len;
  
  worker_ctx->data_dst = (u8*)malloc(data_dst_len);
  
  status = napi_create_reference(env, callback, 1, &worker_ctx->callback_reference);
  if (status != napi_ok) {
    printf("status = %d\n", status);
    napi_throw_error(env, NULL, "Unable to create reference for callback. napi_create_reference");
    work_data_free(worker_ctx);
    return ret_dummy;
  }
  /* EXTRA */
  status = napi_create_reference(env, data_dst_val, 1, &worker_ctx->data_dst_reference);
  if (status != napi_ok) {
    printf("status = %d\n", status);
    napi_throw_error(env, NULL, "Unable to create reference for callback. napi_create_reference");
    work_data_free(worker_ctx);
    return ret_dummy;
  }
  
  napi_value async_resource_name;
  status = napi_create_string_utf8(env, "dummy", 5, &async_resource_name);
  if (status != napi_ok) {
    printf("status = %d\n", status);
    napi_throw_error(env, NULL, "Unable to create value async_resource_name set to \"dummy\"");
    work_data_free(worker_ctx);
    return ret_dummy;
  }
  
  napi_async_work work;
  status = napi_create_async_work(env,
                                   NULL,
                                   async_resource_name,
                                   execute_sha2,
                                   complete,
                                   (void*)worker_ctx,
                                   &work);
  if (status != napi_ok) {
    printf("status = %d\n", status);
    napi_throw_error(env, NULL, "napi_create_async_work fail");
    work_data_free(worker_ctx);
    return ret_dummy;
  }
  
  status = napi_queue_async_work(env, work);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "napi_queue_async_work fail");
    work_data_free(worker_ctx);
    return ret_dummy;
  }
  
  /*//////////////////////////////////////////////////////////////////////////////////////////////////*/
  return ret_dummy;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_value fn;
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  status = napi_create_function(env, NULL, 0, sha2_sync, NULL, &fn);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }
  
  status = napi_set_named_property(env, exports, "sha2_no_alloc_sync", fn);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  status = napi_create_function(env, NULL, 0, sha2, NULL, &fn);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }
  
  status = napi_set_named_property(env, exports, "sha2_no_alloc", fn);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

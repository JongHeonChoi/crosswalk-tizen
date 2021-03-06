/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "extensions/renderer/runtime_ipc_client.h"

#include "common/logger.h"
#include "common/profiler.h"
#include "common/string_utils.h"

namespace extensions {

namespace {

const int kRoutingIdEmbedderDataIndex = 12;

}  // namespace

RuntimeIPCClient::JSCallback::JSCallback(v8::Isolate* isolate,
                                         v8::Handle<v8::Function> callback) {
  callback_.Reset(isolate, callback);
}

RuntimeIPCClient::JSCallback::~JSCallback() {
  callback_.Reset();
}

void RuntimeIPCClient::JSCallback::Call(v8::Isolate* isolate,
                                        v8::Handle<v8::Value> args[]) {
  if (!callback_.IsEmpty()) {
    v8::HandleScope handle_scope(isolate);
    v8::TryCatch try_catch(isolate);
    v8::Handle<v8::Function> func =
        v8::Local<v8::Function>::New(isolate, callback_);
    func->Call(func, 1, args);
    if (try_catch.HasCaught()) {
      LOGGER(ERROR) << "Exception when running Javascript callback";
      v8::String::Utf8Value exception_str(try_catch.Exception());
      LOGGER(ERROR) << (*exception_str);
    }
  }
}

// static
RuntimeIPCClient* RuntimeIPCClient::GetInstance() {
  static RuntimeIPCClient self;
  return &self;
}

RuntimeIPCClient::RuntimeIPCClient() {
}

int RuntimeIPCClient::GetRoutingId(v8::Handle<v8::Context> context) {
  v8::Handle<v8::Value> value =
      context->GetEmbedderData(kRoutingIdEmbedderDataIndex);
  int routing_id = 0;
  if (value->IsNumber()) {
    routing_id = value->IntegerValue();
  } else {
    LOGGER(WARN) << "Failed to get routing index from context.";
  }

  return routing_id;
}

void RuntimeIPCClient::SetRoutingId(v8::Handle<v8::Context> context,
                                    int routing_id) {
  context->SetEmbedderData(kRoutingIdEmbedderDataIndex,
                           v8::Integer::New(context->GetIsolate(), routing_id));
}

void RuntimeIPCClient::SendMessage(v8::Handle<v8::Context> context,
                                   const std::string& type,
                                   const std::string& value) {
  SendMessage(context, type, "", "", value);
}

void RuntimeIPCClient::SendMessage(v8::Handle<v8::Context> context,
                                   const std::string& type,
                                   const std::string& id,
                                   const std::string& value) {
  SendMessage(context, type, id, "", value);
}

void RuntimeIPCClient::SendMessage(v8::Handle<v8::Context> context,
                                   const std::string& type,
                                   const std::string& id,
                                   const std::string& ref_id,
                                   const std::string& value) {
  int routing_id = GetRoutingId(context);
  if (routing_id < 1) {
    LOGGER(ERROR) << "Invalid routing handle for IPC.";
    return;
  }

  Ewk_IPC_Wrt_Message_Data* msg = ewk_ipc_wrt_message_data_new();
  ewk_ipc_wrt_message_data_type_set(msg, type.c_str());
  ewk_ipc_wrt_message_data_id_set(msg, id.c_str());
  ewk_ipc_wrt_message_data_reference_id_set(msg, ref_id.c_str());
  ewk_ipc_wrt_message_data_value_set(msg, value.c_str());

  if (!ewk_ipc_plugins_message_send(routing_id, msg)) {
    LOGGER(ERROR) << "Failed to send message to runtime using ewk_ipc.";
  }

  ewk_ipc_wrt_message_data_del(msg);
}

std::string RuntimeIPCClient::SendSyncMessage(v8::Handle<v8::Context> context,
                                              const std::string& type,
                                              const std::string& value) {
  return SendSyncMessage(context, type, "", "", value);
}

std::string RuntimeIPCClient::SendSyncMessage(v8::Handle<v8::Context> context,
                                              const std::string& type,
                                              const std::string& id,
                                              const std::string& value) {
  return SendSyncMessage(context, type, id, "", value);
}

std::string RuntimeIPCClient::SendSyncMessage(v8::Handle<v8::Context> context,
                                              const std::string& type,
                                              const std::string& id,
                                              const std::string& ref_id,
                                              const std::string& value) {
  int routing_id = GetRoutingId(context);
  if (routing_id < 1) {
    LOGGER(ERROR) << "Invalid routing handle for IPC.";
    return std::string();
  }

  Ewk_IPC_Wrt_Message_Data* msg = ewk_ipc_wrt_message_data_new();
  ewk_ipc_wrt_message_data_type_set(msg, type.c_str());
  ewk_ipc_wrt_message_data_id_set(msg, id.c_str());
  ewk_ipc_wrt_message_data_reference_id_set(msg, ref_id.c_str());
  ewk_ipc_wrt_message_data_value_set(msg, value.c_str());

  if (!ewk_ipc_plugins_sync_message_send(routing_id, msg)) {
    LOGGER(ERROR) << "Failed to send message to runtime using ewk_ipc.";
    ewk_ipc_wrt_message_data_del(msg);
    return std::string();
  }

  Eina_Stringshare* msg_value = ewk_ipc_wrt_message_data_value_get(msg);
  std::string result(msg_value);
  eina_stringshare_del(msg_value);

  ewk_ipc_wrt_message_data_del(msg);

  return result;
}

void RuntimeIPCClient::SendAsyncMessage(v8::Handle<v8::Context> context,
                                        const std::string& type,
                                        const std::string& value,
                                        ReplyCallback callback) {
  int routing_id = GetRoutingId(context);
  if (routing_id < 1) {
    LOGGER(ERROR) << "Invalid routing handle for IPC.";
    return;
  }

  std::string msg_id = common::utils::GenerateUUID();

  Ewk_IPC_Wrt_Message_Data* msg = ewk_ipc_wrt_message_data_new();
  ewk_ipc_wrt_message_data_id_set(msg, msg_id.c_str());
  ewk_ipc_wrt_message_data_type_set(msg, type.c_str());
  ewk_ipc_wrt_message_data_value_set(msg, value.c_str());

  if (!ewk_ipc_plugins_message_send(routing_id, msg)) {
    LOGGER(ERROR) << "Failed to send message to runtime using ewk_ipc.";
    ewk_ipc_wrt_message_data_del(msg);
    return;
  }

  callbacks_[msg_id] = callback;

  ewk_ipc_wrt_message_data_del(msg);
}

void RuntimeIPCClient::HandleMessageFromRuntime(
    const Ewk_IPC_Wrt_Message_Data* msg) {
  if (msg == NULL) {
    LOGGER(ERROR) << "received message is NULL";
    return;
  }

  Eina_Stringshare* msg_refid = ewk_ipc_wrt_message_data_reference_id_get(msg);

  if (msg_refid == NULL || !strcmp(msg_refid, "")) {
    if (msg_refid) eina_stringshare_del(msg_refid);
    LOGGER(ERROR) << "No reference id of received message.";
    return;
  }

  auto it = callbacks_.find(msg_refid);
  if (it == callbacks_.end()) {
    eina_stringshare_del(msg_refid);
    LOGGER(ERROR) << "No registered callback with reference id : " << msg_refid;
    return;
  }

  Eina_Stringshare* msg_type = ewk_ipc_wrt_message_data_type_get(msg);
  Eina_Stringshare* msg_value = ewk_ipc_wrt_message_data_value_get(msg);

  ReplyCallback func = it->second;
  if (func) {
    func(msg_type, msg_value);
  }

  callbacks_.erase(it);

  eina_stringshare_del(msg_refid);
  eina_stringshare_del(msg_type);
  eina_stringshare_del(msg_value);
}

}  // namespace extensions

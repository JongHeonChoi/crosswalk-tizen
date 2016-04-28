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

#ifndef XWALK_RUNTIME_BROWSER_IME_RUNTIME_H_
#define XWALK_RUNTIME_BROWSER_IME_RUNTIME_H_

#include <app.h>
#include <inputmethod.h>
#include <string>

#include "runtime/browser/native_window.h"
#include "runtime/browser/ime_application.h"

namespace runtime {

class ImeRuntime {
 public:
  ImeRuntime(std::unique_ptr<common::ApplicationData> app_data);
  virtual ~ImeRuntime();

  virtual int Exec(int argc, char* argv[]);

  virtual void OnCreate();
  virtual void OnTerminate();
  virtual void OnShow(int context_id, ime_context_h context);
  virtual void OnHide(int context_id);
  virtual void OnAppControl();

 private:
  ImeApplication* application_;
  NativeWindow* native_window_;
  std::string app_id_;
  std::unique_ptr<common::ApplicationData> app_data_;
};

}  // namespace runtime

#endif  // XWALK_RUNTIME_BROWSER_IME_RUNTIME_H_

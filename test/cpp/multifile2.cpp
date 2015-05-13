/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2015 NAN contributors
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 ********************************************************************/

#include <nan.h>

NAN_METHOD(ReturnString) {
  v8::Local<v8::String> s = NanNew<v8::String>(*v8::String::Utf8Value(args[0]));
  NanReturnValue(s);
}

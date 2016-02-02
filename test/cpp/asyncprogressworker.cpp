/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2016 NAN contributors
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 ********************************************************************/

#ifndef _WIN32
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif
#include <nan.h>
#include <string>
#include <sstream>

using namespace Nan;  // NOLINT(build/namespaces)

class ProgressWorker : public AsyncProgressWorker {
 public:
  ProgressWorker(
      Callback *callback
    , Callback *progress
    , int milliseconds
    , int iters)
    : AsyncProgressWorker(callback), progress(progress)
    , milliseconds(milliseconds), iters(iters) {}
  ~ProgressWorker() {}

  void Execute (const AsyncProgressWorker::ExecutionProgress& progress) {
    for (int i = 0; i < iters; ++i) {
      progress.Send(reinterpret_cast<const char*>(&i), sizeof(int));
      Sleep(milliseconds);
    }
  }

  void HandleProgressCallback(const char *data, size_t size) {
    HandleScope scope;

    v8::Local<v8::Value> argv[] = {
        New<v8::Integer>(*reinterpret_cast<int*>(const_cast<char*>(data)))
    };
    progress->Call(1, argv);
  }

 private:
  Callback *progress;
  int milliseconds;
  int iters;
};

class ProgressWorkerNoWait : public AsyncProgressWorker {
 public:
  ProgressWorkerNoWait(
      Callback *callback
    , Callback *progress
    , int iters)
    : AsyncProgressWorker(callback), progress(progress)
    , iters(iters) {}
  ~ProgressWorkerNoWait() {}

  void Execute (const AsyncProgressWorker::ExecutionProgress& progress) {
    for (int i = 0; i < iters; ++i) {
      std::stringstream ss;
      ss << i;
      const std::string& s = ss.str();
      progress.Send(s.c_str(), s.length()+1);
    }
  }

  void HandleProgressCallback(const char *data, size_t size) {
    HandleScope scope;

    v8::Local<v8::Value> argv[] = {
      New<v8::String>(data).ToLocalChecked()
    };
    progress->Call(1, argv);
  }

 private:
  Callback *progress;
  int iters;
};

NAN_METHOD(DoProgress) {
  Callback *progress = new Callback(info[2].As<v8::Function>());
  Callback *callback = new Callback(info[3].As<v8::Function>());
  AsyncQueueWorker(new ProgressWorker(
      callback
    , progress
    , To<uint32_t>(info[0]).FromJust()
    , To<uint32_t>(info[1]).FromJust()));
}

NAN_METHOD(DoProgressNoWait) {
  Callback *progress = new Callback(info[1].As<v8::Function>());
  Callback *callback = new Callback(info[2].As<v8::Function>());
  AsyncQueueWorker(new ProgressWorkerNoWait(
      callback
    , progress
    , To<uint32_t>(info[0]).FromJust()));
}

NAN_MODULE_INIT(Init) {
  Set(target
    , New<v8::String>("a").ToLocalChecked()
    , New<v8::FunctionTemplate>(DoProgress)->GetFunction());
  Set(target
    , New<v8::String>("b").ToLocalChecked()
    , New<v8::FunctionTemplate>(DoProgressNoWait)->GetFunction());
}

NODE_MODULE(asyncprogressworker, Init)

/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2015 NAN contributors:
 *   - Rod Vagg <https://github.com/rvagg>
 *   - Benjamin Byholm <https://github.com/kkoopa>
 *   - Trevor Norris <https://github.com/trevnorris>
 *   - Nathan Rajlich <https://github.com/TooTallNate>
 *   - Brett Lawson <https://github.com/brett19>
 *   - Ben Noordhuis <https://github.com/bnoordhuis>
 *   - David Siegel <https://github.com/agnat>
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 *
 * Version 1.8.4: current Node 12: 0.12.2, Node 10: 0.10.38, io.js: 1.8.1
 *
 * See https://github.com/nodejs/nan for the latest update to this file
 **********************************************************************************/

#ifndef NAN_H_
#define NAN_H_

#include <uv.h>
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>
#include <node_object_wrap.h>
#include <cstring>
#include <climits>
#include <cstdlib>
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4530 )
# include <string>
# pragma warning( pop )
#else
# include <string>
#endif

#if defined(__GNUC__) && !(defined(DEBUG) && DEBUG)
# define NAN_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER) && !(defined(DEBUG) && DEBUG)
# define NAN_INLINE __forceinline
#else
# define NAN_INLINE inline
#endif

#if defined(__GNUC__) && \
    !(defined(V8_DISABLE_DEPRECATIONS) && V8_DISABLE_DEPRECATIONS)
# define NAN_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER) && \
    !(defined(V8_DISABLE_DEPRECATIONS) && V8_DISABLE_DEPRECATIONS)
# define NAN_DEPRECATED __declspec(deprecated)
#else
# define NAN_DEPRECATED
#endif

#if __cplusplus >= 201103L
# define NAN_DISALLOW_ASSIGN(CLASS) void operator=(const CLASS&) = delete;
# define NAN_DISALLOW_COPY(CLASS) CLASS(const CLASS&) = delete;
# define NAN_DISALLOW_MOVE(CLASS)                                              \
    CLASS(CLASS&&) = delete;  /* NOLINT(build/c++11) */                        \
    void operator=(CLASS&&) = delete;
#else
# define NAN_DISALLOW_ASSIGN(CLASS) void operator=(const CLASS&);
# define NAN_DISALLOW_COPY(CLASS) CLASS(const CLASS&);
# define NAN_DISALLOW_MOVE(CLASS)
#endif

#define NAN_DISALLOW_ASSIGN_COPY(CLASS)                                        \
    NAN_DISALLOW_ASSIGN(CLASS)                                                 \
    NAN_DISALLOW_COPY(CLASS)

#define NAN_DISALLOW_ASSIGN_MOVE(CLASS)                                        \
    NAN_DISALLOW_ASSIGN(CLASS)                                                 \
    NAN_DISALLOW_MOVE(CLASS)

#define NAN_DISALLOW_COPY_MOVE(CLASS)                                          \
    NAN_DISALLOW_COPY(CLASS)                                                   \
    NAN_DISALLOW_MOVE(CLASS)

#define NAN_DISALLOW_ASSIGN_COPY_MOVE(CLASS)                                   \
    NAN_DISALLOW_ASSIGN(CLASS)                                                 \
    NAN_DISALLOW_COPY(CLASS)                                                   \
    NAN_DISALLOW_MOVE(CLASS)

#define NODE_0_10_MODULE_VERSION 11
#define NODE_0_12_MODULE_VERSION 12
#define ATOM_0_21_MODULE_VERSION 41
#define IOJS_1_0_MODULE_VERSION  42
#define IOJS_1_1_MODULE_VERSION  43

#if (NODE_MODULE_VERSION < NODE_0_12_MODULE_VERSION)
typedef v8::InvocationCallback NanFunctionCallback;
typedef v8::Script             NanUnboundScript;
typedef v8::Script             NanBoundScript;
#else
typedef v8::FunctionCallback   NanFunctionCallback;
typedef v8::UnboundScript      NanUnboundScript;
typedef v8::Script             NanBoundScript;
#endif

#if (NODE_MODULE_VERSION < ATOM_0_21_MODULE_VERSION)
typedef v8::String::ExternalAsciiStringResource
    NanExternalOneByteStringResource;
#else
typedef v8::String::ExternalOneByteStringResource
    NanExternalOneByteStringResource;
#endif

#include "nan_new.h"  // NOLINT(build/include)

// uv helpers
#ifdef UV_VERSION_MAJOR
#ifndef UV_VERSION_PATCH
#define UV_VERSION_PATCH 0
#endif
#define NAUV_UVVERSION  ((UV_VERSION_MAJOR << 16) | \
                     (UV_VERSION_MINOR <<  8) | \
                     (UV_VERSION_PATCH))
#else
#define NAUV_UVVERSION 0x000b00
#endif


#if NAUV_UVVERSION < 0x000b17
#define NAUV_WORK_CB(func) \
    void func(uv_async_t *async, int)
#else
#define NAUV_WORK_CB(func) \
    void func(uv_async_t *async)
#endif

#if NAUV_UVVERSION >= 0x000b0b

typedef uv_key_t nauv_key_t;

inline int nauv_key_create(nauv_key_t *key) {
  return uv_key_create(key);
}

inline void nauv_key_delete(nauv_key_t *key) {
  uv_key_delete(key);
}

inline void* nauv_key_get(nauv_key_t *key) {
  return uv_key_get(key);
}

inline void nauv_key_set(nauv_key_t *key, void *value) {
  uv_key_set(key, value);
}

#else

/* Implement thread local storage for older versions of libuv.
 * This is essentially a backport of libuv commit 5d2434bf
 * written by Ben Noordhuis, adjusted for names and inline.
 */

#ifndef WIN32

#include <pthread.h>

typedef pthread_key_t nauv_key_t;

inline int nauv_key_create(nauv_key_t* key) {
  return -pthread_key_create(key, NULL);
}

inline void nauv_key_delete(nauv_key_t* key) {
  if (pthread_key_delete(*key))
    abort();
}

inline void* nauv_key_get(nauv_key_t* key) {
  return pthread_getspecific(*key);
}

inline void nauv_key_set(nauv_key_t* key, void* value) {
  if (pthread_setspecific(*key, value))
    abort();
}

#else

#include <windows.h>

typedef struct {
  DWORD tls_index;
} nauv_key_t;

inline int nauv_key_create(nauv_key_t* key) {
  key->tls_index = TlsAlloc();
  if (key->tls_index == TLS_OUT_OF_INDEXES)
    return UV_ENOMEM;
  return 0;
}

inline void nauv_key_delete(nauv_key_t* key) {
  if (TlsFree(key->tls_index) == FALSE)
    abort();
  key->tls_index = TLS_OUT_OF_INDEXES;
}

inline void* nauv_key_get(nauv_key_t* key) {
  void* value = TlsGetValue(key->tls_index);
  if (value == NULL)
    if (GetLastError() != ERROR_SUCCESS)
      abort();
  return value;
}

inline void nauv_key_set(nauv_key_t* key, void* value) {
  if (TlsSetValue(key->tls_index, value) == FALSE)
    abort();
}

#endif
#endif

template<typename T>
v8::Local<T> NanNew(v8::Handle<T>);

namespace Nan { namespace imp {
  template<typename T>
  NAN_INLINE v8::Persistent<T> &NanEnsureHandleOrPersistent(
      v8::Persistent<T> &val) {  // NOLINT(runtime/references)
    return val;
  }

  template<typename T>
  NAN_INLINE
  v8::Handle<T> NanEnsureHandleOrPersistent(const v8::Handle<T> &val) {
    return val;
  }

  template<typename T>
  NAN_INLINE v8::Local<T> NanEnsureHandleOrPersistent(const v8::Local<T> &val) {
    return val;
  }

  template<typename T>
  NAN_INLINE v8::Local<v8::Value> NanEnsureHandleOrPersistent(const T &val) {
    return NanNew(val);
  }

  template<typename T>
  NAN_INLINE v8::Local<T> NanEnsureLocal(const v8::Local<T> &val) {
    return val;
  }

  template<typename T>
  NAN_INLINE v8::Local<T> NanEnsureLocal(const v8::Persistent<T> &val) {
    return NanNew(val);
  }

  template<typename T>
  NAN_INLINE v8::Local<T> NanEnsureLocal(const v8::Handle<T> &val) {
    return NanNew(val);
  }

  template<typename T>
  NAN_INLINE v8::Local<v8::Value> NanEnsureLocal(const T &val) {
    return NanNew(val);
  }
}  // end of namespace imp
}  // end of namespace Nan

/* io.js 1.0  */
#if NODE_MODULE_VERSION >= IOJS_1_0_MODULE_VERSION \
  || NODE_VERSION_AT_LEAST(0, 11, 15)
  NAN_INLINE
  void NanSetCounterFunction(v8::CounterLookupCallback cb) {
    v8::Isolate::GetCurrent()->SetCounterFunction(cb);
  }

  NAN_INLINE
  void NanSetCreateHistogramFunction(v8::CreateHistogramCallback cb) {
    v8::Isolate::GetCurrent()->SetCreateHistogramFunction(cb);
  }

  NAN_INLINE
  void NanSetAddHistogramSampleFunction(v8::AddHistogramSampleCallback cb) {
    v8::Isolate::GetCurrent()->SetAddHistogramSampleFunction(cb);
  }

  NAN_INLINE bool NanIdleNotification(int idle_time_in_ms) {
    return v8::Isolate::GetCurrent()->IdleNotification(idle_time_in_ms);
  }

  NAN_INLINE void NanLowMemoryNotification() {
    v8::Isolate::GetCurrent()->LowMemoryNotification();
  }

  NAN_INLINE void NanContextDisposedNotification() {
    v8::Isolate::GetCurrent()->ContextDisposedNotification();
  }
#else
  NAN_INLINE
  void NanSetCounterFunction(v8::CounterLookupCallback cb) {
    v8::V8::SetCounterFunction(cb);
  }

  NAN_INLINE
  void NanSetCreateHistogramFunction(v8::CreateHistogramCallback cb) {
    v8::V8::SetCreateHistogramFunction(cb);
  }

  NAN_INLINE
  void NanSetAddHistogramSampleFunction(v8::AddHistogramSampleCallback cb) {
    v8::V8::SetAddHistogramSampleFunction(cb);
  }

  NAN_INLINE bool NanIdleNotification(int idle_time_in_ms) {
    return v8::V8::IdleNotification(idle_time_in_ms);
  }

  NAN_INLINE void NanLowMemoryNotification() {
    v8::V8::LowMemoryNotification();
  }

  NAN_INLINE void NanContextDisposedNotification() {
    v8::V8::ContextDisposedNotification();
  }
#endif

#if (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
// Node 0.11+ (0.11.12 and below won't compile with these)

# define _NAN_METHOD_ARGS_TYPE const v8::FunctionCallbackInfo<v8::Value>&
# define _NAN_METHOD_ARGS _NAN_METHOD_ARGS_TYPE args
# define _NAN_METHOD_RETURN_TYPE void

# define _NAN_GETTER_ARGS_TYPE const v8::PropertyCallbackInfo<v8::Value>&
# define _NAN_GETTER_ARGS _NAN_GETTER_ARGS_TYPE args
# define _NAN_GETTER_RETURN_TYPE void

# define _NAN_SETTER_ARGS_TYPE const v8::PropertyCallbackInfo<void>&
# define _NAN_SETTER_ARGS _NAN_SETTER_ARGS_TYPE args
# define _NAN_SETTER_RETURN_TYPE void

# define _NAN_PROPERTY_GETTER_ARGS_TYPE                                        \
    const v8::PropertyCallbackInfo<v8::Value>&
# define _NAN_PROPERTY_GETTER_ARGS _NAN_PROPERTY_GETTER_ARGS_TYPE args
# define _NAN_PROPERTY_GETTER_RETURN_TYPE void

# define _NAN_PROPERTY_SETTER_ARGS_TYPE                                        \
    const v8::PropertyCallbackInfo<v8::Value>&
# define _NAN_PROPERTY_SETTER_ARGS _NAN_PROPERTY_SETTER_ARGS_TYPE args
# define _NAN_PROPERTY_SETTER_RETURN_TYPE void

# define _NAN_PROPERTY_ENUMERATOR_ARGS_TYPE                                    \
    const v8::PropertyCallbackInfo<v8::Array>&
# define _NAN_PROPERTY_ENUMERATOR_ARGS _NAN_PROPERTY_ENUMERATOR_ARGS_TYPE args
# define _NAN_PROPERTY_ENUMERATOR_RETURN_TYPE void

# define _NAN_PROPERTY_DELETER_ARGS_TYPE                                       \
    const v8::PropertyCallbackInfo<v8::Boolean>&
# define _NAN_PROPERTY_DELETER_ARGS                                            \
    _NAN_PROPERTY_DELETER_ARGS_TYPE args
# define _NAN_PROPERTY_DELETER_RETURN_TYPE void

# define _NAN_PROPERTY_QUERY_ARGS_TYPE                                         \
    const v8::PropertyCallbackInfo<v8::Integer>&
# define _NAN_PROPERTY_QUERY_ARGS _NAN_PROPERTY_QUERY_ARGS_TYPE args
# define _NAN_PROPERTY_QUERY_RETURN_TYPE void

# define _NAN_INDEX_GETTER_ARGS_TYPE                                           \
    const v8::PropertyCallbackInfo<v8::Value>&
# define _NAN_INDEX_GETTER_ARGS _NAN_INDEX_GETTER_ARGS_TYPE args
# define _NAN_INDEX_GETTER_RETURN_TYPE void

# define _NAN_INDEX_SETTER_ARGS_TYPE                                           \
    const v8::PropertyCallbackInfo<v8::Value>&
# define _NAN_INDEX_SETTER_ARGS _NAN_INDEX_SETTER_ARGS_TYPE args
# define _NAN_INDEX_SETTER_RETURN_TYPE void

# define _NAN_INDEX_ENUMERATOR_ARGS_TYPE                                       \
    const v8::PropertyCallbackInfo<v8::Array>&
# define _NAN_INDEX_ENUMERATOR_ARGS _NAN_INDEX_ENUMERATOR_ARGS_TYPE args
# define _NAN_INDEX_ENUMERATOR_RETURN_TYPE void

# define _NAN_INDEX_DELETER_ARGS_TYPE                                          \
    const v8::PropertyCallbackInfo<v8::Boolean>&
# define _NAN_INDEX_DELETER_ARGS _NAN_INDEX_DELETER_ARGS_TYPE args
# define _NAN_INDEX_DELETER_RETURN_TYPE void

# define _NAN_INDEX_QUERY_ARGS_TYPE                                            \
    const v8::PropertyCallbackInfo<v8::Integer>&
# define _NAN_INDEX_QUERY_ARGS _NAN_INDEX_QUERY_ARGS_TYPE args
# define _NAN_INDEX_QUERY_RETURN_TYPE void

# define NanScope() v8::HandleScope scope(v8::Isolate::GetCurrent())
# define NanEscapableScope()                                                   \
  v8::EscapableHandleScope scope(v8::Isolate::GetCurrent())

# define NanEscapeScope(val) scope.Escape(Nan::imp::NanEnsureLocal(val))
# define NanLocker() v8::Locker locker(v8::Isolate::GetCurrent())
# define NanUnlocker() v8::Unlocker unlocker(v8::Isolate::GetCurrent())
# define NanReturnValue(value)                                                 \
return args.GetReturnValue().Set(Nan::imp::NanEnsureHandleOrPersistent(value))
# define NanReturnUndefined() return
# define NanReturnHolder() NanReturnValue(args.Holder())
# define NanReturnThis() NanReturnValue(args.This())
# define NanReturnNull() return args.GetReturnValue().SetNull()
# define NanReturnEmptyString() return args.GetReturnValue().SetEmptyString()

  NAN_INLINE
  v8::Local<v8::Object> NanObjectWrapHandle(const node::ObjectWrap *obj) {
    return const_cast<node::ObjectWrap*>(obj)->handle();
  }

  NAN_INLINE v8::Local<v8::Primitive> NanUndefined() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(v8::Undefined(v8::Isolate::GetCurrent())));
  }

  NAN_INLINE v8::Local<v8::Primitive> NanNull() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(v8::Null(v8::Isolate::GetCurrent())));
  }

  NAN_INLINE v8::Local<v8::Boolean> NanTrue() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(v8::True(v8::Isolate::GetCurrent())));
  }

  NAN_INLINE v8::Local<v8::Boolean> NanFalse() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(v8::False(v8::Isolate::GetCurrent())));
  }

  NAN_INLINE int NanAdjustExternalMemory(int bc) {
    return static_cast<int>(
        v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(bc));
  }

  NAN_INLINE void NanSetTemplate(
      v8::Handle<v8::Template> templ
    , const char *name
    , v8::Handle<v8::Data> value) {
    templ->Set(v8::Isolate::GetCurrent(), name, value);
  }

  NAN_INLINE void NanSetTemplate(
      v8::Handle<v8::Template> templ
    , v8::Handle<v8::String> name
    , v8::Handle<v8::Data> value
    , v8::PropertyAttribute attributes) {
    templ->Set(name, value, attributes);
  }

  NAN_INLINE v8::Local<v8::Context> NanGetCurrentContext() {
    return v8::Isolate::GetCurrent()->GetCurrentContext();
  }

  NAN_INLINE void* NanGetInternalFieldPointer(
      v8::Handle<v8::Object> object
    , int index) {
    return object->GetAlignedPointerFromInternalField(index);
  }

  NAN_INLINE void NanSetInternalFieldPointer(
      v8::Handle<v8::Object> object
    , int index
    , void* value) {
    object->SetAlignedPointerInInternalField(index, value);
  }

# define NAN_GC_CALLBACK(name)                                                 \
    void name(v8::Isolate *isolate, v8::GCType type, v8::GCCallbackFlags flags)

  NAN_INLINE void NanAddGCEpilogueCallback(
      v8::Isolate::GCEpilogueCallback callback
    , v8::GCType gc_type_filter = v8::kGCTypeAll) {
    v8::Isolate::GetCurrent()->AddGCEpilogueCallback(callback, gc_type_filter);
  }

  NAN_INLINE void NanRemoveGCEpilogueCallback(
      v8::Isolate::GCEpilogueCallback callback) {
    v8::Isolate::GetCurrent()->RemoveGCEpilogueCallback(callback);
  }

  NAN_INLINE void NanAddGCPrologueCallback(
      v8::Isolate::GCPrologueCallback callback
    , v8::GCType gc_type_filter = v8::kGCTypeAll) {
    v8::Isolate::GetCurrent()->AddGCPrologueCallback(callback, gc_type_filter);
  }

  NAN_INLINE void NanRemoveGCPrologueCallback(
      v8::Isolate::GCPrologueCallback callback) {
    v8::Isolate::GetCurrent()->RemoveGCPrologueCallback(callback);
  }

  NAN_INLINE void NanGetHeapStatistics(
      v8::HeapStatistics *heap_statistics) {
    v8::Isolate::GetCurrent()->GetHeapStatistics(heap_statistics);
  }

  template<typename T>
  NAN_INLINE void NanAssignPersistent(
      v8::Persistent<T>& handle
    , v8::Handle<T> obj) {
      handle.Reset(v8::Isolate::GetCurrent(), obj);
  }

  template<typename T>
  NAN_INLINE void NanAssignPersistent(
      v8::Persistent<T>& handle
    , const v8::Persistent<T>& obj) {
      handle.Reset(v8::Isolate::GetCurrent(), obj);
  }

  template<typename T, typename P>
  class _NanWeakCallbackData;

  template<typename T, typename P>
  struct _NanWeakCallbackInfo {
    typedef void (*Callback)(const _NanWeakCallbackData<T, P>& data);
    NAN_INLINE _NanWeakCallbackInfo(v8::Handle<T> handle, P* param, Callback cb)
      : parameter(param), callback(cb) {
       NanAssignPersistent(persistent, handle);
    }

    NAN_INLINE ~_NanWeakCallbackInfo() {
      persistent.Reset();
    }

    P* const parameter;
    Callback const callback;
    v8::Persistent<T> persistent;

   private:
    NAN_DISALLOW_ASSIGN_COPY_MOVE(_NanWeakCallbackInfo)
  };

  template<typename T, typename P>
  class _NanWeakCallbackData {
   public:
    NAN_INLINE _NanWeakCallbackData(_NanWeakCallbackInfo<T, P> *info)
      : info_(info) { }

    NAN_INLINE v8::Local<T> GetValue() const {
      return NanNew(info_->persistent);
    }

    NAN_INLINE P* GetParameter() const { return info_->parameter; }

    NAN_INLINE bool IsNearDeath() const {
      return info_->persistent.IsNearDeath();
    }

    NAN_INLINE void Revive() const;

    NAN_INLINE _NanWeakCallbackInfo<T, P>* GetCallbackInfo() const {
      return info_;
    }

   private:
    _NanWeakCallbackInfo<T, P>* info_;
  };

  template<typename T, typename P>
  static void _NanWeakCallbackDispatcher(
    const v8::WeakCallbackData<T, _NanWeakCallbackInfo<T, P> > &data) {
      _NanWeakCallbackInfo<T, P> *info = data.GetParameter();
      _NanWeakCallbackData<T, P> wcbd(info);
      info->callback(wcbd);
      if (wcbd.IsNearDeath()) {
        delete wcbd.GetCallbackInfo();
      }
  }

  template<typename T, typename P>
  NAN_INLINE void _NanWeakCallbackData<T, P>::Revive() const {
      info_->persistent.SetWeak(info_, &_NanWeakCallbackDispatcher<T, P>);
  }

  template<typename T, typename P>
  NAN_INLINE _NanWeakCallbackInfo<T, P>* NanMakeWeakPersistent(
      v8::Handle<T> handle
    , P* parameter
    , typename _NanWeakCallbackInfo<T, P>::Callback callback) {
      _NanWeakCallbackInfo<T, P> *cbinfo =
       new _NanWeakCallbackInfo<T, P>(handle, parameter, callback);
      cbinfo->persistent.SetWeak(cbinfo, &_NanWeakCallbackDispatcher<T, P>);
      return cbinfo;
  }

# define NAN_WEAK_CALLBACK(name)                                               \
    template<typename T, typename P>                                           \
    static void name(const _NanWeakCallbackData<T, P> &data)

# define X(NAME)                                                               \
    NAN_INLINE v8::Local<v8::Value> Nan ## NAME(const char *errmsg) {          \
      NanEscapableScope();                                                     \
      return NanEscapeScope(v8::Exception::NAME(NanNew(errmsg)));              \
    }                                                                          \
                                                                               \
    NAN_INLINE                                                                 \
    v8::Local<v8::Value> Nan ## NAME(v8::Handle<v8::String> errmsg) {          \
      return v8::Exception::NAME(errmsg);                                      \
    }                                                                          \
                                                                               \
    NAN_INLINE void NanThrow ## NAME(const char *errmsg) {                     \
      NanScope();                                                              \
      v8::Isolate::GetCurrent()->ThrowException(                               \
          v8::Exception::NAME(NanNew(errmsg)));                                \
    }                                                                          \
                                                                               \
    NAN_INLINE void NanThrow ## NAME(v8::Handle<v8::String> errmsg) {          \
      NanScope();                                                              \
      v8::Isolate::GetCurrent()->ThrowException(                               \
          v8::Exception::NAME(NanNew(errmsg)));                                \
    }

  X(Error)
  X(RangeError)
  X(ReferenceError)
  X(SyntaxError)
  X(TypeError)

# undef X

  NAN_INLINE void NanThrowError(v8::Handle<v8::Value> error) {
    v8::Isolate::GetCurrent()->ThrowException(error);
  }

  namespace Nan { namespace imp {
    NAN_INLINE v8::Local<v8::Value> E(const char *msg, const int errorNumber) {
      NanEscapableScope();
      v8::Local<v8::Value> err = v8::Exception::Error(NanNew<v8::String>(msg));
      v8::Local<v8::Object> obj = err.As<v8::Object>();
      obj->Set(NanNew<v8::String>("code"), NanNew<v8::Int32>(errorNumber));
      return NanEscapeScope(err);
    }
  }  // end of namespace imp
  }  // end of namespace Nan

  NAN_DEPRECATED NAN_INLINE v8::Local<v8::Value> NanError(
      const char *msg
    , const int errorNumber
  ) {
    return Nan::imp::E(msg, errorNumber);
  }

  NAN_DEPRECATED NAN_INLINE void NanThrowError(
      const char *msg
    , const int errorNumber
  ) {
    NanThrowError(Nan::imp::E(msg, errorNumber));
  }

  template<typename T> NAN_INLINE void NanDisposePersistent(
      v8::Persistent<T> &handle
  ) {
    handle.Reset();
  }

  NAN_INLINE v8::Local<v8::Object> NanNewBufferHandle (
      char *data
    , size_t length
    , node::smalloc::FreeCallback callback
    , void *hint
  ) {
    return node::Buffer::New(
        v8::Isolate::GetCurrent(), data, length, callback, hint);
  }

  NAN_INLINE v8::Local<v8::Object> NanNewBufferHandle (
      const char *data
    , uint32_t size
  ) {
    return node::Buffer::New(v8::Isolate::GetCurrent(), data, size);
  }

  NAN_INLINE v8::Local<v8::Object> NanNewBufferHandle (uint32_t size) {
    return node::Buffer::New(v8::Isolate::GetCurrent(), size);
  }

  NAN_INLINE v8::Local<v8::Object> NanBufferUse(
      char* data
    , uint32_t size
  ) {
    return node::Buffer::Use(v8::Isolate::GetCurrent(), data, size);
  }

  NAN_INLINE bool NanHasInstance(
      const v8::Persistent<v8::FunctionTemplate>& function_template
    , v8::Handle<v8::Value> value
  ) {
    return NanNew(function_template)->HasInstance(value);
  }

  NAN_INLINE v8::Local<NanBoundScript> NanCompileScript(
      v8::Local<v8::String> s
    , const v8::ScriptOrigin& origin
  ) {
    v8::ScriptCompiler::Source source(s, origin);
    return v8::ScriptCompiler::Compile(v8::Isolate::GetCurrent(), &source);
  }

  NAN_INLINE v8::Local<NanBoundScript> NanCompileScript(
      v8::Local<v8::String> s
  ) {
    v8::ScriptCompiler::Source source(s);
    return v8::ScriptCompiler::Compile(v8::Isolate::GetCurrent(), &source);
  }

  NAN_INLINE v8::Local<v8::Value> NanRunScript(
      v8::Handle<NanUnboundScript> script
  ) {
    return script->BindToCurrentContext()->Run();
  }

  NAN_INLINE v8::Local<v8::Value> NanRunScript(
      v8::Handle<NanBoundScript> script
  ) {
    return script->Run();
  }

  NAN_INLINE v8::Local<v8::Value> NanMakeCallback(
      v8::Handle<v8::Object> target
    , v8::Handle<v8::Function> func
    , int argc
    , v8::Handle<v8::Value>* argv) {
    return NanNew(node::MakeCallback(
        v8::Isolate::GetCurrent(), target, func, argc, argv));
  }

  NAN_INLINE v8::Local<v8::Value> NanMakeCallback(
      v8::Handle<v8::Object> target
    , v8::Handle<v8::String> symbol
    , int argc
    , v8::Handle<v8::Value>* argv) {
    return NanNew(node::MakeCallback(
        v8::Isolate::GetCurrent(), target, symbol, argc, argv));
  }

  NAN_INLINE v8::Local<v8::Value> NanMakeCallback(
      v8::Handle<v8::Object> target
    , const char* method
    , int argc
    , v8::Handle<v8::Value>* argv) {
    return NanNew(node::MakeCallback(
        v8::Isolate::GetCurrent(), target, method, argc, argv));
  }

  NAN_INLINE void NanFatalException(const v8::TryCatch& try_catch) {
    node::FatalException(v8::Isolate::GetCurrent(), try_catch);
  }

  template<typename T>
  NAN_INLINE void NanSetIsolateData(
      v8::Isolate *isolate
    , T *data
  ) {
      isolate->SetData(0, data);
  }

  template<typename T>
  NAN_INLINE T *NanGetIsolateData(
      v8::Isolate *isolate
  ) {
      return static_cast<T*>(isolate->GetData(0));
  }

  class NanAsciiString {
   public:
    NAN_INLINE explicit NanAsciiString(v8::Handle<v8::Value> from) {
      v8::Local<v8::String> toStr = from->ToString();
      size = toStr->Length();
      buf = new char[size + 1];
      size = toStr->WriteOneByte(reinterpret_cast<unsigned char*>(buf));
    }

    NAN_INLINE int length() const {
      return size;
    }


    NAN_INLINE char* operator*() { return buf; }
    NAN_INLINE const char* operator*() const { return buf; }

    NAN_INLINE ~NanAsciiString() {
      delete[] buf;
    }

   private:
    NAN_DISALLOW_ASSIGN_COPY_MOVE(NanAsciiString)

    char *buf;
    int size;
  };

  class NanUtf8String {
   public:
    NAN_INLINE explicit NanUtf8String(v8::Handle<v8::Value> from) {
      v8::Local<v8::String> toStr = from->ToString();
      size = toStr->Utf8Length();
      buf = new char[size + 1];
      toStr->WriteUtf8(buf);
    }

    NAN_INLINE int length() const {
      return size;
    }

    NAN_INLINE char* operator*() { return buf; }
    NAN_INLINE const char* operator*() const { return buf; }

    NAN_INLINE ~NanUtf8String() {
      delete[] buf;
    }

   private:
    NAN_DISALLOW_ASSIGN_COPY_MOVE(NanUtf8String)

    char *buf;
    int size;
  };

  class NanUcs2String {
   public:
    NAN_INLINE explicit NanUcs2String(v8::Handle<v8::Value> from) {
      v8::Local<v8::String> toStr = from->ToString();
      size = toStr->Length();
      buf = new uint16_t[size + 1];
      toStr->Write(buf);
    }

    NAN_INLINE int length() const {
      return size;
    }

    NAN_INLINE uint16_t* operator*() { return buf; }
    NAN_INLINE const uint16_t* operator*() const { return buf; }

    NAN_INLINE ~NanUcs2String() {
      delete[] buf;
    }

   private:
    NAN_DISALLOW_ASSIGN_COPY_MOVE(NanUcs2String)

    uint16_t *buf;
    int size;
  };

#else
// Node 0.8 and 0.10

# define _NAN_METHOD_ARGS_TYPE const v8::Arguments&
# define _NAN_METHOD_ARGS _NAN_METHOD_ARGS_TYPE args
# define _NAN_METHOD_RETURN_TYPE v8::Handle<v8::Value>

# define _NAN_GETTER_ARGS_TYPE const v8::AccessorInfo &
# define _NAN_GETTER_ARGS _NAN_GETTER_ARGS_TYPE args
# define _NAN_GETTER_RETURN_TYPE v8::Handle<v8::Value>

# define _NAN_SETTER_ARGS_TYPE const v8::AccessorInfo &
# define _NAN_SETTER_ARGS _NAN_SETTER_ARGS_TYPE args
# define _NAN_SETTER_RETURN_TYPE void

# define _NAN_PROPERTY_GETTER_ARGS_TYPE const v8::AccessorInfo&
# define _NAN_PROPERTY_GETTER_ARGS _NAN_PROPERTY_GETTER_ARGS_TYPE args
# define _NAN_PROPERTY_GETTER_RETURN_TYPE v8::Handle<v8::Value>

# define _NAN_PROPERTY_SETTER_ARGS_TYPE const v8::AccessorInfo&
# define _NAN_PROPERTY_SETTER_ARGS _NAN_PROPERTY_SETTER_ARGS_TYPE args
# define _NAN_PROPERTY_SETTER_RETURN_TYPE v8::Handle<v8::Value>

# define _NAN_PROPERTY_ENUMERATOR_ARGS_TYPE const v8::AccessorInfo&
# define _NAN_PROPERTY_ENUMERATOR_ARGS _NAN_PROPERTY_ENUMERATOR_ARGS_TYPE args
# define _NAN_PROPERTY_ENUMERATOR_RETURN_TYPE v8::Handle<v8::Array>

# define _NAN_PROPERTY_DELETER_ARGS_TYPE const v8::AccessorInfo&
# define _NAN_PROPERTY_DELETER_ARGS _NAN_PROPERTY_DELETER_ARGS_TYPE args
# define _NAN_PROPERTY_DELETER_RETURN_TYPE v8::Handle<v8::Boolean>

# define _NAN_PROPERTY_QUERY_ARGS_TYPE const v8::AccessorInfo&
# define _NAN_PROPERTY_QUERY_ARGS _NAN_PROPERTY_QUERY_ARGS_TYPE args
# define _NAN_PROPERTY_QUERY_RETURN_TYPE v8::Handle<v8::Integer>

# define _NAN_INDEX_GETTER_ARGS_TYPE const v8::AccessorInfo&
# define _NAN_INDEX_GETTER_ARGS _NAN_INDEX_GETTER_ARGS_TYPE args
# define _NAN_INDEX_GETTER_RETURN_TYPE v8::Handle<v8::Value>

# define _NAN_INDEX_SETTER_ARGS_TYPE const v8::AccessorInfo&
# define _NAN_INDEX_SETTER_ARGS _NAN_INDEX_SETTER_ARGS_TYPE args
# define _NAN_INDEX_SETTER_RETURN_TYPE v8::Handle<v8::Value>

# define _NAN_INDEX_ENUMERATOR_ARGS_TYPE const v8::AccessorInfo&
# define _NAN_INDEX_ENUMERATOR_ARGS _NAN_INDEX_ENUMERATOR_ARGS_TYPE args
# define _NAN_INDEX_ENUMERATOR_RETURN_TYPE v8::Handle<v8::Array>

# define _NAN_INDEX_DELETER_ARGS_TYPE const v8::AccessorInfo&
# define _NAN_INDEX_DELETER_ARGS _NAN_INDEX_DELETER_ARGS_TYPE args
# define _NAN_INDEX_DELETER_RETURN_TYPE v8::Handle<v8::Boolean>

# define _NAN_INDEX_QUERY_ARGS_TYPE const v8::AccessorInfo&
# define _NAN_INDEX_QUERY_ARGS _NAN_INDEX_QUERY_ARGS_TYPE args
# define _NAN_INDEX_QUERY_RETURN_TYPE v8::Handle<v8::Integer>

# define NanScope() v8::HandleScope scope
# define NanEscapableScope() v8::HandleScope scope
# define NanEscapeScope(val) scope.Close(val)
# define NanLocker() v8::Locker locker
# define NanUnlocker() v8::Unlocker unlocker
# define NanReturnValue(value)                                                 \
    return scope.Close(Nan::imp::NanEnsureHandleOrPersistent(value))
# define NanReturnHolder() NanReturnValue(args.Holder())
# define NanReturnThis() NanReturnValue(args.This())
# define NanReturnUndefined() return v8::Undefined()
# define NanReturnNull() return v8::Null()
# define NanReturnEmptyString() return v8::String::Empty()

  NAN_INLINE
  v8::Local<v8::Object> NanObjectWrapHandle(const node::ObjectWrap *obj) {
    return v8::Local<v8::Object>::New(obj->handle_);
  }

  NAN_INLINE v8::Local<v8::Primitive> NanUndefined() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(v8::Undefined()));
  }

  NAN_INLINE v8::Local<v8::Primitive> NanNull() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(v8::Null()));
  }

  NAN_INLINE v8::Local<v8::Boolean> NanTrue() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(v8::True()));
  }

  NAN_INLINE v8::Local<v8::Boolean> NanFalse() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(v8::False()));
  }

  NAN_INLINE int NanAdjustExternalMemory(int bc) {
    return static_cast<int>(v8::V8::AdjustAmountOfExternalAllocatedMemory(bc));
  }

  NAN_INLINE void NanSetTemplate(
      v8::Handle<v8::Template> templ
    , const char *name
    , v8::Handle<v8::Data> value) {
    templ->Set(name, value);
  }

  NAN_INLINE void NanSetTemplate(
      v8::Handle<v8::Template> templ
    , v8::Handle<v8::String> name
    , v8::Handle<v8::Data> value
    , v8::PropertyAttribute attributes) {
    templ->Set(name, value, attributes);
  }

  NAN_INLINE v8::Local<v8::Context> NanGetCurrentContext() {
    return v8::Context::GetCurrent();
  }

  NAN_INLINE void* NanGetInternalFieldPointer(
      v8::Handle<v8::Object> object
    , int index) {
    return object->GetPointerFromInternalField(index);
  }

  NAN_INLINE void NanSetInternalFieldPointer(
      v8::Handle<v8::Object> object
    , int index
    , void* value) {
    object->SetPointerInInternalField(index, value);
  }

# define NAN_GC_CALLBACK(name)                                                 \
    void name(v8::GCType type, v8::GCCallbackFlags flags)

  NAN_INLINE void NanAddGCEpilogueCallback(
    v8::GCEpilogueCallback callback
  , v8::GCType gc_type_filter = v8::kGCTypeAll) {
    v8::V8::AddGCEpilogueCallback(callback, gc_type_filter);
  }
  NAN_INLINE void NanRemoveGCEpilogueCallback(
    v8::GCEpilogueCallback callback) {
    v8::V8::RemoveGCEpilogueCallback(callback);
  }
  NAN_INLINE void NanAddGCPrologueCallback(
    v8::GCPrologueCallback callback
  , v8::GCType gc_type_filter = v8::kGCTypeAll) {
    v8::V8::AddGCPrologueCallback(callback, gc_type_filter);
  }
  NAN_INLINE void NanRemoveGCPrologueCallback(
    v8::GCPrologueCallback callback) {
    v8::V8::RemoveGCPrologueCallback(callback);
  }
  NAN_INLINE void NanGetHeapStatistics(
    v8::HeapStatistics *heap_statistics) {
    v8::V8::GetHeapStatistics(heap_statistics);
  }

  template<typename T>
  NAN_INLINE void NanAssignPersistent(
      v8::Persistent<T>& handle
    , v8::Handle<T> obj) {
      handle.Dispose();
      handle = v8::Persistent<T>::New(obj);
  }

  template<typename T, typename P>
  class _NanWeakCallbackData;

  template<typename T, typename P>
  struct _NanWeakCallbackInfo {
    typedef void (*Callback)(const _NanWeakCallbackData<T, P> &data);
    NAN_INLINE _NanWeakCallbackInfo(v8::Handle<T> handle, P* param, Callback cb)
      : parameter(param)
      , callback(cb)
      , persistent(v8::Persistent<T>::New(handle)) { }

    NAN_INLINE ~_NanWeakCallbackInfo() {
      persistent.Dispose();
      persistent.Clear();
    }

    P* const parameter;
    Callback const callback;
    v8::Persistent<T> persistent;

   private:
    NAN_DISALLOW_ASSIGN_COPY_MOVE(_NanWeakCallbackInfo)
  };

  template<typename T, typename P>
  class _NanWeakCallbackData {
   public:
    NAN_INLINE _NanWeakCallbackData(_NanWeakCallbackInfo<T, P> *info)
      : info_(info) { }

    NAN_INLINE v8::Local<T> GetValue() const {
      return NanNew(info_->persistent);
    }

    NAN_INLINE P* GetParameter() const { return info_->parameter; }

    NAN_INLINE bool IsNearDeath() const {
      return info_->persistent.IsNearDeath();
    }

    NAN_INLINE void Revive() const;

    NAN_INLINE _NanWeakCallbackInfo<T, P>* GetCallbackInfo() const {
      return info_;
    }

   private:
    _NanWeakCallbackInfo<T, P>* info_;
  };

  template<typename T, typename P>
  static void _NanWeakPersistentDispatcher(
      v8::Persistent<v8::Value> object, void *data) {
    (void) object;  // unused
    _NanWeakCallbackInfo<T, P>* info =
        static_cast<_NanWeakCallbackInfo<T, P>*>(data);
    _NanWeakCallbackData<T, P> wcbd(info);
    info->callback(wcbd);
    if (wcbd.IsNearDeath()) {
      delete wcbd.GetCallbackInfo();
    }
  }

  template<typename T, typename P>
  NAN_INLINE void _NanWeakCallbackData<T, P>::Revive() const {
      info_->persistent.MakeWeak(
          info_
        , &_NanWeakPersistentDispatcher<T, P>);
  }

  template<typename T, typename P>
  NAN_INLINE _NanWeakCallbackInfo<T, P>* NanMakeWeakPersistent(
    v8::Handle<T> handle
  , P* parameter
  , typename _NanWeakCallbackInfo<T, P>::Callback callback) {
      _NanWeakCallbackInfo<T, P> *cbinfo =
        new _NanWeakCallbackInfo<T, P>(handle, parameter, callback);
      cbinfo->persistent.MakeWeak(
          cbinfo
        , &_NanWeakPersistentDispatcher<T, P>);
      return cbinfo;
  }

# define NAN_WEAK_CALLBACK(name)                                               \
    template<typename T, typename P>                                           \
    static void name(const _NanWeakCallbackData<T, P> &data)

# define X(NAME)                                                               \
    NAN_INLINE v8::Local<v8::Value> Nan ## NAME(const char *errmsg) {          \
      NanEscapableScope();                                                     \
      return NanEscapeScope(v8::Exception::NAME(NanNew(errmsg)));              \
    }                                                                          \
                                                                               \
    NAN_INLINE                                                                 \
    v8::Local<v8::Value> Nan ## NAME(v8::Handle<v8::String> errmsg) {          \
      return v8::Exception::NAME(errmsg);                                      \
    }                                                                          \
                                                                               \
    NAN_INLINE v8::Local<v8::Value> NanThrow ## NAME(const char *errmsg) {     \
      NanEscapableScope();                                                     \
      return NanEscapeScope(NanNew(v8::ThrowException(                         \
          v8::Exception::NAME(NanNew(errmsg)))));                              \
    }                                                                          \
                                                                               \
    NAN_INLINE                                                                 \
    v8::Local<v8::Value> NanThrow ## NAME(v8::Handle<v8::String> errmsg) {     \
      NanEscapableScope();                                                     \
      return NanEscapeScope(NanNew(v8::ThrowException(                         \
          v8::Exception::NAME(errmsg))));                                      \
    }

  X(Error)
  X(RangeError)
  X(ReferenceError)
  X(SyntaxError)
  X(TypeError)

# undef X

  NAN_INLINE v8::Local<v8::Value> NanThrowError(v8::Handle<v8::Value> error) {
    NanEscapableScope();
    return NanEscapeScope(v8::Local<v8::Value>::New(v8::ThrowException(error)));
  }

  namespace Nan { namespace imp {
    NAN_INLINE v8::Local<v8::Value> E(const char *msg, const int errorNumber) {
      NanEscapableScope();
      v8::Local<v8::Value> err = v8::Exception::Error(NanNew<v8::String>(msg));
      v8::Local<v8::Object> obj = err.As<v8::Object>();
      obj->Set(NanNew<v8::String>("code"), NanNew<v8::Int32>(errorNumber));
      return NanEscapeScope(err);
    }
  }  // end of namespace imp
  }  // end of namespace Nan

  NAN_DEPRECATED NAN_INLINE v8::Local<v8::Value> NanError(
      const char *msg
    , const int errorNumber
  ) {
    return Nan::imp::E(msg, errorNumber);
  }

  NAN_DEPRECATED NAN_INLINE v8::Local<v8::Value> NanThrowError(
      const char *msg
    , const int errorNumber
  ) {
    return NanThrowError(Nan::imp::E(msg, errorNumber));
  }

  template<typename T>
  NAN_INLINE void NanDisposePersistent(
      v8::Persistent<T> &handle) {  // NOLINT(runtime/references)
    handle.Dispose();
    handle.Clear();
  }

  NAN_INLINE v8::Local<v8::Object> NanNewBufferHandle (
      char *data
    , size_t length
    , node::Buffer::free_callback callback
    , void *hint
  ) {
    return NanNew(
        node::Buffer::New(data, length, callback, hint)->handle_);
  }

  NAN_INLINE v8::Local<v8::Object> NanNewBufferHandle (
      const char *data
    , uint32_t size
  ) {
#if NODE_MODULE_VERSION >= NODE_0_10_MODULE_VERSION
    return NanNew(node::Buffer::New(data, size)->handle_);
#else
    return NanNew(
      node::Buffer::New(const_cast<char*>(data), size)->handle_);
#endif
  }

  NAN_INLINE v8::Local<v8::Object> NanNewBufferHandle (uint32_t size) {
    return NanNew(node::Buffer::New(size)->handle_);
  }

  NAN_INLINE void FreeData(char *data, void *hint) {
    (void) hint;  // unused
    delete[] data;
  }

  NAN_INLINE v8::Local<v8::Object> NanBufferUse(
      char* data
    , uint32_t size
  ) {
    return NanNew(
        node::Buffer::New(data, size, FreeData, NULL)->handle_);
  }

  NAN_INLINE bool NanHasInstance(
      const v8::Persistent<v8::FunctionTemplate>& function_template
    , v8::Handle<v8::Value> value
  ) {
    return function_template->HasInstance(value);
  }

  NAN_INLINE v8::Local<NanBoundScript> NanCompileScript(
      v8::Local<v8::String> s
    , const v8::ScriptOrigin& origin
  ) {
    return v8::Script::Compile(s, const_cast<v8::ScriptOrigin *>(&origin));
  }

  NAN_INLINE v8::Local<NanBoundScript> NanCompileScript(
    v8::Local<v8::String> s
  ) {
    return v8::Script::Compile(s);
  }

  NAN_INLINE v8::Local<v8::Value> NanRunScript(v8::Handle<v8::Script> script) {
    return script->Run();
  }

  NAN_INLINE v8::Local<v8::Value> NanMakeCallback(
      v8::Handle<v8::Object> target
    , v8::Handle<v8::Function> func
    , int argc
    , v8::Handle<v8::Value>* argv) {
# if NODE_VERSION_AT_LEAST(0, 8, 0)
    return NanNew(node::MakeCallback(target, func, argc, argv));
# else
    v8::TryCatch try_catch;
    v8::Local<v8::Value> result = func->Call(target, argc, argv);
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }
    return result;
# endif
  }

  NAN_INLINE v8::Local<v8::Value> NanMakeCallback(
      v8::Handle<v8::Object> target
    , v8::Handle<v8::String> symbol
    , int argc
    , v8::Handle<v8::Value>* argv) {
# if NODE_VERSION_AT_LEAST(0, 8, 0)
    return NanNew(node::MakeCallback(target, symbol, argc, argv));
# else
    v8::Local<v8::Function> callback = target->Get(symbol).As<v8::Function>();
    return NanMakeCallback(target, callback, argc, argv);
# endif
  }

  NAN_INLINE v8::Local<v8::Value> NanMakeCallback(
      v8::Handle<v8::Object> target
    , const char* method
    , int argc
    , v8::Handle<v8::Value>* argv) {
# if NODE_VERSION_AT_LEAST(0, 8, 0)
    return NanNew(node::MakeCallback(target, method, argc, argv));
# else
    return NanMakeCallback(target, NanNew(method), argc, argv);
# endif
  }

  NAN_INLINE void NanFatalException(const v8::TryCatch& try_catch) {
    node::FatalException(const_cast<v8::TryCatch&>(try_catch));
  }

  template<typename T>
  NAN_INLINE void NanSetIsolateData(
      v8::Isolate *isolate
    , T *data
  ) {
      isolate->SetData(data);
  }

  template<typename T>
  NAN_INLINE T *NanGetIsolateData(
      v8::Isolate *isolate
  ) {
      return static_cast<T*>(isolate->GetData());
  }

  class NanAsciiString {
   public:
    NAN_INLINE explicit NanAsciiString(v8::Handle<v8::Value> from) {
      v8::Local<v8::String> toStr = from->ToString();
      size = toStr->Length();
      buf = new char[size + 1];
      size = toStr->WriteAscii(buf);
    }

    NAN_INLINE int length() const {
      return size;
    }


    NAN_INLINE char* operator*() { return buf; }
    NAN_INLINE const char* operator*() const { return buf; }

    NAN_INLINE ~NanAsciiString() {
      delete[] buf;
    }

   private:
    NAN_DISALLOW_ASSIGN_COPY_MOVE(NanAsciiString)

    char *buf;
    int size;
  };

  class NanUtf8String {
   public:
    NAN_INLINE explicit NanUtf8String(v8::Handle<v8::Value> from) {
      v8::Local<v8::String> toStr = from->ToString();
      size = toStr->Utf8Length();
      buf = new char[size + 1];
      toStr->WriteUtf8(buf);
    }

    NAN_INLINE int length() const {
      return size;
    }

    NAN_INLINE char* operator*() { return buf; }
    NAN_INLINE const char* operator*() const { return buf; }

    NAN_INLINE ~NanUtf8String() {
      delete[] buf;
    }

   private:
    NAN_DISALLOW_ASSIGN_COPY_MOVE(NanUtf8String)

    char *buf;
    int size;
  };

  class NanUcs2String {
   public:
    NAN_INLINE explicit NanUcs2String(v8::Handle<v8::Value> from) {
      v8::Local<v8::String> toStr = from->ToString();
      size = toStr->Length();
      buf = new uint16_t[size + 1];
      toStr->Write(buf);
    }

    NAN_INLINE int length() const {
      return size;
    }

    NAN_INLINE uint16_t* operator*() { return buf; }
    NAN_INLINE const uint16_t* operator*() const { return buf; }

    NAN_INLINE ~NanUcs2String() {
      delete[] buf;
    }

   private:
    NAN_DISALLOW_ASSIGN_COPY_MOVE(NanUcs2String)

    uint16_t *buf;
    int size;
  };

#endif  // NODE_MODULE_VERSION

typedef void (*NanFreeCallback)(char *data, void *hint);

#define NAN_METHOD(name) _NAN_METHOD_RETURN_TYPE name(_NAN_METHOD_ARGS)
#define NAN_GETTER(name)                                                       \
    _NAN_GETTER_RETURN_TYPE name(                                              \
        v8::Local<v8::String> property                                         \
      , _NAN_GETTER_ARGS)
#define NAN_SETTER(name)                                                       \
    _NAN_SETTER_RETURN_TYPE name(                                              \
        v8::Local<v8::String> property                                         \
      , v8::Local<v8::Value> value                                             \
      , _NAN_SETTER_ARGS)
#define NAN_PROPERTY_GETTER(name)                                              \
    _NAN_PROPERTY_GETTER_RETURN_TYPE name(                                     \
        v8::Local<v8::String> property                                         \
      , _NAN_PROPERTY_GETTER_ARGS)
#define NAN_PROPERTY_SETTER(name)                                              \
    _NAN_PROPERTY_SETTER_RETURN_TYPE name(                                     \
        v8::Local<v8::String> property                                         \
      , v8::Local<v8::Value> value                                             \
      , _NAN_PROPERTY_SETTER_ARGS)
#define NAN_PROPERTY_ENUMERATOR(name)                                          \
    _NAN_PROPERTY_ENUMERATOR_RETURN_TYPE name(_NAN_PROPERTY_ENUMERATOR_ARGS)
#define NAN_PROPERTY_DELETER(name)                                             \
    _NAN_PROPERTY_DELETER_RETURN_TYPE name(                                    \
        v8::Local<v8::String> property                                         \
      , _NAN_PROPERTY_DELETER_ARGS)
#define NAN_PROPERTY_QUERY(name)                                               \
    _NAN_PROPERTY_QUERY_RETURN_TYPE name(                                      \
        v8::Local<v8::String> property                                         \
      , _NAN_PROPERTY_QUERY_ARGS)
# define NAN_INDEX_GETTER(name)                                                \
    _NAN_INDEX_GETTER_RETURN_TYPE name(uint32_t index, _NAN_INDEX_GETTER_ARGS)
#define NAN_INDEX_SETTER(name)                                                 \
    _NAN_INDEX_SETTER_RETURN_TYPE name(                                        \
        uint32_t index                                                         \
      , v8::Local<v8::Value> value                                             \
      , _NAN_INDEX_SETTER_ARGS)
#define NAN_INDEX_ENUMERATOR(name)                                             \
    _NAN_INDEX_ENUMERATOR_RETURN_TYPE name(_NAN_INDEX_ENUMERATOR_ARGS)
#define NAN_INDEX_DELETER(name)                                                \
    _NAN_INDEX_DELETER_RETURN_TYPE name(                                       \
        uint32_t index                                                         \
      , _NAN_INDEX_DELETER_ARGS)
#define NAN_INDEX_QUERY(name)                                                  \
    _NAN_INDEX_QUERY_RETURN_TYPE name(uint32_t index, _NAN_INDEX_QUERY_ARGS)

class NanCallback {
 public:
  NanCallback() {
    NanScope();
    v8::Local<v8::Object> obj = NanNew<v8::Object>();
    NanAssignPersistent(handle, obj);
  }

  explicit NanCallback(const v8::Handle<v8::Function> &fn) {
    NanScope();
    v8::Local<v8::Object> obj = NanNew<v8::Object>();
    NanAssignPersistent(handle, obj);
    SetFunction(fn);
  }

  ~NanCallback() {
    if (handle.IsEmpty()) return;
    NanDisposePersistent(handle);
  }

  bool operator==(const NanCallback &other) const {
    NanScope();
    v8::Local<v8::Value> a = NanNew(handle)->Get(kCallbackIndex);
    v8::Local<v8::Value> b = NanNew(other.handle)->Get(kCallbackIndex);
    return a->StrictEquals(b);
  }

  bool operator!=(const NanCallback &other) const {
    return !this->operator==(other);
  }

  NAN_INLINE void SetFunction(const v8::Handle<v8::Function> &fn) {
    NanScope();
    NanNew(handle)->Set(kCallbackIndex, fn);
  }

  NAN_INLINE v8::Local<v8::Function> GetFunction() const {
    NanEscapableScope();
    return NanEscapeScope(NanNew(handle)->Get(kCallbackIndex)
        .As<v8::Function>());
  }

  NAN_INLINE bool IsEmpty() const {
    NanScope();
    return NanNew(handle)->Get(kCallbackIndex)->IsUndefined();
  }

  NAN_INLINE v8::Handle<v8::Value>
  Call(v8::Handle<v8::Object> target
     , int argc
     , v8::Handle<v8::Value> argv[]) const {
#if (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    return Call_(isolate, target, argc, argv);
#else
    return Call_(target, argc, argv);
#endif
  }

  NAN_INLINE v8::Handle<v8::Value>
  Call(int argc, v8::Handle<v8::Value> argv[]) const {
#if (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    return Call_(isolate, isolate->GetCurrentContext()->Global(), argc, argv);
#else
    return Call_(v8::Context::GetCurrent()->Global(), argc, argv);
#endif
  }

 private:
  NAN_DISALLOW_ASSIGN_COPY_MOVE(NanCallback)
  v8::Persistent<v8::Object> handle;
  static const uint32_t kCallbackIndex = 0;

#if (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
  v8::Handle<v8::Value> Call_(v8::Isolate *isolate
                           , v8::Handle<v8::Object> target
                           , int argc
                           , v8::Handle<v8::Value> argv[]) const {
    NanEscapableScope();

    v8::Local<v8::Function> callback = NanNew(handle)->
        Get(kCallbackIndex).As<v8::Function>();
    return NanEscapeScope(node::MakeCallback(
        isolate
      , target
      , callback
      , argc
      , argv
    ));
  }
#else
  v8::Handle<v8::Value> Call_(v8::Handle<v8::Object> target
                           , int argc
                           , v8::Handle<v8::Value> argv[]) const {
    NanEscapableScope();

#if NODE_VERSION_AT_LEAST(0, 8, 0)
    v8::Local<v8::Function> callback = handle->
        Get(kCallbackIndex).As<v8::Function>();
    return NanEscapeScope(node::MakeCallback(
        target
      , callback
      , argc
      , argv
    ));
#else
    v8::Local<v8::Function> callback = handle->
        Get(kCallbackIndex).As<v8::Function>();
    return NanEscapeScope(NanMakeCallback(
        target, callback, argc, argv));
#endif
  }
#endif
};

/* abstract */ class NanAsyncWorker {
 public:
  explicit NanAsyncWorker(NanCallback *callback_)
      : callback(callback_), errmsg_(NULL) {
    request.data = this;

    NanScope();
    v8::Local<v8::Object> obj = NanNew<v8::Object>();
    NanAssignPersistent(persistentHandle, obj);
  }

  virtual ~NanAsyncWorker() {
    NanScope();

    if (!persistentHandle.IsEmpty())
      NanDisposePersistent(persistentHandle);
    if (callback)
      delete callback;
    if (errmsg_)
      delete[] errmsg_;
  }

  virtual void WorkComplete() {
    NanScope();

    if (errmsg_ == NULL)
      HandleOKCallback();
    else
      HandleErrorCallback();
    delete callback;
    callback = NULL;
  }

  NAN_INLINE void SaveToPersistent(
      const char *key, const v8::Local<v8::Object> &obj) {
    v8::Local<v8::Object> handle = NanNew(persistentHandle);
    handle->Set(NanNew<v8::String>(key), obj);
  }

  v8::Local<v8::Object> GetFromPersistent(const char *key) const {
    NanEscapableScope();
    v8::Local<v8::Object> handle = NanNew(persistentHandle);
    return NanEscapeScope(handle->Get(NanNew(key)).As<v8::Object>());
  }

  virtual void Execute() = 0;

  uv_work_t request;

  virtual void Destroy() {
      delete this;
  }

 protected:
  v8::Persistent<v8::Object> persistentHandle;
  NanCallback *callback;

  virtual void HandleOKCallback() {
    callback->Call(0, NULL);
  }

  virtual void HandleErrorCallback() {
    NanScope();

    v8::Local<v8::Value> argv[] = {
        v8::Exception::Error(NanNew<v8::String>(ErrorMessage()))
    };
    callback->Call(1, argv);
  }

  void SetErrorMessage(const char *msg) {
    if (errmsg_) {
      delete[] errmsg_;
    }

    size_t size = strlen(msg) + 1;
    errmsg_ = new char[size];
    memcpy(errmsg_, msg, size);
  }

  const char* ErrorMessage() const {
    return errmsg_;
  }

 private:
  NAN_DISALLOW_ASSIGN_COPY_MOVE(NanAsyncWorker)
  char *errmsg_;
};

/* abstract */ class NanAsyncProgressWorker : public NanAsyncWorker {
 public:
  explicit NanAsyncProgressWorker(NanCallback *callback_)
      : NanAsyncWorker(callback_), asyncdata_(NULL), asyncsize_(0) {
    async = new uv_async_t;
    uv_async_init(
        uv_default_loop()
      , async
      , AsyncProgress_
    );
    async->data = this;

    uv_mutex_init(&async_lock);
  }

  virtual ~NanAsyncProgressWorker() {
    uv_mutex_destroy(&async_lock);

    if (asyncdata_) {
      delete[] asyncdata_;
    }
  }

  void WorkProgress() {
    uv_mutex_lock(&async_lock);
    char *data = asyncdata_;
    size_t size = asyncsize_;
    asyncdata_ = NULL;
    uv_mutex_unlock(&async_lock);

    // Dont send progress events after we've already completed.
    if (callback) {
        HandleProgressCallback(data, size);
    }
    delete[] data;
  }

  class ExecutionProgress {
    friend class NanAsyncProgressWorker;
   public:
    // You could do fancy generics with templates here.
    void Send(const char* data, size_t size) const {
        that_->SendProgress_(data, size);
    }

   private:
    explicit ExecutionProgress(NanAsyncProgressWorker* that) : that_(that) {}
    NAN_DISALLOW_ASSIGN_COPY_MOVE(ExecutionProgress)
    NanAsyncProgressWorker* const that_;
  };

  virtual void Execute(const ExecutionProgress& progress) = 0;
  virtual void HandleProgressCallback(const char *data, size_t size) = 0;

  virtual void Destroy() {
      uv_close(reinterpret_cast<uv_handle_t*>(async), AsyncClose_);
  }

 private:
  void Execute() /*final override*/ {
      ExecutionProgress progress(this);
      Execute(progress);
  }

  void SendProgress_(const char *data, size_t size) {
    char *new_data = new char[size];
    memcpy(new_data, data, size);

    uv_mutex_lock(&async_lock);
    char *old_data = asyncdata_;
    asyncdata_ = new_data;
    asyncsize_ = size;
    uv_mutex_unlock(&async_lock);

    if (old_data) {
      delete[] old_data;
    }
    uv_async_send(async);
  }

  NAN_INLINE static NAUV_WORK_CB(AsyncProgress_) {
    NanAsyncProgressWorker *worker =
            static_cast<NanAsyncProgressWorker*>(async->data);
    worker->WorkProgress();
  }

  NAN_INLINE static void AsyncClose_(uv_handle_t* handle) {
    NanAsyncProgressWorker *worker =
            static_cast<NanAsyncProgressWorker*>(handle->data);
    delete reinterpret_cast<uv_async_t*>(handle);
    delete worker;
  }

  uv_async_t *async;
  uv_mutex_t async_lock;
  char *asyncdata_;
  size_t asyncsize_;
};

NAN_INLINE void NanAsyncExecute (uv_work_t* req) {
  NanAsyncWorker *worker = static_cast<NanAsyncWorker*>(req->data);
  worker->Execute();
}

NAN_INLINE void NanAsyncExecuteComplete (uv_work_t* req) {
  NanAsyncWorker* worker = static_cast<NanAsyncWorker*>(req->data);
  worker->WorkComplete();
  worker->Destroy();
}

NAN_INLINE void NanAsyncQueueWorker (NanAsyncWorker* worker) {
  uv_queue_work(
      uv_default_loop()
    , &worker->request
    , NanAsyncExecute
    , (uv_after_work_cb)NanAsyncExecuteComplete
  );
}

namespace Nan { namespace imp {

inline
NanExternalOneByteStringResource const*
GetExternalResource(v8::Local<v8::String> str) {
#if NODE_MODULE_VERSION < ATOM_0_21_MODULE_VERSION
    return str->GetExternalAsciiStringResource();
#else
    return str->GetExternalOneByteStringResource();
#endif
}

inline
bool
IsExternal(v8::Local<v8::String> str) {
#if NODE_MODULE_VERSION < ATOM_0_21_MODULE_VERSION
    return str->IsExternalAscii();
#else
    return str->IsExternalOneByte();
#endif
}

}  // end of namespace imp
}  // end of namespace Nan

namespace Nan {
  enum Encoding {ASCII, UTF8, BASE64, UCS2, BINARY, HEX, BUFFER};
}

#if !NODE_VERSION_AT_LEAST(0, 10, 0)
# include "nan_string_bytes.h"  // NOLINT(build/include)
#endif

NAN_INLINE v8::Local<v8::Value> NanEncode(
    const void *buf, size_t len, enum Nan::Encoding encoding = Nan::BINARY) {
#if (NODE_MODULE_VERSION >= ATOM_0_21_MODULE_VERSION)
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  node::encoding node_enc = static_cast<node::encoding>(encoding);

  if (encoding == Nan::UCS2) {
    return node::Encode(
        isolate
      , reinterpret_cast<const uint16_t *>(buf)
      , len / 2);
  } else {
    return node::Encode(
        isolate
      , reinterpret_cast<const char *>(buf)
      , len
      , node_enc);
  }
#elif (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
  return node::Encode(
      v8::Isolate::GetCurrent()
    , buf, len
    , static_cast<node::encoding>(encoding));
#else
# if NODE_VERSION_AT_LEAST(0, 10, 0)
  return node::Encode(buf, len, static_cast<node::encoding>(encoding));
# else
  return Nan::imp::Encode(reinterpret_cast<const char*>(buf), len, encoding);
# endif
#endif
}

NAN_INLINE ssize_t NanDecodeBytes(
    v8::Handle<v8::Value> val, enum Nan::Encoding encoding = Nan::BINARY) {
#if (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
  return node::DecodeBytes(
      v8::Isolate::GetCurrent()
    , val
    , static_cast<node::encoding>(encoding));
#else
# if (NODE_MODULE_VERSION < NODE_0_10_MODULE_VERSION)
  if (encoding == Nan::BUFFER) {
    return node::DecodeBytes(val, node::BINARY);
  }
# endif
  return node::DecodeBytes(val, static_cast<node::encoding>(encoding));
#endif
}

NAN_INLINE ssize_t NanDecodeWrite(
    char *buf
  , size_t len
  , v8::Handle<v8::Value> val
  , enum Nan::Encoding encoding = Nan::BINARY) {
#if (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
  return node::DecodeWrite(
      v8::Isolate::GetCurrent()
    , buf
    , len
    , val
    , static_cast<node::encoding>(encoding));
#else
# if (NODE_MODULE_VERSION < NODE_0_10_MODULE_VERSION)
  if (encoding == Nan::BUFFER) {
    return node::DecodeWrite(buf, len, val, node::BINARY);
  }
# endif
  return node::DecodeWrite(
      buf
    , len
    , val
    , static_cast<node::encoding>(encoding));
#endif
}

NAN_INLINE void NanSetPrototypeTemplate(
    v8::Local<v8::FunctionTemplate> templ
  , const char *name
  , v8::Handle<v8::Data> value
) {
  NanSetTemplate(templ->PrototypeTemplate(), name, value);
}

NAN_INLINE void NanSetPrototypeTemplate(
    v8::Local<v8::FunctionTemplate> templ
  , v8::Handle<v8::String> name
  , v8::Handle<v8::Data> value
  , v8::PropertyAttribute attributes
) {
  NanSetTemplate(templ->PrototypeTemplate(), name, value, attributes);
}

NAN_INLINE void NanSetInstanceTemplate(
    v8::Local<v8::FunctionTemplate> templ
  , const char *name
  , v8::Handle<v8::Data> value
) {
  NanSetTemplate(templ->InstanceTemplate(), name, value);
}

NAN_INLINE void NanSetInstanceTemplate(
    v8::Local<v8::FunctionTemplate> templ
  , v8::Handle<v8::String> name
  , v8::Handle<v8::Data> value
  , v8::PropertyAttribute attributes
) {
  NanSetTemplate(templ->InstanceTemplate(), name, value, attributes);
}

//=== Export ==================================================================

inline
void
NanExport(v8::Handle<v8::Object> target, const char * name,
    NanFunctionCallback f) {
  target->Set(NanNew<v8::String>(name),
      NanNew<v8::FunctionTemplate>(f)->GetFunction());
}

//=== Tap Reverse Binding =====================================================

struct NanTap {
  explicit NanTap(v8::Handle<v8::Value> t) : t_() {
    NanAssignPersistent(t_, t->ToObject());
  }

  ~NanTap() { NanDisposePersistent(t_); }  // not sure if neccessary

  inline void plan(int i) {
    v8::Handle<v8::Value> arg = NanNew(i);
    NanMakeCallback(NanNew(t_), "plan", 1, &arg);
  }

  inline void ok(bool isOk, const char * msg = NULL) {
    v8::Handle<v8::Value> args[2];
    args[0] = NanNew(isOk);
    if (msg) args[1] = NanNew(msg);
    NanMakeCallback(NanNew(t_), "ok", msg ? 2 : 1, args);
  }

 private:
  v8::Persistent<v8::Object> t_;
};

#define NAN_STRINGIZE2(x) #x
#define NAN_STRINGIZE(x) NAN_STRINGIZE2(x)
#define NAN_TEST_EXPRESSION(expression) \
  ( expression ), __FILE__ ":" NAN_STRINGIZE(__LINE__) ": " #expression

#define return_NanValue(v) NanReturnValue(v)
#define return_NanUndefined() NanReturnUndefined()
#define NAN_EXPORT(target, function) NanExport(target, #function, function)

#endif  // NAN_H_

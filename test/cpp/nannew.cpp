/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2018 NAN contributors
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 ********************************************************************/

// toys used in testing
// included first, so windows actually defines M_PI
#ifdef _WIN32
# define _USE_MATH_DEFINES
#endif
#include <nan.h>
#include <cmath>
#include <ctime>
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4530 )
# include <string>
# pragma warning( pop )
#else
# include <string>
#endif

using namespace Nan;  // NOLINT(build/namespaces)

using namespace v8;  // NOLINT(build/namespaces)

namespace {

template <typename T, typename U>
struct is_same {
  static const bool value = false;
};

template <typename T>
struct is_same<T, T> {
  static const bool value = true;
};

template <typename T, typename U>
bool
assertType(U value) {
  return is_same<v8::Local<T>, U>::value;
}

bool
stringMatches(Local<Value> value, const char * match) {
  Nan::Utf8String v(value);
  return std::string(*v) == std::string(match);
}

#define _(e) NAN_TEST_EXPRESSION(e)

NAN_METHOD(testArray) {
  Tap t(info[0]);

  t.plan(3);

  t.ok(_( New<Array>()->Length() == 0));
  t.ok(_( New<Array>(7)->Length() == 7));
  t.ok(_( assertType<Array>(New<Array>(7))));

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testBoolean) {
  Tap t(info[0]);

  t.plan(6);

  t.ok(_( New<Boolean>(true)->Value() == true));
  t.ok(_( New<Boolean>(false)->Value() == false));
  t.ok(_( assertType<Boolean>( New<Boolean>(true))));

  t.ok(_( New(true)->Value() == true));
  t.ok(_( New(false)->Value() == false));
  t.ok(_( assertType<Boolean>( New(true))));

  info.GetReturnValue().SetUndefined();
}

#if (NODE_MODULE_VERSION < 12)
# define V(x) x->BooleanValue()
#else
# define V(x) x->ValueOf()
#endif
NAN_METHOD(testBooleanObject) {
  Tap t(info[0]);

  t.plan(3);

  t.ok(_( assertType<BooleanObject>( New<BooleanObject>(true))));
  t.ok(_( V(New<BooleanObject>(true)) == true));
  t.ok(_( V(New<BooleanObject>(false)) == false));

  info.GetReturnValue().SetUndefined();
}
#undef V

NAN_METHOD(testContext) {
  Tap t(info[0]);

  t.plan(5);
  t.ok(_( assertType<Context>( New<Context>())));
  ExtensionConfiguration extensions(0, NULL);
  t.ok(_( assertType<Context>( New<Context>(&extensions))));
  t.ok(_( assertType<Context>(
          New<Context>(static_cast<ExtensionConfiguration *>(NULL)
          , Local<ObjectTemplate>()))));
  t.ok(_( assertType<Context>(
          New<Context>(&extensions, Local<ObjectTemplate>()))));
  t.ok(_( assertType<Context>(
          New<Context>(&extensions
          , Local<ObjectTemplate>(), Local<Value>()))));

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testDate) {
  Tap t(info[0]);

  t.plan(1);

  t.ok(_( assertType<Date>(
      New<Date>(static_cast<double>(time(NULL))).ToLocalChecked())));

  info.GetReturnValue().SetUndefined();
}

int ttt = 23;

NAN_METHOD(testExternal) {
  Tap t(info[0]);

  t.plan(2);

  t.ok(_(New<External>(&ttt)->Value() == &ttt));
  t.ok(_( assertType<External>(New<External>(&ttt))));

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testFunction) {
  Tap t(info[0]);
  t.plan(2);

  t.ok(_( assertType<Function>(New<Function>(testFunction))));
  v8::Local<String> data = New("plonk").ToLocalChecked();
  t.ok(_( assertType<Function>(New<Function>(testFunction, data))));

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testFunctionTemplate) {
  Tap t(info[0]);

  t.plan(4);

  t.ok(_( assertType<FunctionTemplate>( New<FunctionTemplate>())));
  t.ok(_( assertType<FunctionTemplate>(
          New<FunctionTemplate>(testFunctionTemplate))));
  v8::Local<String> data = New("plonk").ToLocalChecked();
  t.ok(_( assertType<FunctionTemplate>(
          New<FunctionTemplate>( testFunctionTemplate, data))));
  v8::Local<Signature> signature = New<Signature>();
  t.ok(_( assertType<FunctionTemplate>(
          New<FunctionTemplate>( testFunctionTemplate, data, signature))));

  info.GetReturnValue().SetUndefined();
}

const double epsilon = 1e-9;

NAN_METHOD(testNumber) {
  Tap t(info[0]);

  t.plan(17);

  t.ok(_( New<Int32>(5)->Value() == 5 ));
  t.ok(_( New<Int32>(-42)->Value() == -42 ));
  t.ok(_( assertType<Int32>( New<Int32>(23) )));

  t.ok(_( New<Uint32>(5u)->Value() == 5u ));
  t.ok(_( assertType<Uint32>( New<Uint32>(23u) )));

  t.ok(_( New<Integer>(5)->Value() == 5 ));
  t.ok(_( New<Integer>(-1337)->Value() == -1337 ));
  t.ok(_( assertType<Integer>( New<Integer>(-42) )));

  t.ok(_( fabs(New<Number>(M_PI)->Value() - M_PI) < epsilon));
  t.ok(_( fabs(New<Number>(-M_PI)->Value() + M_PI) < epsilon));
  t.ok(_( assertType<Number>( New<Number>(M_E) )));

  // === Convenience

  t.ok(_( New(5)->Value() == 5 ));
  t.ok(_( assertType<Int32>( New(23) )));

  t.ok(_( New(5u)->Value() == 5u ));
  t.ok(_( assertType<Uint32>( New(23u) )));

  t.ok(_( fabs(New(M_PI)->Value() - M_PI) < epsilon));
  t.ok(_( assertType<Number>( New(M_E) )));

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testNumberObject) {
  Tap t(info[0]);

  t.plan(2);

  t.ok(_( assertType<NumberObject>( New<NumberObject>(M_PI))));
  t.ok(_(
      fabs(
          To<double>(New<NumberObject>(M_PI)).FromJust() - M_PI
      ) < epsilon));

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testObject) {
  Tap t(info[0]);

  t.plan(1);

  t.ok(_(assertType<Object>( New<Object>())));

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testObjectTemplate) {
  Tap t(info[0]);

  t.plan(1);

  t.ok(_(assertType<ObjectTemplate>( New<ObjectTemplate>())));

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testScript) {
  Tap t(info[0]);

  t.plan(6);

  Nan::ScriptOrigin origin(New("foo").ToLocalChecked(), New(5));

  t.ok(_( assertType<Script>(New<Script>(
      New("2 + 3").ToLocalChecked()).ToLocalChecked())));
  t.ok(_( assertType<Script>(New<Script>(
      New("2 + 3").ToLocalChecked(), origin).ToLocalChecked())));
  t.ok(_( assertType<Nan::UnboundScript>(New<Nan::UnboundScript>(
      New("2 + 3").ToLocalChecked()).ToLocalChecked())));
  t.ok(_( assertType<Nan::UnboundScript>(
      New<Nan::UnboundScript>(
          New("2 + 3").ToLocalChecked()
        , origin).ToLocalChecked())));

  // for the fans of the bound script
  t.ok(_( To<int32_t>(RunScript(
      New<BoundScript>(New("2 + 3").ToLocalChecked()
    ).ToLocalChecked()).ToLocalChecked()).FromJust() == 5));
  t.ok(_( To<int32_t>(RunScript(
      New<Nan::UnboundScript>(New("2 + 3").ToLocalChecked()
    ).ToLocalChecked()).ToLocalChecked()).FromJust() == 5));

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testSignature) {
  Tap t(info[0]);

  t.plan(3);

  typedef FunctionTemplate FT;
  typedef Signature Sig;
  t.ok(_( assertType<Sig>(New<Sig>())));
  t.ok(_( assertType<Sig>(New<Sig>(New<FT>(testSignature)))));

  t.ok(_( assertType<Sig>(
      New<Sig>(New<FT>(testSignature)))));

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testString) {
  Tap t(info[0]);

  t.plan(16);

  t.ok(_( stringMatches(
      New<String>("Hello World").ToLocalChecked(), "Hello World")));
  t.ok(_( stringMatches(
      New<String>("Hello World", 4).ToLocalChecked(), "Hell")));
  t.ok(_( stringMatches(
      New<String>(std::string("foo")).ToLocalChecked(), "foo")));
  t.ok(_( assertType<String>(
      New<String>("plonk.").ToLocalChecked())));

  t.ok(_( stringMatches( New<String>().ToLocalChecked(), "")));
  t.ok(_( assertType<String>( New<String>().ToLocalChecked())));

  const uint8_t *ustring = reinterpret_cast<const uint8_t *>("unsigned chars");
  t.ok(_( stringMatches(
      NewOneByteString(ustring).ToLocalChecked(), "unsigned chars")));
  t.ok(_( stringMatches(
      NewOneByteString(ustring, 8).ToLocalChecked(), "unsigned")));

  // === Convenience

  t.ok(_( stringMatches(
      New("using namespace nan; // is poetry").ToLocalChecked()
    , "using namespace nan; // is poetry")));
  t.ok(_( assertType<String>( New("plonk.").ToLocalChecked())));

  t.ok(_( stringMatches( New("Hello World", 4).ToLocalChecked(), "Hell")));
  t.ok(_( assertType<String>( New("plonk.", 4).ToLocalChecked())));

  const uint16_t widestring[] = {'H', 'e', 'l', 'l', 'o', '\0'};
  t.ok(_( stringMatches( New(widestring, 4).ToLocalChecked(), "Hell")));
  t.ok(_( assertType<String>( New(widestring, 4).ToLocalChecked())));

  t.ok(_( stringMatches( New(std::string("bar")).ToLocalChecked(), "bar")));
  t.ok(_( assertType<String>( New(std::string("plonk.")).ToLocalChecked())));

  info.GetReturnValue().SetUndefined();
}

#if (NODE_MODULE_VERSION < 12)
# define V(x) x->StringValue()
#else
# define V(x) x->ValueOf()
#endif
NAN_METHOD(testStringObject) {
  Tap t(info[0]);

  t.plan(2);

  t.ok(_( stringMatches(
          V(New<StringObject>(New<String>("plonk").ToLocalChecked())),
          "plonk")));
  t.ok(_( assertType<StringObject>(
          New<StringObject>(New<String>("plonk").ToLocalChecked()))));

  info.GetReturnValue().SetUndefined();
}
#undef V

NAN_METHOD(testPersistents) {
  Tap t(info[0]);

  t.plan(1);

  Nan::Persistent<String> p;
  p.Reset(New("foo").ToLocalChecked());
  t.ok(_( assertType<String>( New(p))));
  p.Reset();

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testGlobals) {
  Tap t(info[0]);

  t.plan(1);

  Nan::Global<String> p;
  p.Reset(New("foo").ToLocalChecked());
  t.ok(_( assertType<String>( New(p))));
  p.Reset();

  info.GetReturnValue().SetUndefined();
}

//==============================================================================
// Regression Tests
//==============================================================================


// See https://github.com/nodejs/nan/issues/212
NAN_METHOD(testRegression212) {
  Tap t(info[0]);

  t.plan(1);

  typedef int  gint;
  typedef gint gboolean;
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4800 )
#endif
  t.ok(_( assertType<Boolean>( New<Boolean>(gboolean(23)))));
#if defined(_MSC_VER)
# pragma warning( pop )
#endif

  info.GetReturnValue().SetUndefined();
}

/* Compile time regression test for https://github.com/nodejs/nan/issues/242
 * In the presence of overloaded functions NaN should be able to pick the one
 * matching FunctionCallback.
 */
void overloaded() {}
NAN_METHOD(overloaded) {
    overloaded();  // not unused
    info.GetReturnValue().SetUndefined();
}

NAN_METHOD(testRegression242) {
  Tap t(info[0]);

  // These lines must *compile*. Not much to test at runtime.
  Local<FunctionTemplate> ft = New<FunctionTemplate>(overloaded);
  (void)ft;  // not unused
  Local<Function> f = New<Function>(overloaded);
  (void)f;  // not unused

  t.plan(1);

  t.ok(true, "compile-time regression test #242");

  info.GetReturnValue().SetUndefined();
}


//==============================================================================
// JavaScript Tests
//==============================================================================

NAN_METHOD(newIntegerWithValue) {
  info.GetReturnValue().Set(
      New(static_cast<int32_t>(To<int64_t>(info[0]).FromJust())));
}

NAN_METHOD(newNumberWithValue) {
  info.GetReturnValue().Set(New(To<double>(info[0]).FromJust()));
}

NAN_METHOD(newUint32WithValue) {
  info.GetReturnValue().Set(New(To<uint32_t>(info[0]).FromJust()));
}

NAN_METHOD(newStringFromChars) {
  info.GetReturnValue().Set(New("hello?").ToLocalChecked());
}

NAN_METHOD(newStringFromCharsWithLength) {
  info.GetReturnValue().Set(New("hello?", 4).ToLocalChecked());
}

NAN_METHOD(newStringFromStdString) {
  info.GetReturnValue().Set(New(std::string("hello!")).ToLocalChecked());
}

NAN_METHOD(newExternal) {
  info.GetReturnValue().Set(New<External>(&ttt));
}

NAN_METHOD(invokeMakeMaybe) {
  Nan::MaybeLocal<v8::Number> number = MakeMaybe(New<v8::Number>(3.141592654));
  Nan::MaybeLocal<v8::String> string = MakeMaybe(New<v8::String>("probably"));
  (void)string;
  info.GetReturnValue().Set(number.ToLocalChecked());
}

NAN_MODULE_INIT(Init) {
  NAN_EXPORT(target, testArray);
  NAN_EXPORT(target, testBoolean);
  NAN_EXPORT(target, testBooleanObject);
  NAN_EXPORT(target, testContext);
  NAN_EXPORT(target, testDate);
  NAN_EXPORT(target, testExternal);
  NAN_EXPORT(target, testFunction);
  NAN_EXPORT(target, testFunctionTemplate);
  NAN_EXPORT(target, testNumber);
  NAN_EXPORT(target, testNumberObject);
  NAN_EXPORT(target, testObject);
  NAN_EXPORT(target, testObjectTemplate);
  NAN_EXPORT(target, testScript);
  NAN_EXPORT(target, testSignature);
  NAN_EXPORT(target, testString);
  NAN_EXPORT(target, testStringObject);

  NAN_EXPORT(target, testPersistents);
  NAN_EXPORT(target, testGlobals);

  NAN_EXPORT(target, testRegression212);
  NAN_EXPORT(target, testRegression242);

  NAN_EXPORT(target, newIntegerWithValue);
  NAN_EXPORT(target, newNumberWithValue);
  NAN_EXPORT(target, newUint32WithValue);
  NAN_EXPORT(target, newStringFromChars);
  NAN_EXPORT(target, newStringFromCharsWithLength);
  NAN_EXPORT(target, newStringFromStdString);

  NAN_EXPORT(target, newExternal);

  NAN_EXPORT(target, invokeMakeMaybe);
}

}  // end of anonymous namespace

NODE_MODULE(nannew, Init)

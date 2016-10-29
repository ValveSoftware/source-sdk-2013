// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/renderer/performance_test.h"
#include "cefclient/renderer/performance_test_setup.h"
#include "include/cef_v8.h"

namespace client {
namespace performance_test {

namespace {

// Test function implementations.

PERF_TEST_FUNC(V8NullCreate) {
  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateNull();
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8BoolCreate) {
  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateBool(true);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8IntCreate) {
  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateInt(-5);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8UIntCreate) {
  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateUInt(10);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8DoubleCreate) {
  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateDouble(12.432);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8DateCreate) {
  static cef_time_t time = {2012, 1, 0, 1};

  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateDate(time);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8StringCreate) {
  CefString str = "test string";

  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateString(str);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8ArrayCreate) {
  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateArray(1);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8ArraySetValue) {
  CefRefPtr<CefV8Value> val = CefV8Value::CreateBool(true);
  CefRefPtr<CefV8Value> array = CefV8Value::CreateArray(1);
  array->SetValue(0, val);

  PERF_ITERATIONS_START()
  array->SetValue(0, val);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8ArrayGetValue) {
  CefRefPtr<CefV8Value> val = CefV8Value::CreateBool(true);
  CefRefPtr<CefV8Value> array = CefV8Value::CreateArray(1);
  array->SetValue(0, val);

  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> ret = array->GetValue(0);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8FunctionCreate) {
  class Handler : public CefV8Handler {
   public:
    Handler() {}
    virtual bool Execute(const CefString& name,
                         CefRefPtr<CefV8Value> object,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval,
                         CefString& exception) OVERRIDE { return false; }
    IMPLEMENT_REFCOUNTING(Handler);
  };

  CefString name = "name";
  CefRefPtr<CefV8Handler> handler = new Handler();

  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateFunction(name, handler);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8FunctionExecute) {
  class Handler : public CefV8Handler {
   public:
    Handler() {}
    virtual bool Execute(const CefString& name,
                         CefRefPtr<CefV8Value> object,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval,
                         CefString& exception) OVERRIDE { return true; }
    IMPLEMENT_REFCOUNTING(Handler);
  };

  CefString name = "name";
  CefRefPtr<CefV8Handler> handler = new Handler();
  CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction(name, handler);
  CefRefPtr<CefV8Value> obj = CefV8Context::GetCurrentContext()->GetGlobal();
  CefV8ValueList args;

  PERF_ITERATIONS_START()
  func->ExecuteFunction(obj, args);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8FunctionExecuteWithContext) {
  class Handler : public CefV8Handler {
   public:
    Handler() {}
    virtual bool Execute(const CefString& name,
                         CefRefPtr<CefV8Value> object,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval,
                         CefString& exception) OVERRIDE { return true; }
    IMPLEMENT_REFCOUNTING(Handler);
  };

  CefString name = "name";
  CefRefPtr<CefV8Handler> handler = new Handler();
  CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction(name, handler);
  CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
  CefRefPtr<CefV8Value> obj = context->GetGlobal();
  CefV8ValueList args;

  PERF_ITERATIONS_START()
  func->ExecuteFunctionWithContext(context, obj, args);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8ObjectCreate) {
  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateObject(NULL);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8ObjectCreateWithAccessor) {
  class Accessor : public CefV8Accessor {
   public:
    Accessor() {}
    virtual bool Get(const CefString& name,
                     const CefRefPtr<CefV8Value> object,
                     CefRefPtr<CefV8Value>& retval,
                     CefString& exception) OVERRIDE {
      return true;
    }
    virtual bool Set(const CefString& name,
                     const CefRefPtr<CefV8Value> object,
                     const CefRefPtr<CefV8Value> value,
                     CefString& exception) OVERRIDE {
      return true;
    }
    IMPLEMENT_REFCOUNTING(Accessor);
  };

  CefRefPtr<CefV8Accessor> accessor = new Accessor();

  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> value = CefV8Value::CreateObject(accessor);
  PERF_ITERATIONS_END()
}


PERF_TEST_FUNC(V8ObjectSetValue) {
  CefString name = "name";
  CefRefPtr<CefV8Value> val = CefV8Value::CreateBool(true);
  CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(NULL);
  obj->SetValue(name, val, V8_PROPERTY_ATTRIBUTE_NONE);

  PERF_ITERATIONS_START()
  obj->SetValue(name, val, V8_PROPERTY_ATTRIBUTE_NONE);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8ObjectGetValue) {
  CefString name = "name";
  CefRefPtr<CefV8Value> val = CefV8Value::CreateBool(true);
  CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(NULL);
  obj->SetValue(name, val, V8_PROPERTY_ATTRIBUTE_NONE);

  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> ret = obj->GetValue(name);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8ObjectSetValueWithAccessor) {
  class Accessor : public CefV8Accessor {
   public:
    Accessor() {}
    virtual bool Get(const CefString& name,
                     const CefRefPtr<CefV8Value> object,
                     CefRefPtr<CefV8Value>& retval,
                     CefString& exception) OVERRIDE {
      return true;
    }
    virtual bool Set(const CefString& name,
                     const CefRefPtr<CefV8Value> object,
                     const CefRefPtr<CefV8Value> value,
                     CefString& exception) OVERRIDE {
      val_ = value;
      return true;
    }
    CefRefPtr<CefV8Value> val_;
    IMPLEMENT_REFCOUNTING(Accessor);
  };

  CefRefPtr<CefV8Accessor> accessor = new Accessor();

  CefString name = "name";
  CefRefPtr<CefV8Value> val = CefV8Value::CreateBool(true);
  CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(accessor);
  obj->SetValue(name, V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
  obj->SetValue(name, val, V8_PROPERTY_ATTRIBUTE_NONE);

  PERF_ITERATIONS_START()
  obj->SetValue(name, val, V8_PROPERTY_ATTRIBUTE_NONE);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8ObjectGetValueWithAccessor) {
  class Accessor : public CefV8Accessor {
   public:
    Accessor() : val_(CefV8Value::CreateBool(true)) {}
    virtual bool Get(const CefString& name,
                     const CefRefPtr<CefV8Value> object,
                     CefRefPtr<CefV8Value>& retval,
                     CefString& exception) OVERRIDE {
      retval = val_;
      return true;
    }
    virtual bool Set(const CefString& name,
                     const CefRefPtr<CefV8Value> object,
                     const CefRefPtr<CefV8Value> value,
                     CefString& exception) OVERRIDE {
      return true;
    }
    CefRefPtr<CefV8Value> val_;
    IMPLEMENT_REFCOUNTING(Accessor);
  };

  CefRefPtr<CefV8Accessor> accessor = new Accessor();

  CefString name = "name";
  CefRefPtr<CefV8Value> val = CefV8Value::CreateBool(true);
  CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(accessor);
  obj->SetValue(name, V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
  obj->SetValue(name, val, V8_PROPERTY_ATTRIBUTE_NONE);

  PERF_ITERATIONS_START()
  CefRefPtr<CefV8Value> ret = obj->GetValue(name);
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8ContextEnterExit) {
  CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();

  PERF_ITERATIONS_START()
  context->Enter();
  context->Exit();
  PERF_ITERATIONS_END()
}

PERF_TEST_FUNC(V8ContextEval) {
  CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
  CefString jsCode = "var i = 0;";
  CefRefPtr<CefV8Value> retval;
  CefRefPtr<CefV8Exception> exception;

  PERF_ITERATIONS_START()
  context->Eval(jsCode, retval, exception);
  PERF_ITERATIONS_END()
}

}  // namespace


// Test function entries.

const PerfTestEntry kPerfTests[] = {
  PERF_TEST_ENTRY(V8NullCreate),
  PERF_TEST_ENTRY(V8BoolCreate),
  PERF_TEST_ENTRY(V8IntCreate),
  PERF_TEST_ENTRY(V8UIntCreate),
  PERF_TEST_ENTRY(V8DoubleCreate),
  PERF_TEST_ENTRY(V8DateCreate),
  PERF_TEST_ENTRY(V8StringCreate),
  PERF_TEST_ENTRY(V8ArrayCreate),
  PERF_TEST_ENTRY(V8ArraySetValue),
  PERF_TEST_ENTRY(V8ArrayGetValue),
  PERF_TEST_ENTRY(V8FunctionCreate),
  PERF_TEST_ENTRY(V8FunctionExecute),
  PERF_TEST_ENTRY(V8FunctionExecuteWithContext),
  PERF_TEST_ENTRY(V8ObjectCreate),
  PERF_TEST_ENTRY(V8ObjectCreateWithAccessor),
  PERF_TEST_ENTRY(V8ObjectSetValue),
  PERF_TEST_ENTRY(V8ObjectGetValue),
  PERF_TEST_ENTRY(V8ObjectSetValueWithAccessor),
  PERF_TEST_ENTRY(V8ObjectGetValueWithAccessor),
  PERF_TEST_ENTRY(V8ContextEnterExit),
  PERF_TEST_ENTRY(V8ContextEval),
};

const int kPerfTestsCount = (sizeof(kPerfTests) / sizeof(kPerfTests[0]));

}  // namespace performance_test
}  // namespace client

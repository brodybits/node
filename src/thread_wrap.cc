#include "env.h"
#include "env-inl.h"
#include "handle_wrap.h"

#include <unistd.h>

namespace node {

using v8::Context;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::Value;

class ThreadWrap : public HandleWrap {
 public:
  static void Initialize(Local<Object> target,
                         Local<Value> unused,
                         Local<Context> context) {
    Environment* env = Environment::GetCurrent(context);
    Local<FunctionTemplate> constructor = env->NewFunctionTemplate(New);
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(FIXED_ONE_BYTE_STRING(env->isolate(), "Thread"));

    env->SetProtoMethod(constructor, "close", HandleWrap::Close);

    env->SetProtoMethod(constructor, "run", ThreadRun);

    env->SetProtoMethod(constructor, "ref", HandleWrap::Ref);
    env->SetProtoMethod(constructor, "unref", HandleWrap::Unref);
    env->SetProtoMethod(constructor, "hasRef", HandleWrap::HasRef);

    target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "Thread"),
                constructor->GetFunction());
  }

  size_t self_size() const override { return sizeof(*this); }

 private:
  static void New(const FunctionCallbackInfo<Value>& args) {
    // This constructor should not be exposed to public javascript.
    // Therefore we assert that we are not trying to call this as a
    // normal function.
    CHECK(args.IsConstructCall());
    Environment* env = Environment::GetCurrent(args);
    new ThreadWrap(env, args.This());
  }

  ThreadWrap(Environment* env, Local<Object> object)
      : HandleWrap(env,
                   object,
                   reinterpret_cast<uv_handle_t*>(&default_work_),
                   AsyncWrap::PROVIDER_THREADWRAP) {
  }

  static void ThreadRun(const FunctionCallbackInfo<Value>& args) {
    ThreadWrap* wrap;
    ASSIGN_OR_RETURN_UNWRAP(&wrap, args.Holder());

    Environment* env = Environment::GetCurrent(args);
    CHECK(args[0]->IsString());
    node::Utf8Value scriptRef(env->isolate(), args[0]);

    uv_thread_t thid;
    char* script = strdup(*scriptRef);
    uv_thread_create(&thid, DoThreadRun, reinterpret_cast<void*>(script));
  }

  static void DoThreadRun(void* arg) {
    char* script = static_cast<char*>(arg);
    int argsSize = strlen(script) + 9;

    char* args = static_cast<char*>(malloc(argsSize));
    snprintf(args, argsSize, "node -e %s", script);
    args[4] = args[7] = '\0';

    char* argv[] = {args, args+5, args+8};
    StartWorker(3, argv);

    // CLEANUP after delay
    usleep(50*1000);
    free(args);
    free(script);
  }

  // TBD ???:
  // WARNING: uv_thread_t is NOT an uv_handle_t instance.
  // uv_thread_t thread_;

  // NOTE: Need a real uv_handle_t instance for HandleWrap to work.
  uv_work_t default_work_;
};


}  // namespace node

NODE_MODULE_CONTEXT_AWARE_BUILTIN(thread_wrap, node::ThreadWrap::Initialize)

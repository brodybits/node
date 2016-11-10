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

    env->SetProtoMethod(constructor, "numberTest", NumberTest);
    env->SetProtoMethod(constructor, "threadTest", ThreadTest);
    env->SetProtoMethod(constructor, "threadTest2", ThreadTest2);
    env->SetProtoMethod(constructor, "threadTest3", ThreadTest3);

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

  static void NumberTest(const FunctionCallbackInfo<Value>& args) {
    ThreadWrap* wrap;
    ASSIGN_OR_RETURN_UNWRAP(&wrap, args.Holder());
    int signal = args[0]->Int32Value();
    printf("test number: %d\n", signal);
    args.GetReturnValue().Set(2);
  }

  static void ThreadTest(const FunctionCallbackInfo<Value>& args) {
    ThreadWrap* wrap;
    ASSIGN_OR_RETURN_UNWRAP(&wrap, args.Holder());
    int number = args[0]->Int32Value();
    uv_thread_t thid;
    uv_thread_create(&thid, DoThreadTest, &number);
    //printf("test number: %d\n", signal);
    //args.GetReturnValue().Set(2);
  }

  static void ThreadTest2(const FunctionCallbackInfo<Value>& args) {
    ThreadWrap* wrap;
    ASSIGN_OR_RETURN_UNWRAP(&wrap, args.Holder());
    int number = args[0]->Int32Value();
    uv_thread_t thid;
    uv_thread_create(&thid, DoThreadTest2, &number);
    //printf("test number: %d\n", signal);
    //args.GetReturnValue().Set(2);
  }

  static void ThreadTest3(const FunctionCallbackInfo<Value>& args) {
    ThreadWrap* wrap;
    ASSIGN_OR_RETURN_UNWRAP(&wrap, args.Holder());

    Environment* env = Environment::GetCurrent(args);
    CHECK(args[0]->IsString());
    node::Utf8Value script(env->isolate(), args[0]);
    //const char* script1 = *script;

    uv_thread_t thid;
    //uv_thread_create(&thid, DoThreadTest3, (void *)*script1);
    uv_thread_create(&thid, DoThreadTest3, (void *)strdup(*script));
  }

  static void DoThreadTest(void* args) {
    printf("test thread output\n");
  }

  static void DoThreadTest2(void* args) {
    printf("DoThreadTest2\n");
    char * argv[] = {"./node", "-e", "console.log('******** test log from new thread ********');"};
    StartWorker(3, argv);
  }

  static void DoThreadTest3(void* arg) {
    printf("DoThreadTest3\n");
    //char* script1 = static_cast<char *>(args);
    char* script1 = static_cast<char *>(arg);
    printf("got script1: %s", script1);
    //char* argv[] = {"./node", "-e", static_cast<char *>(args)};
    //char* argv[] = {"./node", "-e", script1};
    //char * argv[] = {"./node", "-e", "console.log('******** test log from bogus script ********');"};
    char * args = static_cast<char *>(malloc(strlen(script1)+9));
    sprintf(args, "node -e %s", script1);
    args[4] = args[7] = '\0';
    char* argv[] = {args, args+5, args+8};
    StartWorker(3, argv);

    // CLEANUP (FUTURE TBD)
    usleep(50*1000);
    free(args);
    free(script1);
  }

  // TBD ???:
  // WARNING: uv_thread_t is NOT an uv_handle_t instance.
  //uv_thread_t thread_;

  // NOTE: Need a real uv_handle_t instance for HandleWrap to work.
  uv_work_t default_work_;
};


}  // namespace node

NODE_MODULE_CONTEXT_AWARE_BUILTIN(thread_wrap, node::ThreadWrap::Initialize)

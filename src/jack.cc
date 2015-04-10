#include <nan.h>
#include <node.h>
#include <jack/jack.h>
//#include "jack.h"

using node::ObjectWrap;
using v8::FunctionTemplate;
using v8::Handle;
using v8::Local;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;

static Persistent<FunctionTemplate> constructor;

class Client : public ObjectWrap {

  private:

    jack_client_t * client;

    Client (const char * name) {
      client = jack_client_open(name, JackNullOption, NULL);
    }

    ~Client () {
      jack_client_close(client);
    }

    static NAN_METHOD(New) {
      NanScope();
      Client * obj  = new Client(*NanUtf8String(args[0]));
      obj->Wrap(args.This());
      NanReturnValue(args.This());
    }

  public:

    static void Init() {
      Local<FunctionTemplate> t =
        NanNew<FunctionTemplate>(Client::New);
      NanAssignPersistent(constructor, t);
      t->SetClassName(NanNew("Client"));
      t->InstanceTemplate()->SetInternalFieldCount(1);
    }

};

void init (Handle<Object> exports) {
  Client::Init();
  Local<FunctionTemplate> handle = NanNew(constructor);
  exports->Set(NanSymbol("Client"), handle->GetFunction());
}

NODE_MODULE(jack, init)

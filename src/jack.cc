#pragma GCC diagnostic ignored "-Wwrite-strings" // mmm, pragma!

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

    static NAN_METHOD(New) {
      NanScope();
      Client * obj = new Client(args.This(), *NanUtf8String(args[0]));
      obj->Wrap(args.This());
      NanReturnValue(args.This());
    }

    jack_client_t * client;

    Local<Object> _this;

    Client (Local<Object> self, const char * name) {

      _this = self;

      client = jack_client_open(name, JackNullOption, NULL);

      jack_set_client_registration_callback ( client
                                            , client_registration_callback
                                            , this );

      jack_set_port_registration_callback ( client
                                          , port_registration_callback
                                          , this );

      jack_activate(client);

    }

    ~Client () {
      jack_client_close(client);
    }

    static void client_registration_callback
      ( const char * name
      , int          reg
      , void       * client_ptr)
    {
      //v8::FunctionCallbackInfo<v8::Value> argv[2] =
      Handle<Value> argv[2] =
      { NanNew(reg ? "client-registered"
                   : "client-unregistered")
      , NanNew(name) };
      static_cast<Client*>(client_ptr)->emitEvent(argv);
    }

    static void port_registration_callback
      ( jack_port_id_t port
      , int            reg
      , void         * client_ptr)
    {
      Handle<Value> argv[2] =
      { NanNew(reg ? "port-registered"
                   : "port-unregistered")
      , NanUndefined() };
      static_cast<Client*>(client_ptr)->emitEvent(argv);
    }

  public:

    static void Init() {
      Local<FunctionTemplate> t =
        NanNew<FunctionTemplate>(Client::New);
      NanAssignPersistent(constructor, t);
      t->SetClassName(NanNew("Client"));
      t->InstanceTemplate()->SetInternalFieldCount(1);
    }

    void emitEvent (Handle<Value> * argv) {
      NanScope();
      NanMakeCallback(_this, "emit", 2, argv);
      NanReturnUndefined();
    }

};

//void * client_registered ( const char * name
                         //, int          reg
                         //, void       * client_ptr ) {
  //printf("CLIENT REGISTERED 1");
  //static_cast<Client*>(client_ptr)->emitEvent("client-registered");
//}

void init (Handle<Object> exports) {
  Client::Init();
  Local<FunctionTemplate> handle = NanNew(constructor);
  exports->Set(NanNew("Client"), handle->GetFunction());
}

NODE_MODULE(jack, init)

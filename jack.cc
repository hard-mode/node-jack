#pragma GCC diagnostic ignored "-Wwrite-strings" // mmm, pragma!

#include <nan.h>
#include <node.h>
#include <jack/jack.h>
//#include "jack.h"

using node::ObjectWrap;
using v8::Function;
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

    // node constructor

    Persistent<Object> self;

    static NAN_METHOD(New) {
      NanScope();
      Local<Object> self = args.This();
      Client * obj = new Client(* NanUtf8String(args[0]));
      obj->Wrap(self);
      NanAssignPersistent(obj->self, self);
      NanReturnValue(self);
    }

    // actual constructor and jack connection

    jack_client_t * client;

    Client (const char * name) {

      client = jack_client_open(name, JackNullOption, NULL);

      jack_set_client_registration_callback ( client
                                            , client_registration_callback
                                            , this );

      jack_set_port_registration_callback ( client
                                          , port_registration_callback
                                          , this );

      jack_activate(client);

    }

    // destructor

    ~Client () {
      jack_client_close(client);
    }

    // static callbacks for jack

    static void client_registration_callback
      ( const char * name
      , int          reg
      , void       * client_ptr)
    {
      Client * c = static_cast<Client*>(client_ptr);
      if (reg) {
        c->onClientRegistered(name);
      } else {
        c->onClientUnregistered(name);
      }
    }

    static void port_registration_callback
      ( jack_port_id_t port
      , int            reg
      , void         * client_ptr)
    {
      Client * c = static_cast<Client*>(client_ptr);
      if (reg) {
        c->onPortRegistered();
      } else {
        c->onPortUnregistered();
      }
    }

  public:

    // create prototype

    static void Init() {
      Local<FunctionTemplate> t = NanNew<FunctionTemplate>(Client::New);
      NanAssignPersistent(constructor, t);
      t->SetClassName(NanNew("Client"));
      t->InstanceTemplate()->SetInternalFieldCount(1);
    }

    // event emit intermediaries
    // need to be public for static_casted clients to work
    // and should be replaced with a more robust mechanism

    void onClientRegistered (const char * name) {
      fprintf(stderr, "\nclient registered %s", name);
    }

    void onClientUnregistered (const char * name) {
      fprintf(stderr, "\nclient unregistered %s", name);
    }

    void onPortRegistered () {
      fprintf(stderr, "\nport registered");
    }

    void onPortUnregistered () {
      fprintf(stderr, "\nport unregistered");
    }

    void emitEvent (const char * name, const char * arg) {
      fprintf(stderr, "\nemit %s %s", name, arg);
      //Local<Object> yaze = NanNew(self);
      //if (yaze->Has(NanNew("onport"))
      //&& ( name == "port-registered" ||
           //name == "port-unregistered")) {
        //NanMakeCallback(yaze, yaze->Get("onport"), 0, NULL);
      //}
    }

};

void init (Handle<Object> exports) {
  Client::Init();
  exports->Set(NanNew("Client"), NanNew(constructor)->GetFunction());
}

NODE_MODULE(jack, init)

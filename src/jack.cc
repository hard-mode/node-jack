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
      Client * obj  = new Client(*NanUtf8String(args[0]));
      obj->Wrap(args.This());
      NanReturnValue(args.This());
    }

    jack_client_t * client;

    Client (const char * name) {
      client = jack_client_open(name, JackNullOption, NULL);
      jack_set_client_registration_callback(
        client, client_registered, this);
      jack_set_port_registration_callback(
        client, port_registered, this);
      jack_activate(client);
    }

    ~Client () {
      jack_client_close(client);
    }

    static void client_registered
      ( const char * name
      , int          reg
      , void       * client_ptr)
    {
      Client* client = static_cast<Client*> client_ptr;
      if (reg) {
        client->emitEvent("client-registered", name); 
      } else {
        client->emitEvent("client-unregistered", name); 
      }
    }

    static void port_registered
      ( jack_port_id_t port
      , int            reg
      , void         * client_ptr)
    {
      Client* client = static_cast<Client*> client_ptr;
      if (reg) {
        client->emitEvent("port-registered"); 
      } else {
        client->emitEvent("port-unregistered"); 
      }
    }

  public:

    NAN_METHOD (emitEvent) {
      NanScope();
      fprintf(stderr, "EVENT");
      NanReturnUndefined();
    }

    static void Init() {
      Local<FunctionTemplate> t =
        NanNew<FunctionTemplate>(Client::New);
      NanAssignPersistent(constructor, t);
      t->SetClassName(NanNew("Client"));
      t->InstanceTemplate()->SetInternalFieldCount(1);
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

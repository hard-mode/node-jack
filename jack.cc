#pragma GCC diagnostic ignored "-Wwrite-strings" // mmm, pragma!

#include <nan.h>
#include <node.h>
#include <uv.h>
#include <jack/jack.h>
//#include "jack.h"

using node::ObjectWrap;
using v8::Context;
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

    Persistent<Object>  self;

    static NAN_METHOD(New) {
      NanScope();
      Local<Object> self = args.This();
      Client * c = new Client(* NanUtf8String(args[0]));
      c->Wrap(self);
      NanAssignPersistent(c->self, self);
      NanReturnValue(self);
    }

    // native constructor and destructor
    // handles setup and teardown of jack client

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

    ~Client () {
      jack_client_close(client);
    }

    // static callbacks for jack hook into libuv

    struct event_args {
      Client     * c;
      const char * evt;
      void       * arg;
    };

    uv_work_t * baton;
    uv_sem_t    semaphore;

    static void uv_work_plug (uv_work_t * task) {}

    static void emit_event
      ( uv_work_t * baton
      , int         status )
    {
      event_args * args = (event_args *) baton->data;

      NanScope();
      Local<Object> self   = NanNew(args->c->self);
      Local<Value>  argv[] = { NanNew(args->evt), NanNew(args->arg) };
      Local<Function>::Cast(self->Get(NanNew("emit")))->Call(self, 2, argv);

      delete baton;
      args->c->baton = NULL;
      uv_sem_post(&(args->c->semaphore));
    }

    static void callback
      ( void           * client_ptr
      , const char     * event
      , void           * arg)
    {
      Client * c = static_cast<Client*>(client_ptr);
      if (c->baton) {
        uv_sem_wait(&(c->semaphore));
        uv_sem_destroy(&(c->semaphore));
      }
      c->baton = new uv_work_t();
      if (uv_sem_init(&(c->semaphore), 0) < 0) return;

      event_args args = { c, event, arg };
      c->baton->data = (void *)(&args);
      uv_queue_work(
        uv_default_loop(), c->baton,
        Client::uv_work_plug, c->emit_event);

      uv_sem_wait(&(c->semaphore));
      uv_sem_destroy(&(c->semaphore));
    }

    static void client_registration_callback
      ( const char * name
      , int          reg
      , void       * client_ptr )
    {
      callback( client_ptr
              , reg ? "client-registered" : "client-unregistered"
              , const_cast<char *>(name));
    }

    static void port_registration_callback
      ( jack_port_id_t port
      , int            reg
      , void         * client_ptr )
    {
      callback( client_ptr
              , reg ? "port-registered" : "port-unregistered"
              , NULL);
    }

  public:

    // create prototype

    static void Init() {
      Local<FunctionTemplate> t = NanNew<FunctionTemplate>(Client::New);
      NanAssignPersistent(constructor, t);
      t->SetClassName(NanNew("Client"));
      t->InstanceTemplate()->SetInternalFieldCount(1);
    }

};

void init (Handle<Object> exports) {
  Client::Init();
  exports->Set(NanNew("Client"), NanNew(constructor)->GetFunction());
}

NODE_MODULE(jack, init)

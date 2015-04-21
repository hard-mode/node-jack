#pragma GCC diagnostic ignored "-Wwrite-strings" // mmm, pragma!

#include <nan.h>
#include <node.h>
#include <uv.h>
#include <jack/jack.h>
//#include "jack.h"

using node::ObjectWrap;
using v8::Array;
using v8::Context;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
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
      Local<Object> self = args.Holder();
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

      jack_set_port_connect_callback ( client
                                     , port_connect_callback
                                     , this );

      jack_activate(client);

    }

    ~Client () {
      jack_client_close(client);
    }

    // port methods
    static NAN_METHOD(Connect) {
      NanScope();
      Client * c = ObjectWrap::Unwrap<Client>(args.Holder());
      jack_connect(c->client,
        *NanUtf8String(args[0]), *NanUtf8String(args[1]));
      NanReturnUndefined();
    }

    static NAN_METHOD(Disconnect) {
      NanScope();
      Client * c = ObjectWrap::Unwrap<Client>(args.Holder());
      jack_disconnect(c->client,
        *NanUtf8String(args[0]), *NanUtf8String(args[1]));
      NanReturnUndefined();
    }

    static NAN_METHOD(GetPorts) {
      NanScope();
      Client * c = ObjectWrap::Unwrap<Client>(args.Holder());
      const char ** ports;
      size_t        count;
      uint32_t      i;

      ports = jack_get_ports(c->client, NULL, JACK_DEFAULT_AUDIO_TYPE, 0);
      count = 0;
      while (ports[count] != NULL) count++;
      Local<Array> a = NanNew<Array>(count);
      for (i = 0; i < count; i++) a->Set(i, NanNew(ports[i]));

      ports = jack_get_ports(c->client, NULL, JACK_DEFAULT_MIDI_TYPE, 0);
      count = 0;
      while (ports[count] != NULL) count++;
      Local<Array> m = NanNew<Array>(count);
      for (i = 0; i < count; i++) m->Set(i, NanNew(ports[i]));

      Local<Object> p = NanNew<Object>();
      p->Set(NanNew("audio"), a);
      p->Set(NanNew("midi"),  m);

      NanReturnValue(p);
    }

    // static callbacks for jack events
    // manually hooking into libuv event loop

    struct event_args {
      Client     * c;
      const char * evt;
      void       * arg;
    };

    uv_work_t * baton;
    uv_sem_t    semaphore;

    static void uv_work_plug (uv_work_t * task) {}

    template <typename T>
    static void emit_event
      ( uv_work_t * baton
      , int         status )
    {
      event_args * args = (event_args *) baton->data;

      NanScope();
      Local<Object> self   = NanNew(args->c->self);
      Local<Value>  argv[] = { NanNew(args->evt), NanNew((T)args->arg) };
      Local<Function>::Cast(self->Get(NanNew("emit")))->Call(self, 2, argv);

      delete baton;
      args->c->baton = NULL;
      uv_sem_post(&(args->c->semaphore));
    }

    template <typename T>
    static void callback
      ( void       * client_ptr
      , const char * event
      , void       * arg)
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
      uv_queue_work(uv_default_loop(), c->baton, uv_work_plug, emit_event<T>);

      uv_sem_wait(&(c->semaphore));
      uv_sem_destroy(&(c->semaphore));
    }

    static void client_registration_callback
      ( const char * name
      , int          reg
      , void       * client_ptr )
    {
      callback<char *>
        ( client_ptr
        , reg ? "client-registered" : "client-unregistered"
        , const_cast<char *>(name));
    }

    static void port_registration_callback
      ( jack_port_id_t port
      , int            reg
      , void         * client_ptr )
    {
      Client * c = static_cast<Client*>(client_ptr);
      callback<char *>
        ( client_ptr
        , reg ? "port-registered" : "port-unregistered"
        , const_cast<char *>(
            jack_port_name(jack_port_by_id(c->client, port))));
    }

    static void port_connect_callback
      ( jack_port_id_t a
      , jack_port_id_t b
      , int            conn
      , void         * client_ptr )
    {

      const char * argv[2] =
        { const_cast<char *>(jack_port_name(jack_port_by_id(
            static_cast<Client*>(client_ptr)->client, a)))
        , const_cast<char *>(jack_port_name(jack_port_by_id(
            static_cast<Client*>(client_ptr)->client, b))) };

      callback<char **>
        ( client_ptr
        , conn ? "connect" : "disconnect"
        , argv);

    }

  public:

    // create prototype

    static void Init() {
      NanScope();

      Local<FunctionTemplate> t = NanNew<FunctionTemplate>(Client::New);
      NanAssignPersistent(constructor, t);
      t->SetClassName(NanNew("Client"));
      t->InstanceTemplate()->SetInternalFieldCount(1);
      NODE_SET_PROTOTYPE_METHOD(t, "connect",    Connect);
      NODE_SET_PROTOTYPE_METHOD(t, "disconnect", Disconnect);
      NODE_SET_PROTOTYPE_METHOD(t, "getPorts",   GetPorts);

      //Local<ObjectTemplate>   p = t->PrototypeTemplate();
      //p->SetAccessor(NanNew("ports"), GetPorts);
    }

};

void init (Handle<Object> exports) {
  Client::Init();
  exports->Set(NanNew("Client"), NanNew(constructor)->GetFunction());
}

NODE_MODULE(jack, init)

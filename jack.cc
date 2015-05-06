#pragma GCC diagnostic ignored "-Wwrite-strings" // mmm, pragma!

#include <nan.h>
#include <node.h>
#include <uv.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#define MAX_PORTS 64

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

//template <typename T>
//class CallbackWorker : public NanAsyncWorker {};

//class ClientRegistrationCallbackWorker : CallbackWorker<JackClientRegistrationCallback> {};

//class PortRegistrationCallbackWorker : CallbackWorker<JackPortRegistrationCallback> {};

//class PortConnectCallbackWorker : CallbackWorker<JackPortConnectCallback> {};


//template <typename T>
//class EmitEventWorker : public NanAsyncWorker {
  //void HandleOKCallback () {
    //NanScope();
    //Local<Object> self   = NanNew(args->c->self);
    //Local<Value>  argv[] = { NanNew(args->evt), NanNew((T)args->arg) };
    //Local<Function>::Cast(self->Get(NanNew("emit")))->Call(self, 2, argv);
  //}

  //Handle<Object> 
//};

class Client : public ObjectWrap {

  private:

    // node constructor

    Persistent<Object> self;

    static NAN_METHOD(New) {
      NanScope();

      if (!args.IsConstructCall()) {
        return NanThrowTypeError("Use the new operator to create instances of this object.");
      }

      Local<Object> self = args.Holder();
      Client      * c    = new Client(* NanUtf8String(args[0]));
      c->Wrap(self);
      NanAssignPersistent(c->self, self);

      NanReturnValue(self);
    }

    // native constructor and destructor
    // handles setup and teardown of jack client

    jack_client_t * client;

    Client (const char * name) {

      midi_output_count = 0;

      client = jack_client_open(name, JackNullOption, NULL);

      jack_set_process_callback(
        client, process_callback,             this );

      jack_set_client_registration_callback(
        client, client_registration_callback, this );

      jack_set_port_registration_callback(
        client, port_registration_callback,   this );

      jack_set_port_connect_callback(
        client, port_connect_callback,        this );

      jack_activate(client);

    }

    ~Client () {
      if (client) jack_client_close(client);
    }

    // close client

    static NAN_METHOD(Close) {
      NanScope();
      Client * c = ObjectWrap::Unwrap<Client>(args.Holder());
      jack_client_close(c->client);
      c->client = NULL;
      NanReturnUndefined();
    }

    // connect and disconnect ports

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

    // get list of all ports

    Handle<Array> get_ports
      ( const char  * port_name_pattern
      , const char  * type_name_pattern
      , unsigned long flags )
    {
      NanEscapableScope();

      Handle<Array> a;
      const char ** ports;
      size_t        count;
      uint32_t      i;

      ports = jack_get_ports(client, port_name_pattern, type_name_pattern, flags);
      if (ports == NULL) {
        return NanEscapeScope(NanNew<Array>(0)); 
      } else {
        count = 0;
        while (ports[count] != NULL) count++;
        a = NanNew<Array>(count);
        for (i = 0; i < count; i++) a->Set(i, NanNew(ports[i]));
      }

      return NanEscapeScope(a);
    }

    static NAN_METHOD(GetPorts) {
      NanScope();

      Client      * c = ObjectWrap::Unwrap<Client>(args.Holder());
      Local<Object> p = NanNew<Object>();

      Local<Object> a = NanNew<Object>();
      a->Set(NanNew("in"), c->get_ports(NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput));
      a->Set(NanNew("out"), c->get_ports(NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput));
      p->Set(NanNew("audio"), a);

      Local<Object> m = NanNew<Object>();
      m->Set(NanNew("in"), c->get_ports(NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput));
      m->Set(NanNew("out"), c->get_ports(NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput));
      p->Set(NanNew("midi"), m);

      NanReturnValue(p);
    }

    // create ports

    static NAN_METHOD(RegisterAudioInput) {
      NanScope();

      Client      * c = ObjectWrap::Unwrap<Client>(args.Holder());
      Local<Object> p = NanNew<Object>();

      jack_port_register( c->client
                        , *NanUtf8String(args[0])
                        , JACK_DEFAULT_AUDIO_TYPE
                        , JackPortIsInput
                        , 0);

      NanReturnValue(p);
    }
    
    static NAN_METHOD(RegisterAudioOutput) {
      NanScope();

      Client      * c = ObjectWrap::Unwrap<Client>(args.Holder());
      Local<Object> p = NanNew<Object>();

      jack_port_register( c->client
                        , *NanUtf8String(args[0])
                        , JACK_DEFAULT_AUDIO_TYPE
                        , JackPortIsOutput
                        , 0);

      NanReturnValue(p);
    }

    int           midi_output_count;
    jack_port_t * midi_outputs [MAX_PORTS];
    
    static NAN_METHOD(RegisterMIDIInput) {
      NanScope();

      Client      * c = ObjectWrap::Unwrap<Client>(args.Holder());
      Local<Object> p = NanNew<Object>();

      jack_port_register( c->client
                        , *NanUtf8String(args[0])
                        , JACK_DEFAULT_MIDI_TYPE
                        , JackPortIsInput
                        , 0);

      NanReturnValue(p);
    }
    
    static NAN_METHOD(RegisterMIDIOutput) {
      NanScope();

      Client      * c = ObjectWrap::Unwrap<Client>(args.Holder());

      c->midi_outputs[c->midi_output_count] = jack_port_register(
        c->client, *NanUtf8String(args[0]),
        JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

      c->midi_output_count++;

      NanReturnValue(NanNew(c->midi_output_count-1));
    }

    // for manually hooking into libuv event loop

    uv_work_t * baton;
    uv_work_t * process_baton;
    uv_sem_t    semaphore;
    uv_sem_t    process_semaphore;
    static void uv_work_plug (uv_work_t * task) {}

    // jack process callback

    struct process_callback_args {
      Client       * c;
      jack_nframes_t nframes;
    };

    static int process_callback
      ( jack_nframes_t nframes
      , void         * arg)
    {
      return static_cast<Client*>(arg)->_process_callback(nframes);
    }

    int _process_callback ( jack_nframes_t nframes ) {

      if (process_baton) {
        uv_sem_wait(&process_semaphore);
        uv_sem_destroy(&process_semaphore);
      }
      process_baton = new uv_work_t();

      if (uv_sem_init(&process_semaphore, 0) < 0) { perror("uv_sem_init"); return 1; }

      void * midi_out_buffer;
      for (int i = 0; i < midi_output_count; i++) {
        midi_out_buffer = jack_port_get_buffer(midi_outputs[i], nframes);
        jack_midi_clear_buffer(midi_out_buffer);
      }

      process_callback_args args = { this, nframes };
      process_baton->data = (void *)(&args);
      uv_queue_work(uv_default_loop(), process_baton, uv_work_plug, _process_callback_uv);
      uv_sem_wait(&process_semaphore);
      uv_sem_destroy(&process_semaphore);

      return 0;
    };

    static void _process_callback_uv
      ( uv_work_t * baton
      , int         status )
    {
      event_args * args = (event_args *) baton->data;
      NanScope();

      // TODO write queued midi events to midi output port buffer

      delete baton;
      args->c->process_baton = NULL;
      uv_sem_post(&(args->c->process_semaphore));
      NanReturnUndefined();
    }

    // static callbacks for jack events

    struct event_args {
      Client     * c;
      const char * evt;
      void       * arg;
    };

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
      NODE_SET_PROTOTYPE_METHOD(t, "close",               Close);
      NODE_SET_PROTOTYPE_METHOD(t, "connect",             Connect);
      NODE_SET_PROTOTYPE_METHOD(t, "disconnect",          Disconnect);
      NODE_SET_PROTOTYPE_METHOD(t, "getPorts",            GetPorts);
      NODE_SET_PROTOTYPE_METHOD(t, "registerMIDIInput",   RegisterMIDIInput);
      NODE_SET_PROTOTYPE_METHOD(t, "registerMIDIOutput",  RegisterMIDIOutput);
      NODE_SET_PROTOTYPE_METHOD(t, "registerAudioInput",  RegisterAudioInput);
      NODE_SET_PROTOTYPE_METHOD(t, "registerAudioOutput", RegisterAudioOutput);

      //Local<ObjectTemplate>   p = t->PrototypeTemplate();
      //p->SetAccessor(NanNew("ports"), GetPorts);
    }

};

void init (Handle<Object> exports) {
  Client::Init();
  exports->Set(NanNew("Client"), NanNew(constructor)->GetFunction());
}

NODE_MODULE(jack, init)

#pragma GCC diagnostic ignored "-Wwrite-strings" // mmm, pragma!

#include <nan.h>
#include <node.h>
#include <queue>
#include <uv.h>
#include <vector>
#include <jack/jack.h>
#include <jack/types.h>
#include <jack/midiport.h>

#define MAX_PORTS 64

using node::ObjectWrap;
using std::queue;
using std::vector;
using v8::Array;
using v8::Context;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::Integer;
using v8::Local;
using v8::Number;
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

      if (!args.IsConstructCall()) {
        return NanThrowTypeError("Use the new operator to create instances of this object.");
      }

      Local<Object> self = args.Holder();
      Client      * c    = new Client(*NanUtf8String(args[0]));
      c->Wrap(self);
      NanAssignPersistent(c->self, self);

      NanReturnValue(self);
    }

    // native constructor and destructor
    // handles setup and teardown of jack client

    jack_client_t * client;

    Client (const char * name) {

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

    // audio io

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

    // midi io

    class MidiEvent {
      public:
        jack_nframes_t frame;
        unsigned char  data[3];

        MidiEvent () {};
        MidiEvent(jack_nframes_t _frame, unsigned char* _data) {
          frame = _frame;
          memcpy(data, _data, sizeof(unsigned char)*3);
        }
    };

    struct midi_port_t {
      jack_port_t    * port;
      queue<MidiEvent> events;
    };

    vector<midi_port_t> midi_outputs;
    
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

      Client    * c = ObjectWrap::Unwrap<Client>(args.Holder());
      midi_port_t p = {
        jack_port_register(
          c->client, *NanUtf8String(args[0]),
          JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0),
        queue<MidiEvent>()
      };
      c->midi_outputs.push_back(p);

      NanReturnValue(NanNew<Number>(c->midi_outputs.size()-1));
    }

    static NAN_METHOD(SendMIDI) {
      NanScope();

      Client * c = ObjectWrap::Unwrap<Client>(args.Holder());
      midi_port_t * m = &c->midi_outputs[args[0]->Uint32Value()];
      uint32_t time = args[0]->Uint32Value();
      unsigned char msg[] = { (unsigned char)args[1]->Uint32Value()
                            , (unsigned char)args[2]->Uint32Value()
                            , (unsigned char)args[3]->Uint32Value() };
      m->events.push(MidiEvent(time, msg));

      NanReturnUndefined();
    }

    // for manually hooking into libuv event loop

    uv_work_t * baton;
    uv_sem_t    semaphore;
    uv_work_t * process_baton;
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

      // send queued midi events to corresponding output ports
      void           * midi_out_buffer;
      MidiEvent      * evt = NULL;
      unsigned char  * buf;
      jack_nframes_t   start = jack_last_frame_time(client);

      for (size_t i = 0; i < midi_outputs.size(); i++) {

        // get and clear midi output buffer
        midi_out_buffer = jack_port_get_buffer(midi_outputs[i].port, nframes);
        jack_midi_clear_buffer(midi_out_buffer);

        // if there are no events in the queue move on to next port
        // since calling front() on an empty queue is undefined
        if (midi_outputs[i].events.empty()) continue;

        // get first event in queue
        evt = &midi_outputs[i].events.front();

        for (jack_nframes_t j = 0; j < nframes; j++) {

          // previous iterations of the loop may leave the queue empty
          if (evt == NULL) break;

          if (evt->frame <= start + j) {
            // write frame to output buffer
            buf    = jack_midi_event_reserve(midi_out_buffer, j, 3);
            buf[0] = evt->data[0];
            buf[1] = evt->data[1];
            buf[2] = evt->data[2];

            // pop queue and get next event
            midi_outputs[i].events.pop();
            evt = midi_outputs[i].events.empty()
              ? NULL
              : &midi_outputs[i].events.front();

          }

        }

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
      NanScope();

      event_args * args = (event_args *) baton->data;
      Local<Object> self   = NanNew(args->c->self);
      Local<Value>  argv[] = { NanNew(args->evt), NanNew((T)args->arg) };
      NanMakeCallback(self, "emit", 2, argv);
      //NanMakeCallback(NanGetCurrentContext()->Global(),
                      //Local<Function>::Cast(self->Get(NanNew("emit"))),
                      ////self->Get(NanNew("emit")),
                      //2, argv);
      //Local<Function>::Cast(self->Get(NanNew("emit")))->Call(self, 2, argv);

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
      NODE_SET_PROTOTYPE_METHOD(t, "sendMidi",            SendMIDI);

      //Local<ObjectTemplate>   p = t->PrototypeTemplate();
      //p->SetAccessor(NanNew("ports"), GetPorts);
    }

};

void init (Handle<Object> exports) {
  Client::Init();
  exports->Set(NanNew("Client"), NanNew(constructor)->GetFunction());
}

NODE_MODULE(jack, init)

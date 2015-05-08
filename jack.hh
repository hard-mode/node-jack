#ifndef __JACK_HH__
#define __JACK_HH__

#pragma GCC diagnostic ignored "-Wwrite-strings" // mmm, pragma!

#include <nan.h>
#include <node.h>
#include <queue>
#include <uv.h>
#include <vector>
#include <jack/jack.h>
#include <jack/types.h>
#include <jack/midiport.h>

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

    static NAN_METHOD(New);

    // native constructor and destructor
    // handles setup and teardown of jack client

    jack_client_t * client;

    Client (const char * name);

    ~Client ();

    // close client

    static NAN_METHOD(Close);

    // connect and disconnect ports

    static NAN_METHOD(Connect);

    static NAN_METHOD(Disconnect);

    // get list of all ports

    Handle<Array> get_ports
      ( const char  * port_name_pattern
      , const char  * type_name_pattern
      , unsigned long flags );

    static NAN_METHOD(GetPorts);

    // audio io

    static NAN_METHOD(RegisterAudioInput);    

    static NAN_METHOD(RegisterAudioOutput);

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
    
    static NAN_METHOD(RegisterMIDIInput);    

    static NAN_METHOD(RegisterMIDIOutput);

    static NAN_METHOD(SendMIDI);

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
      , void         * arg);

    int _process_callback ( jack_nframes_t nframes );

    static void _process_callback_uv
      ( uv_work_t * baton
      , int         status );

    // static callbacks for jack events

    struct event_args {
      Client     * c;
      const char * evt;
      void       * arg;
    };

    template <typename T>
    static void emit_event
      ( uv_work_t * baton
      , int         status );

    template <typename T>
    static void callback
      ( void       * client_ptr
      , const char * event
      , void       * arg);

    static void client_registration_callback
      ( const char * name
      , int          reg
      , void       * client_ptr );

    static void port_registration_callback
      ( jack_port_id_t port
      , int            reg
      , void         * client_ptr );

    static void port_connect_callback
      ( jack_port_id_t a
      , jack_port_id_t b
      , int            conn
      , void         * client_ptr );

  public:

    // create prototype

    static void Init();

};

void init (Handle<Object> exports);

#endif

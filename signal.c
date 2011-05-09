#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <jack/jack.h>

#include "wave.h"



static const double MIDDLE_C    = 261.6;
static const double SAMPLE_RATE = 44100;
static const int    BUFFER_SIZE = 44100;

static const char * const CLIENT_NAME   = "signal";
static const char * const SINE_NAME     = "sine";
static const char * const SQUARE_NAME   = "square";
static const char * const TRIANGLE_NAME = "triangle";
static const char * const SAWTOOTH_NAME = "sawtooth";



typedef jack_default_audio_sample_t jack_sample_t;



typedef struct
{
  double       frequency;
  double       sample_rate;

  jack_port_t *sine_port;
  jack_port_t *square_port;
  jack_port_t *triangle_port;
  jack_port_t *sawtooth_port;

  int          current_block;
}
state;



static int process (jack_nframes_t  nframes,
                    void           *arg);



int
main (int   argc,
      char *argv[])
{
  jack_client_t *client = jack_client_open (CLIENT_NAME,
                                            JackNullOption,
                                            NULL);

  state data;

  char *end = argv[1];

  if (argc > 1)
    data.frequency = strtod (argv[1], &end);

  if (end == argv[1])
    data.frequency = MIDDLE_C;

  data.sample_rate = jack_get_sample_rate (client);

  if (data.sample_rate < 1)
    data.sample_rate = SAMPLE_RATE;

  data.sine_port = jack_port_register (client,
                                       SINE_NAME,
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       JackPortIsOutput,
                                       0);

  data.square_port = jack_port_register (client,
                                         SQUARE_NAME,
                                         JACK_DEFAULT_AUDIO_TYPE,
                                         JackPortIsOutput,
                                         0);

  data.triangle_port = jack_port_register (client,
                                           TRIANGLE_NAME,
                                           JACK_DEFAULT_AUDIO_TYPE,
                                           JackPortIsOutput,
                                           0);

  data.sawtooth_port = jack_port_register (client,
                                           SAWTOOTH_NAME,
                                           JACK_DEFAULT_AUDIO_TYPE,
                                           JackPortIsOutput,
                                           0);

  data.current_block = 0;

  jack_set_process_callback (client, process, &data);

  jack_activate (client);

  while (pause ());

  jack_port_unregister (client, data.sine_port);
  jack_port_unregister (client, data.square_port);
  jack_port_unregister (client, data.triangle_port);
  jack_port_unregister (client, data.sawtooth_port);

  jack_deactivate   (client);
  jack_client_close (client);

  return 0;
}



static int
process (jack_nframes_t  nframes,
         void           *arg)
{
  state *data = arg;

  jack_sample_t *sine_buffer     = jack_port_get_buffer (data->sine_port,     nframes);
  jack_sample_t *square_buffer   = jack_port_get_buffer (data->square_port,   nframes);
  jack_sample_t *triangle_buffer = jack_port_get_buffer (data->triangle_port, nframes);
  jack_sample_t *sawtooth_buffer = jack_port_get_buffer (data->sawtooth_port, nframes);

  double period = data->sample_rate / data->frequency;

  int i;

  for (i = 0; i < nframes; i++)
  {
    double current_position = (data->current_block * nframes + i) / period;

    sine_buffer[i]     = sine     (current_position);
    square_buffer[i]   = square   (current_position);
    triangle_buffer[i] = triangle (current_position);
    sawtooth_buffer[i] = sawtooth (current_position);
  }

  data->current_block++;

  return 0;
}

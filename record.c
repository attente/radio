#include <unistd.h>
#include <stdio.h>

#include <jack/jack.h>



static const char * const CLIENT_NAME = "record";
static const char * const INPUT_NAME  = "input";

static const double ZERO = 1E-6;



typedef jack_default_audio_sample_t jack_sample_t;



typedef struct
{
  jack_port_t *input_port;

  int          recording;
}
state;



static int process (jack_nframes_t  nframes,
                    void           *arg);



int
main (int   argc,
      char *argv[])
{
  if (argc != 1)
  {
    printf ("%s > output\n", argv[0]);
    printf ("output [0, %d): bytes per sample\n", (int) sizeof (int));
    printf ("output [%d, ...): samples\n", (int) sizeof (int));

    return 0;
  }

  jack_client_t *client = jack_client_open (CLIENT_NAME,
                                            JackNullOption,
                                            NULL);

  state data;

  data.input_port = jack_port_register (client,
                                        INPUT_NAME,
                                        JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsInput,
                                        0);

  data.recording = 0;

  int size = sizeof (jack_sample_t);

  fwrite (&size, sizeof (size), 1, stdout);
  fflush (stdout);

  jack_set_process_callback (client, process, &data);

  jack_activate (client);

  while (pause ());

  jack_port_unregister (client, data.input_port);

  jack_deactivate   (client);
  jack_client_close (client);

  return 0;
}



static int
process (jack_nframes_t  nframes,
         void           *arg)
{
  state *data = arg;

  jack_sample_t *input_buffer = jack_port_get_buffer (data->input_port, nframes);

  int i;

  #ifdef AUTOSTOP
    data->recording = 0;
  #endif

  for (i = 0; !data->recording && i < nframes; i++)
    data->recording |= input_buffer[i] > ZERO;

  if (data->recording)
  {
    fwrite (input_buffer, sizeof (jack_sample_t), nframes, stdout);
    fflush (stdout);
  }

  return 0;
}

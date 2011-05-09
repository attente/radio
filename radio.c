#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <fftw3.h>
#include <jack/jack.h>

#include "window.h"
#include "wave.h"



static const char * const CLIENT_NAME = "radio";
static const char * const INPUT_NAME  = "input";
static const char * const OUTPUT_NAME = "output";

static const double THRESHOLD   = 0.500;
static const double WINDOW_SIZE = 1.000;



typedef jack_default_audio_sample_t jack_sample_t;



typedef struct
{
  jack_port_t   *input_port;
  jack_port_t   *output_port;

  int            sample_rate;
  int            block_count;
  int            block_size;
  int            block_index;

  jack_sample_t *sample_data;
  double        *window_data;
  double        *input_data;
  fftw_complex  *output_data;
  double        *modulus_data;

  fftw_plan      fft_object;
}
state;



static int    process (jack_nframes_t  nframes,
                       void           *arg);

static double modulus (fftw_complex    z);



int
main (int   argc,
      char *argv[])
{
  jack_client_t *client = jack_client_open (CLIENT_NAME,
                                            JackNullOption,
                                            NULL);

  state data;

  data.input_port = jack_port_register (client,
                                        INPUT_NAME,
                                        JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsInput,
                                        0);

  data.output_port = jack_port_register (client,
                                         OUTPUT_NAME,
                                         JACK_DEFAULT_AUDIO_TYPE,
                                         JackPortIsOutput,
                                         0);

  data.sample_rate = jack_get_sample_rate (client);
  data.block_size  = jack_get_buffer_size (client);
  data.block_count = (int)(WINDOW_SIZE * data.sample_rate / data.block_size);
  data.block_index = 0;

  int buffer_size = data.block_count * data.block_size;

  data.sample_data  = calloc (2 * buffer_size - data.block_size, sizeof (jack_sample_t));
  data.window_data  = malloc (buffer_size * sizeof (double));
  data.input_data   = fftw_malloc (buffer_size * sizeof (double));
  data.output_data  = fftw_malloc ((buffer_size / 2 + 1) * sizeof (fftw_complex));
  data.modulus_data = malloc ((buffer_size / 2 + 1) * sizeof (double));

  int i;

  for (i = 0; i < buffer_size; i++)
    data.window_data[i] = window (buffer_size, i);

  data.fft_object = fftw_plan_dft_r2c_1d (buffer_size, data.input_data, data.output_data, FFTW_MEASURE | FFTW_DESTROY_INPUT);

  jack_set_process_callback (client, process, &data);

  jack_activate (client);

  while (pause ());

  fftw_destroy_plan (data.fft_object);

  free (data.sample_data);
  free (data.window_data);
  fftw_free (data.input_data);
  fftw_free (data.output_data);
  free (data.modulus_data);

  jack_port_unregister (client, data.input_port);
  jack_port_unregister (client, data.output_port);

  jack_deactivate   (client);
  jack_client_close (client);

  return 0;
}



static int
process (jack_nframes_t  nframes,
         void           *arg)
{
  state *data = arg;

  if (nframes != data->block_size)
    return -1;

  jack_sample_t *input_buffer  = jack_port_get_buffer (data->input_port,  nframes);
  jack_sample_t *output_buffer = jack_port_get_buffer (data->output_port, nframes);

  int buffer_size = data->block_count * nframes;
  int block_index = data->block_index % data->block_count;

  memcpy (data->sample_data + buffer_size + block_index * nframes - nframes, input_buffer, nframes * sizeof (jack_sample_t));

  if (block_index)
    memcpy (data->sample_data + block_index * nframes - nframes, input_buffer, nframes * sizeof (jack_sample_t));

  int i;

  for (i = 0; i < buffer_size; i++)
    data->input_data[i] = data->window_data[i] * data->sample_data[block_index * nframes + i];

  fftw_execute (data->fft_object);

  double maximum = 0;

  for (i = 0; i < buffer_size / 2 + 1; i++)
  {
    data->modulus_data[i] = modulus (data->output_data[i]);

    if (maximum < data->modulus_data[i])
      maximum = data->modulus_data[i];
  }

  for (i = 0; i < buffer_size / 2 + 1; i++)
  {
    if (data->modulus_data[i] > THRESHOLD * maximum)
    {
      double amplitude = data->modulus_data[i] / maximum;
      double frequency = i / buffer_size;

      int j;

      for (j = 0; j < nframes; j++)
        output_buffer[j] += amplitude * sine (frequency * j);
    }
  }

  data->block_index++;

  return 0;
}



static double
modulus (fftw_complex z)
{
  return sqrt (z[0] * z[0] + z[1] * z[1]);
}

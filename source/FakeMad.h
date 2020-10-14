// A header-only no-op implementation of enough of libmad to use in Music.cpp
// without actually doing anything.
// Ordinarily provided by <mad.h>

typedef signed long mad_sample_t;
typedef signed long mad_fixed_t;
enum mad_error {
  MAD_ERROR_NONE	   = 0x0000,	/* no error */
};
struct mad_stream {
  unsigned char const *buffer;		/* input bitstream buffer */
  unsigned char const *bufend;		/* end of buffer */
  unsigned char const *next_frame;	/* start of next frame */
  enum mad_error error;			/* error code (see above) */
};

struct mad_frame {
  int options;
};

struct mad_pcm {
  unsigned int samplerate;		/* sampling frequency (Hz) */
  unsigned short channels;		/* number of channels */
  unsigned short length;		/* number of samples per channel */
  mad_fixed_t samples[2][1152];		/* PCM output samples [ch][sample] */
};
struct mad_synth {
  struct mad_pcm pcm;			/* PCM output */
};

# define mad_stream_init(synth)  /* nothing */
# define mad_stream_finish(synth)  /* nothing */
# define mad_frame_init(synth)  /* nothing */
# define mad_frame_finish(synth)  /* nothing */
# define mad_synth_init(synth)  /* nothing */
# define mad_synth_finish(synth)  /* nothing */

# define mad_stream_buffer(a, b, c)  /* nothing */
# define mad_synth_frame(a, b)  /* nothing */

int mad_frame_decode(struct mad_frame *, struct mad_stream *){return 0;}

# define MAD_RECOVERABLE(error)	((error) & 0xff00)
# define MAD_F_FRACBITS		28
# define MAD_F(x)		((mad_fixed_t) (x##L))
# define MAD_F_ONE		MAD_F(0x10000000)

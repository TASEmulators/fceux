/*
 * Copyright (c) 2008-2011, Michael Kohn
 * Copyright (c) 2013, Robin Hahling
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the author nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef H_GWAVI
#define H_GWAVI

#include <stdint.h> /* for size_t */
#include <stddef.h> /* for size_t */

/* structures */
struct gwavi_header_t
{
	unsigned int time_delay;	/* dwMicroSecPerFrame */
	unsigned int data_rate;		/* dwMaxBytesPerSec */
	unsigned int reserved;
	unsigned int flags;		/* dwFlags */
	unsigned int number_of_frames;	/* dwTotalFrames */
	unsigned int initial_frames;	/* dwInitialFrames */
	unsigned int data_streams;	/* dwStreams */
	unsigned int buffer_size;	/* dwSuggestedBufferSize */
	unsigned int width;		/* dwWidth */
	unsigned int height;		/* dwHeight */
	unsigned int time_scale;
	unsigned int playback_data_rate;
	unsigned int starting_time;
	unsigned int data_length;
};

struct gwavi_stream_header_t
{
	char data_type[5];	    /* fccType */
	char codec[5];		    /* fccHandler */
	unsigned int flags;	    /* dwFlags */
	unsigned int priority;
	unsigned int initial_frames;/* dwInitialFrames */
	unsigned int time_scale;    /* dwScale */
	unsigned int data_rate;	    /* dwRate */
	unsigned int start_time;    /* dwStart */
	unsigned int data_length;   /* dwLength */
	unsigned int buffer_size;   /* dwSuggestedBufferSize */
	unsigned int video_quality; /* dwQuality */
	/**
	 * Value between 0-10000. If set to -1, drivers use default quality
	 * value.
	 */
	int audio_quality;
	unsigned int sample_size;   /* dwSampleSize */
};

struct gwavi_stream_format_v_t
{
	unsigned int header_size;
	unsigned int width;
	unsigned int height;
	unsigned short int num_planes;
	unsigned short int bits_per_pixel;
	unsigned int compression_type;
	unsigned int image_size;
	unsigned int x_pels_per_meter;
	unsigned int y_pels_per_meter;
	unsigned int colors_used;
	unsigned int colors_important;
	unsigned int *palette;
	unsigned int palette_count;
};

struct gwavi_stream_format_a_t
{
	unsigned short format_type;
	unsigned int channels;
	unsigned int sample_rate;
	unsigned int bytes_per_second;
	unsigned int block_align;
	unsigned int bits_per_sample;
	unsigned short size;
};

struct gwavi_audio_t
{
	unsigned int channels;
	unsigned int bits;
	unsigned int samples_per_second;
};

class gwavi_t
{
	public:
	gwavi_t(void);
	~gwavi_t(void);

	int open(const char *filename, unsigned int width,
		   unsigned int height, const char *fourcc, unsigned int fps,
			   struct gwavi_audio_t *audio);

	int close(void);

	int add_frame( unsigned char *buffer, size_t len);

	int add_audio( unsigned char *buffer, size_t len);

	int set_codec(const char *fourcc);

	int set_size( unsigned int width, unsigned int height);

	int set_framerate(unsigned int fps);

	private:
	FILE *out;
	struct gwavi_header_t avi_header;
	struct gwavi_stream_header_t stream_header_v;
	struct gwavi_stream_format_v_t stream_format_v;
	struct gwavi_stream_header_t stream_header_a;
	struct gwavi_stream_format_a_t stream_format_a;
	long marker;
	int offsets_ptr;
	int offsets_len;
	long offsets_start;
	unsigned int *offsets;
	int offset_count;
	int bits_per_pixel;
	char fourcc[8];

	// helper functions
	int write_avi_header(FILE *out, struct gwavi_header_t *avi_header);
	int write_stream_header(FILE *out,
			struct gwavi_stream_header_t *stream_header);
	int write_stream_format_v(FILE *out,
			  struct gwavi_stream_format_v_t *stream_format_v);
	int write_stream_format_a(FILE *out,
			  struct gwavi_stream_format_a_t *stream_format_a);
	int write_avi_header_chunk(void);
	int write_index(FILE *out, int count, unsigned int *offsets);
	int check_fourcc(const char *fourcc);
	int write_int(FILE *out, unsigned int n);
	int write_short(FILE *out, unsigned int n);
	int write_chars(FILE *out, const char *s);
	int write_chars_bin(FILE *out, const char *s, int count);

};

#endif /* ndef H_GWAVI */


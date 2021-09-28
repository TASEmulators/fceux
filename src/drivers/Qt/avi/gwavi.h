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
#include <vector>

#pragma pack( push, 2 )

/* structures */
struct gwavi_header_t
{
	uint32_t time_delay;	/* dwMicroSecPerFrame */
	uint32_t data_rate;	/* dwMaxBytesPerSec */
	uint32_t reserved;
	uint32_t flags;		/* dwFlags */
	uint32_t number_of_frames;	/* dwTotalFrames */
	uint32_t initial_frames;	/* dwInitialFrames */
	uint32_t data_streams;	/* dwStreams */
	uint32_t buffer_size;	/* dwSuggestedBufferSize */
	uint32_t width;		/* dwWidth */
	uint32_t height;	/* dwHeight */
	uint32_t time_scale;
	uint32_t playback_data_rate;
	uint32_t starting_time;
	uint32_t data_length;
};


struct gwavi_AVIStreamHeader
{
	char fccType[4];
	char fccHandler[4];
	uint32_t  dwFlags;
	uint16_t  wPriority;
	uint16_t  wLanguage;
	uint32_t  dwInitialFrames;
	uint32_t  dwScale;
	uint32_t  dwRate;
	uint32_t  dwStart;
	uint32_t  dwLength;
	uint32_t  dwSuggestedBufferSize;
	uint32_t  dwQuality;
	uint32_t  dwSampleSize;

	struct
	{
		int16_t  left;
		int16_t  top;
		int16_t  right;
		int16_t  bottom;
	} rcFrame;
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
	int image_width;
	int image_height;
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

struct gwavi_super_indx_t
{
	unsigned long long fpos;
	unsigned int nEntriesInUse;
	char         chunkId[7];
	char         streamId;

	struct {
	   unsigned long long qwOffset;
	   unsigned int       dwSize;
	   unsigned int       dwDuration;
	} aIndex[32];
};

struct gwavi_index_rec_t
{
	long long     fofs;
	unsigned int  len;
	unsigned char type;
	unsigned char keyFrame;
	unsigned char resv[2];
};

#pragma pack( pop )

class gwavi_t
{
	public:
	static const int WORD_SIZE = 2;
	static const unsigned int IF_LIST     = 0x00000001;
	static const unsigned int IF_KEYFRAME = 0x00000010;
	static const unsigned int IF_NO_TIME  = 0x00000100;

	gwavi_t(void);
	~gwavi_t(void);

	int open(const char *filename, unsigned int width,
		   unsigned int height, const char *fourcc, double fps,
			   struct gwavi_audio_t *audio);

	int close(void);

	int add_frame( unsigned char *buffer, size_t len, unsigned int flags = IF_KEYFRAME);

	int add_audio( unsigned char *buffer, size_t len);

	int set_codec(const char *fourcc);

	int set_size( unsigned int width, unsigned int height);

	int set_framerate(double fps);

	int openIn(const char *filename);

	int printHeaders(void);

	private:
	FILE *in;
	FILE *out;
	struct gwavi_header_t avi_header;
	struct gwavi_stream_header_t stream_header_v;
	struct gwavi_stream_format_v_t stream_format_v;
	struct gwavi_super_indx_t stream_index_v;
	struct gwavi_stream_header_t stream_header_a;
	struct gwavi_stream_format_a_t stream_format_a;
	struct gwavi_super_indx_t stream_index_a;
	long long marker;
	long long std_index_base_ofs_v;
	long long std_index_base_ofs_a;
	std::vector <gwavi_index_rec_t> offsets;
	long long movi_fpos;
	int bits_per_pixel;
	int avi_std;
	char fourcc[8];
	char audioEnabled;

	// helper functions
	long long ftell(FILE *fp);
	int fseek(FILE *stream, long long offset, int whence);
	int write_avi_header(FILE *fp, struct gwavi_header_t *avi_header);
	int write_stream_header(FILE *fp,
			struct gwavi_stream_header_t *stream_header);
	int write_stream_format_v(FILE *fp,
			  struct gwavi_stream_format_v_t *stream_format_v);
	int write_stream_format_a(FILE *fp,
			  struct gwavi_stream_format_a_t *stream_format_a);
	int write_stream_std_indx(FILE *fp, struct gwavi_super_indx_t *indx);
	int write_stream_super_indx(FILE *fp, struct gwavi_super_indx_t *indx);
	int write_avi_header_chunk( FILE *fp );
	int write_index1(FILE *fp);
	int check_fourcc(const char *fourcc);
	int write_int(FILE *fp, unsigned int n);
	int write_int64(FILE *out, unsigned long long int n);
	int write_byte(FILE *fp, unsigned char n);
	int write_short(FILE *fp, unsigned int n);
	int write_chars(FILE *fp, const char *s);
	int write_chars_bin(FILE *fp, const char *s, int count);
	int peak_chunk( FILE *fp, long int idx, char *fourcc, unsigned int *size );

	int read_int(FILE *fp, int &n);
	int read_uint(FILE *fp, unsigned int &n);
	int read_short(FILE *fp, int16_t &n);
	int read_short(FILE *fp, int &n);
	int read_ushort(FILE *fp, uint16_t &n);
	int read_chars_bin(FILE *fp, char *s, int count);
	unsigned int readList(int lvl);
	unsigned int readChunk(const char *id, int lvl);
	unsigned int readAviHeader(void);
	unsigned int readStreamHeader(void);
	unsigned int readIndexBlock( unsigned int chunkSize );
};

#endif /* ndef H_GWAVI */


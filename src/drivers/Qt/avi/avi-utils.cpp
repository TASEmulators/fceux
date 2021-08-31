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

/*
 * Set of functions useful to create an AVI file. It is used to write things
 * such as AVI header and so on.
 */

#include <stdio.h>
#include <string.h>

#include "Qt/avi/gwavi.h"

int
gwavi_t::write_avi_header(FILE *fp, struct gwavi_header_t *avi_header)
{
	long marker, t;

	if (write_chars_bin(fp, "avih", 4) == -1) {
		(void)fprintf(stderr, "write_avi_header: write_chars_bin() "
			      "failed\n");
		return -1;
	}
	if ((marker = ftell(fp)) == -1) {
		perror("write_avi_header (ftell)");
		return -1;
	}
	if (write_int(fp, 0) == -1)
		goto write_int_failed;

	if (write_int(fp, avi_header->time_delay) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->data_rate) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->reserved) == -1)
		goto write_int_failed;
	/* dwFlags */
	if (write_int(fp, avi_header->flags) == -1)
		goto write_int_failed;
	/* dwTotalFrames */
	if (write_int(fp, avi_header->number_of_frames) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->initial_frames) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->data_streams) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->buffer_size) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->width) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->height) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->time_scale) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->playback_data_rate) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->starting_time) == -1)
		goto write_int_failed;
	if (write_int(fp, avi_header->data_length) == -1)
		goto write_int_failed;

	if ((t = ftell(fp)) == -1) {
		perror("write_avi_header (ftell)");
		return -1;
	}
	if (fseek(fp, marker, SEEK_SET) == -1) {
		perror("write_avi_header (fseek)");
		return -1;
	}
	if (write_int(fp, (unsigned int)(t - marker - 4)) == -1)
	{
		goto write_int_failed;
	}

	if (fseek(fp, t, SEEK_SET) == -1)
	{
		perror("write_avi_header (fseek)");
		return -1;
	}

	return 0;

write_int_failed:
	(void)fprintf(stderr, "write_avi_header: write_int() failed\n");
	return -1;
}

int
gwavi_t::write_stream_header(FILE *fp, struct gwavi_stream_header_t *stream_header)
{
	long marker, t;

	if (write_chars_bin(fp, "strh", 4) == -1)
		goto write_chars_bin_failed;
	if ((marker = ftell(fp)) == -1) {
		perror("write_stream_header (ftell)");
		return -1;
	}
	if (write_int(fp, 0) == -1)
		goto write_int_failed;

	if (write_chars_bin(fp, stream_header->data_type, 4) == -1)
		goto write_chars_bin_failed;
	if (write_chars_bin(fp, stream_header->codec, 4) == -1)
		goto write_chars_bin_failed;
	if (write_int(fp, stream_header->flags) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_header->priority) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_header->initial_frames) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_header->time_scale) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_header->data_rate) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_header->start_time) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_header->data_length) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_header->buffer_size) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_header->video_quality) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_header->sample_size) == -1)
		goto write_int_failed;
	if (write_int(fp, 0) == -1)
		goto write_int_failed;
	if (write_int(fp, 0) == -1)
		goto write_int_failed;

	if ((t = ftell(fp)) == -1) {
		perror("write_stream_header (ftell)");
		return -1;
	}
	if (fseek(fp, marker, SEEK_SET) == -1) {
		perror("write_stream_header (fseek)");
		return -1;
	}
	write_int(fp, (unsigned int)(t - marker - 4));

	if (fseek(fp, t, SEEK_SET) == -1)
	{
		perror("write_stream_header (fseek)");
		return -1;
	}

	return 0;

write_int_failed:
	(void)fprintf(stderr, "write_stream_header: write_int() failed\n");
	return -1;

write_chars_bin_failed:
	(void)fprintf(stderr, "write_stream_header: write_chars_bin() failed\n");
	return -1;
}

int
gwavi_t::write_stream_format_v(FILE *fp, struct gwavi_stream_format_v_t *stream_format_v)
{
	long marker,t;
	unsigned int i;

	if (write_chars_bin(fp, "strf", 4) == -1) {
		(void)fprintf(stderr, "write_stream_format_v: write_chars_bin()"
			      " failed\n");
		return -1;
	}
	if ((marker = ftell(fp)) == -1) {
		perror("write_stream_format_v (ftell)");
		return -1;
	}
	if (write_int(fp, 0) == -1)
		goto write_int_failed;

	if (write_int(fp, stream_format_v->header_size) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_format_v->width) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_format_v->height) == -1)
		goto write_int_failed;
	if (write_short(fp, stream_format_v->num_planes) == -1) {
		(void)fprintf(stderr, "write_stream_format_v: write_short() "
			      "failed\n");
		return -1;
	}
	if (write_short(fp, stream_format_v->bits_per_pixel) == -1) {
		(void)fprintf(stderr, "write_stream_format_v: write_short() "
			      "failed\n");
		return -1;
	}
	if (write_int(fp, stream_format_v->compression_type) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_format_v->image_size) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_format_v->x_pels_per_meter) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_format_v->y_pels_per_meter) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_format_v->colors_used) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_format_v->colors_important) == -1)
		goto write_int_failed;

	if (stream_format_v->colors_used != 0)
		for (i = 0; i < stream_format_v->colors_used; i++) {
			if (fputc(stream_format_v->palette[i] & 255, fp)
					== EOF)
				goto fputc_failed;
			if (fputc((stream_format_v->palette[i] >> 8) & 255, fp)
					== EOF)
				goto fputc_failed;
			if (fputc((stream_format_v->palette[i] >> 16) & 255, fp)
					== EOF)
				goto fputc_failed;
			if (fputc(0, fp) == EOF)
				goto fputc_failed;
		}

	if ((t = ftell(fp)) == -1) {
		perror("write_stream_format_v (ftell)");
		return -1;
	}
	if (fseek(fp,marker,SEEK_SET) == -1) {
		perror("write_stream_format_v (fseek)");
		return -1;
	}
	if (write_int(fp, (unsigned int)(t - marker - 4)) == -1)
		goto write_int_failed;
	if (fseek(fp, t, SEEK_SET) == -1) {
		perror("write_stream_format_v (fseek)");
		return -1;
	}

	return 0;

write_int_failed:
	(void)fprintf(stderr, "write_stream_format_v: write_int() failed\n");
	return -1;

fputc_failed:
	(void)fprintf(stderr, "write_stream_format_v: fputc() failed\n");
	return -1;
}

int
gwavi_t::write_stream_format_a(FILE *fp, struct gwavi_stream_format_a_t *stream_format_a)
{
	long marker, t;

	if (write_chars_bin(fp, "strf", 4) == -1) {
		(void)fprintf(stderr, "write_stream_format_a: write_chars_bin()"
			      " failed\n");
		return -1;
	}
	if ((marker = ftell(fp)) == -1) {
		perror("write_stream_format_a (ftell)");
		return -1;
	}
	if (write_int(fp, 0) == -1)
		goto write_int_failed;

	if (write_short(fp, stream_format_a->format_type) == -1)
		goto write_short_failed;
	if (write_short(fp, stream_format_a->channels) == -1)
		goto write_short_failed;
	if (write_int(fp, stream_format_a->sample_rate) == -1)
		goto write_int_failed;
	if (write_int(fp, stream_format_a->bytes_per_second) == -1)
		goto write_int_failed;
	if (write_short(fp, stream_format_a->block_align) == -1)
		goto write_short_failed;
	if (write_short(fp, stream_format_a->bits_per_sample) == -1)
		goto write_short_failed;
	if (write_short(fp, stream_format_a->size) == -1)
		goto write_short_failed;

	if ((t = ftell(fp)) == -1) {
		perror("write_stream_format_a (ftell)");
		return -1;
	}
	if (fseek(fp, marker, SEEK_SET) == -1) {
		perror("write_stream_format_a (fseek)");
		return -1;
	}
	if (write_int(fp, (unsigned int)(t - marker - 4)) == -1)
		goto write_int_failed;
	if (fseek(fp, t, SEEK_SET) == -1) {
		perror("write_stream_format_a (fseek)");
		return -1;
	}

	return 0;

write_int_failed:
	(void)fprintf(stderr, "write_stream_format_a: write_int() failed\n");
	return -1;

write_short_failed:
	(void)fprintf(stderr, "write_stream_format_a: write_short() failed\n");
	return -1;
}

int
gwavi_t::write_avi_header_chunk(FILE *fp)
{
	long marker, t;
	long sub_marker;

	if (write_chars_bin(fp, "LIST", 4) == -1)
		goto write_chars_bin_failed;
	if ((marker = ftell(fp)) == -1)
		goto ftell_failed;
	if (write_int(fp, 0) == -1)
		goto write_int_failed;
	if (write_chars_bin(fp, "hdrl", 4) == -1)
		goto write_chars_bin_failed;
	if (write_avi_header(fp, &avi_header) == -1) {
		(void)fprintf(stderr, "write_avi_header_chunk: "
			      "write_avi_header() failed\n");
		return -1;
	}

	if (write_chars_bin(fp, "LIST", 4) == -1)
		goto write_chars_bin_failed;
	if ((sub_marker = ftell(fp)) == -1)
		goto ftell_failed;
	if (write_int(fp, 0) == -1)
		goto write_int_failed;
	if (write_chars_bin(fp, "strl", 4) == -1)
		goto write_chars_bin_failed;
	if (write_stream_header(fp, &stream_header_v) == -1) {
		(void)fprintf(stderr, "write_avi_header_chunk: "
			      "write_stream_header failed\n");
		return -1;
	}
	if (write_stream_format_v(fp, &stream_format_v) == -1) {
		(void)fprintf(stderr, "write_avi_header_chunk: "
			      "write_stream_format_v failed\n");
		return -1;
	}

	if ((t = ftell(fp)) == -1)
		goto ftell_failed;

	if (fseek(fp, sub_marker, SEEK_SET) == -1)
		goto fseek_failed;
	if (write_int(fp, (unsigned int)(t - sub_marker - 4)) == -1)
		goto write_int_failed;
	if (fseek(fp, t, SEEK_SET) == -1)
		goto fseek_failed;

	if (avi_header.data_streams == 2) {
		if (write_chars_bin(fp, "LIST", 4) == -1)
			goto write_chars_bin_failed;
		if ((sub_marker = ftell(fp)) == -1)
			goto ftell_failed;
		if (write_int(fp, 0) == -1)
			goto write_int_failed;
		if (write_chars_bin(fp, "strl", 4) == -1)
			goto write_chars_bin_failed;
		if (write_stream_header(fp, &stream_header_a) == -1) {
			(void)fprintf(stderr, "write_avi_header_chunk: "
				      "write_stream_header failed\n");
			return -1;
		}
		if (write_stream_format_a(fp, &stream_format_a) == -1) {
			(void)fprintf(stderr, "write_avi_header_chunk: "
				      "write_stream_format_a failed\n");
			return -1;
		}

		if ((t = ftell(fp)) == -1)
			goto ftell_failed;
		if (fseek(fp, sub_marker, SEEK_SET) == -1)
			goto fseek_failed;
		if (write_int(fp, (unsigned int)(t - sub_marker - 4)) == -1)
			goto write_int_failed;
		if (fseek(fp, t, SEEK_SET) == -1)
			goto fseek_failed;
	}

	if ((t = ftell(fp)) == -1)
		goto ftell_failed;
	if (fseek(fp, marker, SEEK_SET) == -1)
		goto fseek_failed;
	if (write_int(fp, (unsigned int)(t - marker - 4)) == -1)
		goto write_int_failed;
	if (fseek(fp, t, SEEK_SET) == -1)
		goto fseek_failed;

	return 0;

ftell_failed:
	perror("write_avi_header_chunk (ftell)");
	return -1;

fseek_failed:
	perror("write_avi_header_chunk (fseek)");
	return -1;

write_int_failed:
	(void)fprintf(stderr, "write_avi_header_chunk: write_int() failed\n");
	return -1;

write_chars_bin_failed:
	(void)fprintf(stderr, "write_avi_header_chunk: write_chars_bin() failed\n");
	return -1;
}

int gwavi_t::peak_chunk( FILE *fp, long int idx, char *fourcc, unsigned int *size )
{
	long int cpos, fpos;

	cpos = ftell(fp);

	fpos = movi_fpos + idx;

	fseek( fp, fpos, SEEK_SET );

	read_chars_bin(fp, fourcc, 4);
	fourcc[4] = 0;

	read_uint( fp, *size );

	//printf("Peak Chunk: %s  %u\n", fourcc, *size );

	fseek( fp, cpos, SEEK_SET );

	return 0;
}	

int
gwavi_t::write_index(FILE *fp)
{
	long marker, t;
	unsigned int offset = 4;
	unsigned int r;
	//char fourcc[8];

	if (offsets.size() == 0 )
	{
		return -1;
	}

	if (write_chars_bin(fp, "idx1", 4) == -1) {
		(void)fprintf(stderr, "write_index: write_chars_bin) failed\n");
		return -1;
	}
	if ((marker = ftell(fp)) == -1) {
		perror("write_index (ftell)");
		return -1;
	}
	if (write_int(fp, 0) == -1)
		goto write_int_failed;

	for (size_t i = 0; i < offsets.size(); i++)
	{
		//peak_chunk( fp, offset, fourcc, &r );

		if ( offsets[i].type == 0)
		{
			write_chars(fp, "00dc");
			//printf("Index: %u \n", offset );
		}
		else
		{
			write_chars(fp, "01wb");
		}

		if ( offsets[i].keyFrame )
		{
			if (write_int(fp, 0x10) == -1)
				goto write_int_failed;
		}
		else
		{
			if (write_int(fp, 0x00) == -1)
				goto write_int_failed;
		}
		if (write_int(fp, offset) == -1)
			goto write_int_failed;
		if (write_int(fp, offsets[i].len) == -1)
			goto write_int_failed;

		r = offsets[i].len % WORD_SIZE;

		if ( r > 0 )
		{
			r = WORD_SIZE - r;
		}

		offset = offset + offsets[i].len + 8 + r;
	}

	if ((t = ftell(fp)) == -1) {
		perror("write_index (ftell)");
		return -1;
	}
	if (fseek(fp, marker, SEEK_SET) == -1) {
		perror("write_index (fseek)");
		return -1;
	}
	if (write_int(fp, (unsigned int)(t - marker - 4)) == -1)
		goto write_int_failed;
	if (fseek(fp, t, SEEK_SET) == -1) {
		perror("write_index (fseek)");
		return -1;
	}

	return 0;

write_int_failed:
	(void)fprintf(stderr, "write_index: write_int() failed\n");
	return -1;
}

/**
 * Return 0 if fourcc is valid, 1 non-valid or -1 in case of errors.
 */
int
gwavi_t::check_fourcc(const char *fourcc)
{
	int ret = 0;
	/* list of fourccs from http://fourcc.org/codecs.php */
	const char valid_fourcc[] =
		"3IV1 3IV2 8BPS"
		"AASC ABYR ADV1 ADVJ AEMI AFLC AFLI AJPG AMPG ANIM AP41 ASLC"
		"ASV1 ASV2 ASVX AUR2 AURA AVC1 AVRN"
		"BA81 BINK BLZ0 BT20 BTCV BW10 BYR1 BYR2"
		"CC12 CDVC CFCC CGDI CHAM CJPG CMYK CPLA CRAM CSCD CTRX CVID"
		"CWLT CXY1 CXY2 CYUV CYUY"
		"D261 D263 DAVC DCL1 DCL2 DCL3 DCL4 DCL5 DIV3 DIV4 DIV5 DIVX"
		"DM4V DMB1 DMB2 DMK2 DSVD DUCK DV25 DV50 DVAN DVCS DVE2 DVH1"
		"DVHD DVSD DVSL DVX1 DVX2 DVX3 DX50 DXGM DXTC DXTN"
		"EKQ0 ELK0 EM2V ES07 ESCP ETV1 ETV2 ETVC"
		"FFV1 FLJP FMP4 FMVC FPS1 FRWA FRWD FVF1"
		"GEOX GJPG GLZW GPEG GWLT"
		"H260 H261 H262 H263 H264 H265 H266 H267 H268 H269"
		"HDYC HFYU HMCR HMRR"
		"I263 ICLB IGOR IJPG ILVC ILVR IPDV IR21 IRAW ISME"
		"IV30 IV31 IV32 IV33 IV34 IV35 IV36 IV37 IV38 IV39 IV40 IV41"
		"IV41 IV43 IV44 IV45 IV46 IV47 IV48 IV49 IV50"
		"JBYR JPEG JPGL"
		"KMVC"
		"L261 L263 LBYR LCMW LCW2 LEAD LGRY LJ11 LJ22 LJ2K LJ44 LJPG"
		"LMP2 LMP4 LSVC LSVM LSVX LZO1 LAGS"
		"M261 M263 M4CC M4S2 MC12 MCAM MJ2C MJPG MMES MP2A MP2T MP2V"
		"MP42 MP43 MP4A MP4S MP4T MP4V MPEG MPG4 MPGI MR16 MRCA MRLE"
		"MSVC MSZH"
		"MTX1 MTX2 MTX3 MTX4 MTX5 MTX6 MTX7 MTX8 MTX9"
		"MVI1 MVI2 MWV1"
		"NAVI NDSC NDSM NDSP NDSS NDXC NDXH NDXP NDXS NHVU NTN1 NTN2"
		"NVDS NVHS"
		"NVS0 NVS1 NVS2 NVS3 NVS4 NVS5"
		"NVT0 NVT1 NVT2 NVT3 NVT4 NVT5"
		"PDVC PGVV PHMO PIM1 PIM2 PIMJ PIXL PJPG PVEZ PVMM PVW2"
		"QPEG QPEQ"
		"RGB2 RGBT RLE RLE4 RLE8 RMP4 RPZA RT21 RV20 RV30 RV40 S422 SAN3"
		"SDCC SEDG SFMC SMP4 SMSC SMSD SMSV SP40 SP44 SP54 SPIG SQZ2"
		"STVA STVB STVC STVX STVY SV10 SVQ1 SVQ3"
		"TLMS TLST TM20 TM2X TMIC TMOT TR20 TSCC TV10 TVJP TVMJ TY0N"
		"TY2C TY2N"
		"UCOD ULTI"
		"V210 V261 V655 VCR1 VCR2 VCR3 VCR4 VCR5 VCR6 VCR7 VCR8 VCR9"
		"VDCT VDOM VDTZ VGPX VIDS VIFP VIVO VIXL VLV1 VP30 VP31 VP40"
		"VP50 VP60 VP61 VP62 VP70 VP80 VQC1 VQC2 VQJC VSSV VUUU VX1K"
		"VX2K VXSP VYU9 VYUY"
		"WBVC WHAM WINX WJPG WMV1 WMV2 WMV3 WMVA WNV1 WVC1"
		"X263 X264 XLV0 XMPG XVID"
		"XWV0 XWV1 XWV2 XWV3 XWV4 XWV5 XWV6 XWV7 XWV8 XWV9"
		"XXAN"
		"Y16 Y411 Y41P Y444 Y8 YC12 YUV8 YUV9 YUVP YUY2 YUYV YV12 YV16"
		"YV92"
		"ZLIB ZMBV ZPEG ZYGO ZYYY";

	if (!fourcc) {
		(void)fputs("fourcc cannot be NULL", stderr);
		return -1;
	}
	if (strchr(fourcc, ' ') || !strstr(valid_fourcc, fourcc))
		ret = 1;

	return ret;
}

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
 * Useful IO functions.
 */

#include <stdio.h>

#include "gwavi.h"

long long gwavi_t::ftell(FILE *fp)
{
#ifdef WIN32
	return ::_ftelli64(fp);
#else
	return ::ftell(fp);
#endif
}

int gwavi_t::fseek(FILE *fp, long long offset, int whence)
{
#ifdef WIN32
	return ::_fseeki64( fp, offset, whence );
#else
	return ::fseek( fp, offset, whence );
#endif
}

int
gwavi_t::write_int(FILE *out, unsigned int n)
{
	unsigned char buffer[4];

	buffer[0] = n;
	buffer[1] = n >> 8;
	buffer[2] = n >> 16;
	buffer[3] = n >> 24;

	if (fwrite(buffer, 1, 4, out) != 4)
		return -1;

	return 0;
}

int
gwavi_t::write_short(FILE *out, unsigned int n)
{
	unsigned char buffer[2];

	buffer[0] = n;
	buffer[1] = n >> 8;

	if (fwrite(buffer, 1, 2, out) != 2)
		return -1;

	return 0;
}

int
gwavi_t::write_chars(FILE *out, const char *s)
{
	int t = 0;

	while(s[t] != 0 && t < 255)
		if (fputc(s[t++], out) == EOF)
			return -1;

	return 0;
}

int
gwavi_t::write_chars_bin(FILE *out, const char *s, int count)
{
	if (fwrite(s, 1, count, out) != count)
		return -1;

	return 0;
}

int
gwavi_t::read_int(FILE *in, int &n)
{
	unsigned char buffer[4];

	if (fread(buffer, 1, 4, in) != 4)
		return -1;

	n = (buffer[3] << 24) | (buffer[2] << 16) | 
	    (buffer[1] <<  8) | (buffer[0]);

	return 0;
}

int
gwavi_t::read_uint(FILE *in, unsigned int &n)
{
	unsigned char buffer[4];

	if (fread(buffer, 1, 4, in) != 4)
		return -1;

	n = (buffer[3] << 24) | (buffer[2] << 16) | 
	    (buffer[1] <<  8) | (buffer[0]);

	return 0;
}

int
gwavi_t::read_short(FILE *in, int16_t &n)
{
	unsigned char buffer[2];

	if (fread(buffer, 1, 2, in) != 2)
		return -1;

	n = (buffer[1] << 8) | (buffer[0]);

	return 0;
}

int
gwavi_t::read_ushort(FILE *in, uint16_t &n)
{
	unsigned char buffer[2];

	if (fread(buffer, 1, 2, in) != 2)
		return -1;

	n = (buffer[1] << 8) | (buffer[0]);

	return 0;
}

int
gwavi_t::read_short(FILE *in, int &n)
{
	unsigned char buffer[2];

	if (fread(buffer, 1, 2, in) != 2)
		return -1;

	n = (buffer[1] << 8) | (buffer[0]);

	return 0;
}

int
gwavi_t::read_chars_bin(FILE *in, char *s, int count)
{
	if (fread(s, 1, count, in) != count)
		return -1;

	return 0;
}

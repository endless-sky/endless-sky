/*
 * Internal routine for packing UUIDs
 *
 * Copyright (C) 1996, 1997 Theodore Ts'o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* 
 * Endless sky only uses a small section of libuuid, so rather than pulling in
 * the whole util-linux package as a dependency, I have just copied the
 * Relevant api functions in here
 */

#pragma once 


#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/time.h>

typedef unsigned char uuid_t[16];

struct uuid {
	uint32_t	time_low;
	uint16_t	time_mid;
	uint16_t	time_hi_and_version;
	uint16_t	clock_seq;
	uint8_t	node[6];
};

static int uuid_is_null(const uuid_t uu)
{
	const uuid_t nil = { 0 };

	return !memcmp(uu, nil, sizeof(nil));
}

static void uuid_pack(const struct uuid *uu, uuid_t ptr)
{
	uint32_t	tmp;
	unsigned char	*out = ptr;

	tmp = uu->time_low;
	out[3] = (unsigned char) tmp;
	tmp >>= 8;
	out[2] = (unsigned char) tmp;
	tmp >>= 8;
	out[1] = (unsigned char) tmp;
	tmp >>= 8;
	out[0] = (unsigned char) tmp;

	tmp = uu->time_mid;
	out[5] = (unsigned char) tmp;
	tmp >>= 8;
	out[4] = (unsigned char) tmp;

	tmp = uu->time_hi_and_version;
	out[7] = (unsigned char) tmp;
	tmp >>= 8;
	out[6] = (unsigned char) tmp;

	tmp = uu->clock_seq;
	out[9] = (unsigned char) tmp;
	tmp >>= 8;
	out[8] = (unsigned char) tmp;

	memcpy(out+10, uu->node, 6);
}

static int uuid_parse_range(const char *in_start, const char *in_end, uuid_t uu)
{
	struct uuid	uuid;
	int		i;
	const char	*cp;
	char		buf[3];

	if ((in_end - in_start) != 36)
		return -1;
	for (i=0, cp = in_start; i < 36; i++,cp++) {
		if ((i == 8) || (i == 13) || (i == 18) ||
		    (i == 23)) {
			if (*cp == '-')
				continue;
			return -1;
		}

		if (!isxdigit(*cp))
			return -1;
	}
	errno = 0;
	uuid.time_low = strtoul(in_start, NULL, 16);

	if (!errno)
		uuid.time_mid = strtoul(in_start+9, NULL, 16);
	if (!errno)
		uuid.time_hi_and_version = strtoul(in_start+14, NULL, 16);
	if (!errno)
		uuid.clock_seq = strtoul(in_start+19, NULL, 16);
	if (errno)
		return -1;

	cp = in_start+24;
	buf[2] = 0;
	for (i=0; i < 6; i++) {
		buf[0] = *cp++;
		buf[1] = *cp++;

		errno = 0;
		uuid.node[i] = strtoul(buf, NULL, 16);
		if (errno)
			return -1;
	}

	uuid_pack(&uuid, uu);
	return 0;
}

static int uuid_parse(const char *in, uuid_t uu)
{
	size_t len = strlen(in);
	if (len != 36)
		return -1;

	return uuid_parse_range(in, in + len, uu);
}

static char const hexdigits_lower[] = "0123456789abcdef";
static char const hexdigits_upper[] = "0123456789ABCDEF";

static void uuid_fmt(const uuid_t uuid, char *buf, char const * fmt)
{
	char *p = buf;
	int i;

	for (i = 0; i < 16; i++) {
		if (i == 4 || i == 6 || i == 8 || i == 10) {
			*p++ = '-';
		}
		size_t tmp = uuid[i];
		*p++ = fmt[tmp >> 4];
		*p++ = fmt[tmp & 15];
	}
	*p = '\0';
}

static void uuid_unparse_lower(const uuid_t uu, char *out)
{
	uuid_fmt(uu, out, hexdigits_lower);
}

static void uuid_unpack(const uuid_t in, struct uuid *uu)
{
	const uint8_t	*ptr = in;
	uint32_t		tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	tmp = (tmp << 8) | *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_low = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_mid = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_hi_and_version = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->clock_seq = tmp;

	memcpy(uu->node, ptr, 6);
}


#define UUCMP(u1,u2) if (u1 != u2) return((u1 < u2) ? -1 : 1);

static int uuid_compare(const uuid_t uu1, const uuid_t uu2)
{
	struct uuid	uuid1, uuid2;

	uuid_unpack(uu1, &uuid1);
	uuid_unpack(uu2, &uuid2);

	UUCMP(uuid1.time_low, uuid2.time_low);
	UUCMP(uuid1.time_mid, uuid2.time_mid);
	UUCMP(uuid1.time_hi_and_version, uuid2.time_hi_and_version);
	UUCMP(uuid1.clock_seq, uuid2.clock_seq);
	return memcmp(uuid1.node, uuid2.node, 6);
}
#undef UUCMP

static void crank_random(void)
{
	int i;
	struct timeval tv;
	unsigned int n_pid, n_uid;

	gettimeofday(&tv, NULL);
	n_pid = getpid();
	n_uid = getuid();
	srand((n_pid << 16) ^ n_uid ^ tv.tv_sec ^ tv.tv_usec);

	/* Crank the random number generator a few times */
	gettimeofday(&tv, NULL);
	for (i = (tv.tv_sec ^ tv.tv_usec) & 0x1F; i > 0; i--)
		rand();
}

static int random_get_fd(void)
{
	int i, fd;

	fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
	if (fd == -1)
		fd = open("/dev/random", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (fd >= 0) {
		i = fcntl(fd, F_GETFD);
		if (i >= 0)
			fcntl(fd, F_SETFD, i | FD_CLOEXEC);
	}
	crank_random();
	return fd;
}

#define UL_RAND_READ_ATTEMPTS	8
#define UL_RAND_READ_DELAY	125000	/* microseconds */

static int ul_random_get_bytes(void *buf, size_t nbytes)
{
	unsigned char *cp = (unsigned char *)buf;
	size_t i, n = nbytes;
	int lose_counter = 0;

   int fd = random_get_fd();

   lose_counter = 0;
   if (fd >= 0) {
      while (n > 0) {
         ssize_t x = read(fd, cp, n);
         if (x <= 0) {
            if (lose_counter++ > UL_RAND_READ_ATTEMPTS)
               break;
            usleep(UL_RAND_READ_DELAY);
            continue;
         }
         n -= x;
         cp += x;
         lose_counter = 0;
      }

      close(fd);
   }
	/*
	 * We do this all the time, but this is the only source of
	 * randomness if /dev/random/urandom is out to lunch.
	 */
	crank_random();
	for (cp = (unsigned char*)buf, i = 0; i < nbytes; i++)
		*cp++ ^= (rand() >> 7) & 0xFF;

	return n != 0;
}

static void uuid_generate_random(uuid_t out)
{
	uuid_t	buf;
	struct uuid uu;

   ul_random_get_bytes(buf, sizeof(buf));
   uuid_unpack(buf, &uu);

   uu.clock_seq = (uu.clock_seq & 0x3FFF) | 0x8000;
   uu.time_hi_and_version = (uu.time_hi_and_version & 0x0FFF)
      | 0x4000;
   uuid_pack(&uu, out);
   out += sizeof(uuid_t);
}

static void uuid_copy(uuid_t dst, const uuid_t src)
{
	unsigned char		*cp1;
	const unsigned char	*cp2;
	int			i;

	for (i=0, cp1 = dst, cp2 = src; i < 16; i++)
		*cp1++ = *cp2++;
}

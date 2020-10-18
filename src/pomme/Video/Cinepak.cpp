// Adapted from ffmpeg

// ---- Begin ffmpeg copyright notices ----

/*
 * Cinepak Video Decoder
 * Copyright (C) 2003 The FFmpeg project
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Cinepak video decoder
 * @author Ewald Snel <ewald@rambo.its.tudelft.nl>
 *
 * Cinepak colorspace support (c) 2013 Rl, Aetey Global Technologies AB
 * @author Cinepak colorspace, Rl, Aetey Global Technologies AB
 */

// ---- End ffmpeg copyright notices ----

#include "Video/Cinepak.h"
#include <cstring>
#include <fstream>

class CinepakException: public std::runtime_error
{
public:
	CinepakException(const char* m) : std::runtime_error(m) {}
};

static uint8_t av_clip_uint8(int x)
{
	return x > 255 ? 255 : (x < 0 ? 0 : x);
}

static uint16_t AV_RB16(const uint8_t* in)
{
	return	((uint16_t)in[0] << 8)
		|	((uint16_t)in[1]);
}

static uint32_t AV_RB24(const uint8_t* in)
{
	return	((uint32_t)in[0] << 16)
		|	((uint32_t)in[1] << 8)
		|	(uint32_t)in[2];
}

static uint32_t AV_RB32(const uint8_t* in)
{
	return	((uint32_t)in[0] << 24)
		|	((uint32_t)in[1] << 16)
		|	((uint32_t)in[2] << 8)
		|	(uint32_t)in[3];
}

static void cinepak_decode_codebook (cvid_codebook *codebook,
                                     int chunk_id, int size, const uint8_t *data)
{
	const uint8_t *eod = (data + size);
	uint32_t flag, mask;
	int      i, n;
	uint8_t *p;

	/* check if this chunk contains 4- or 6-element vectors */
	n    = (chunk_id & 0x04) ? 4 : 6;
	flag = 0;
	mask = 0;

	p = codebook[0];
	for (i=0; i < 256; i++) {
		if ((chunk_id & 0x01) && !(mask >>= 1)) {
			if ((data + 4) > eod)
				break;

			flag  = AV_RB32 (data);
			data += 4;
			mask  = 0x80000000;
		}

		if (!(chunk_id & 0x01) || (flag & mask)) {
			int k, kk;

			if ((data + n) > eod)
				break;

			for (k = 0; k < 4; ++k) {
				int r = *data++;
				for (kk = 0; kk < 3; ++kk)
					*p++ = r;
			}
			if (n == 6) {
				int r, g, b, u, v;
				u = *(int8_t *)data++;
				v = *(int8_t *)data++;
				p -= 12;
				for(k=0; k<4; ++k) {
					r = *p++ + v*2;
					g = *p++ - (u/2) - v;
					b = *p   + u*2;
					p -= 2;
					*p++ = av_clip_uint8(r);
					*p++ = av_clip_uint8(g);
					*p++ = av_clip_uint8(b);
				}
			}
		} else {
			p += 12;
		}
	}
}

void cinepak_decode_vectors (
		CinepakContext *s,
		cvid_strip *strip,
		int chunk_id,
		int size,
		const uint8_t *data)
{
	const uint8_t   *eod = (data + size);
	uint32_t         flag, mask;
	uint8_t         *cb0, *cb1, *cb2, *cb3;
	int             x, y;
	char            *ip0, *ip1, *ip2, *ip3;

	flag = 0;
	mask = 0;

	for (y=strip->y1; y < strip->y2; y+=4) {

/* take care of y dimension not being multiple of 4, such streams exist */
		ip0 = ip1 = ip2 = ip3 = s->frame_data0 +
		                        (strip->x1*3) + (y * s->frame_linesize0);
		if(s->avctx_height - y > 1) {
			ip1 = ip0 + s->frame_linesize0;
			if(s->avctx_height - y > 2) {
				ip2 = ip1 + s->frame_linesize0;
				if(s->avctx_height - y > 3) {
					ip3 = ip2 + s->frame_linesize0;
				}
			}
		}
/* to get the correct picture for not-multiple-of-4 cases let us fill each
 * block from the bottom up, thus possibly overwriting the bottommost line
 * more than once but ending with the correct data in place
 * (instead of in-loop checking) */

		for (x=strip->x1; x < strip->x2; x+=4) {
			if ((chunk_id & 0x01) && !(mask >>= 1)) {
				if ((data + 4) > eod)
					throw CinepakException("invalid data");

				flag  = AV_RB32 (data);
				data += 4;
				mask  = 0x80000000;
			}

			if (!(chunk_id & 0x01) || (flag & mask)) {
				if (!(chunk_id & 0x02) && !(mask >>= 1)) {
					if ((data + 4) > eod)
						throw CinepakException("invalid data");

					flag  = AV_RB32 (data);
					data += 4;
					mask  = 0x80000000;
				}

				if ((chunk_id & 0x02) || (~flag & mask)) {
					uint8_t *p;
					if (data >= eod)
						throw CinepakException("invalid data");

					p = strip->v1_codebook[*data++];

					p += 6;
					memcpy(ip3 + 0, p, 3); memcpy(ip3 + 3, p, 3);
					memcpy(ip2 + 0, p, 3); memcpy(ip2 + 3, p, 3);
					p += 3; /* ... + 9 */
					memcpy(ip3 + 6, p, 3); memcpy(ip3 + 9, p, 3);
					memcpy(ip2 + 6, p, 3); memcpy(ip2 + 9, p, 3);
					p -= 9; /* ... + 0 */
					memcpy(ip1 + 0, p, 3); memcpy(ip1 + 3, p, 3);
					memcpy(ip0 + 0, p, 3); memcpy(ip0 + 3, p, 3);
					p += 3; /* ... + 3 */
					memcpy(ip1 + 6, p, 3); memcpy(ip1 + 9, p, 3);
					memcpy(ip0 + 6, p, 3); memcpy(ip0 + 9, p, 3);

				} else if (flag & mask) {
					if ((data + 4) > eod)
						throw CinepakException("invalid data");

					cb0 = strip->v4_codebook[*data++];
					cb1 = strip->v4_codebook[*data++];
					cb2 = strip->v4_codebook[*data++];
					cb3 = strip->v4_codebook[*data++];
					
					memcpy(ip3 + 0, cb2 + 6, 6);
					memcpy(ip3 + 6, cb3 + 6, 6);
					memcpy(ip2 + 0, cb2 + 0, 6);
					memcpy(ip2 + 6, cb3 + 0, 6);
					memcpy(ip1 + 0, cb0 + 6, 6);
					memcpy(ip1 + 6, cb1 + 6, 6);
					memcpy(ip0 + 0, cb0 + 0, 6);
					memcpy(ip0 + 6, cb1 + 0, 6);
				}
			}

			ip0 += 12;  ip1 += 12;
			ip2 += 12;  ip3 += 12;
		}
	}
}

void cinepak_decode_strip (
		CinepakContext *s,
        cvid_strip *strip,
        const uint8_t *data,
        int size)
{
	const uint8_t *eod = (data + size);
	int      chunk_id, chunk_size;

	/* coordinate sanity checks */
	if (strip->x2 > s->width   ||
	    strip->y2 > s->height  ||
	    strip->x1 >= strip->x2 || strip->y1 >= strip->y2)
		throw CinepakException("invalid data");

	while ((data + 4) <= eod) {
		chunk_id   = data[0];
		chunk_size = AV_RB24 (&data[1]) - 4;
		if(chunk_size < 0)
			throw CinepakException("invalid data");

		data      += 4;
		chunk_size = ((data + chunk_size) > eod) ? (eod - data) : chunk_size;

		switch (chunk_id) {

			case 0x20:
			case 0x21:
			case 0x24:
			case 0x25:
				cinepak_decode_codebook (strip->v4_codebook, chunk_id, chunk_size, data);
				break;

			case 0x22:
			case 0x23:
			case 0x26:
			case 0x27:
				cinepak_decode_codebook (strip->v1_codebook, chunk_id, chunk_size, data);
				break;

			case 0x30:
			case 0x31:
			case 0x32:
				cinepak_decode_vectors (s, strip, chunk_id, chunk_size, data);
				return;
		}

		data += chunk_size;
	}

	throw CinepakException("invalid data");
}

void cinepak_predecode_check (CinepakContext *s)
{
	int           num_strips;
	int           encoded_buf_size;

	num_strips  = AV_RB16 (&s->data[8]);
	encoded_buf_size = AV_RB24(&s->data[1]);

	if (s->size < encoded_buf_size)
		throw CinepakException("invalid data");

	if (s->size < 10 /*+ s->sega_film_skip_bytes*/ + num_strips * 12)
		throw CinepakException("invalid data");

	if (num_strips) {
		const uint8_t* data = s->data + 10; //+ s->sega_film_skip_bytes;
		int strip_size = AV_RB24 (data + 1);
		if (strip_size < 12 || strip_size > encoded_buf_size)
			throw CinepakException("invalid data");
	}
}

static void cinepak_decode (CinepakContext *s)
{
	const uint8_t  *eod = (s->data + s->size);
	int           i, strip_size, frame_flags, num_strips;
	int           y0 = 0;

	frame_flags = s->data[0];
	num_strips  = AV_RB16 (&s->data[8]);

	s->data += 10;

	num_strips = std::min(num_strips, CINEPAK_MAX_STRIPS);

//	s->frame->key_frame = 0;

	for (i=0; i < num_strips; i++) {
		if ((s->data + 12) > eod)
			throw CinepakException("invalid data");

		s->strips[i].id = s->data[0];
/* zero y1 means "relative to the previous stripe" */
		if (!(s->strips[i].y1 = AV_RB16 (&s->data[4])))
			s->strips[i].y2 = (s->strips[i].y1 = y0) + AV_RB16 (&s->data[8]);
		else
			s->strips[i].y2 = AV_RB16 (&s->data[8]);
		s->strips[i].x1 = AV_RB16 (&s->data[6]);
		s->strips[i].x2 = AV_RB16 (&s->data[10]);

//		if (s->strips[i].id == 0x10)
//			s->frame->key_frame = 1;

		strip_size = AV_RB24 (&s->data[1]) - 12;
		if (strip_size < 0)
			throw CinepakException("invalid data");
		s->data   += 12;
		strip_size = ((s->data + strip_size) > eod) ? (eod - s->data) : strip_size;

		if ((i > 0) && !(frame_flags & 0x01)) {
			memcpy (s->strips[i].v4_codebook, s->strips[i-1].v4_codebook,
			        sizeof(s->strips[i].v4_codebook));
			memcpy (s->strips[i].v1_codebook, s->strips[i-1].v1_codebook,
			        sizeof(s->strips[i].v1_codebook));
		}

		cinepak_decode_strip (s, &s->strips[i], s->data, strip_size);

		s->data += strip_size;
		y0    = s->strips[i].y2;
	}
}

CinepakContext::CinepakContext(int _width, int _height)
	: strips(CINEPAK_MAX_STRIPS)
{
	avctx_width = _width;
	avctx_height = _height;
	width = (avctx_width + 3) & ~3;
	height = (avctx_height + 3) & ~3;

	frame_data0 = new char[width * height * 3];
	frame_linesize0 = width*3;
	if (!frame_data0)
		throw CinepakException("couldn't allocate frame");
}

void CinepakContext::DecodeFrame(const uint8_t* packet_data, const int packet_size)
{
	if (packet_size < 10)
		throw CinepakException("invalid data -- input buffer too small?");

	this->data = packet_data;
	this->size = packet_size;

	int num_strips = AV_RB16 (&this->data[8]);

	//Empty frame, do not waste time
	if (!num_strips)
		return;

	cinepak_predecode_check(this);
	cinepak_decode(this);
}

CinepakContext::~CinepakContext()
{
	delete[] frame_data0;
}

void CinepakContext::DumpFrameTGA(const char* outFN)
{
	std::ofstream out(outFN, std::ios::out | std::ios::binary);
	uint16_t TGAhead[] = { 0, 2, 0, 0, 0, 0, (uint16_t)width, (uint16_t)height, 24 };
	out.write(reinterpret_cast<char*>(&TGAhead), sizeof(TGAhead));
	out.write(frame_data0, width*height*3);
}

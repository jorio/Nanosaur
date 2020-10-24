#pragma once

#include <cstdint>
#include <vector>

#define CINEPAK_MAX_STRIPS      32

typedef uint8_t cvid_codebook[12];

struct cvid_strip
{
	uint16_t          id;
	uint16_t          x1, y1;
	uint16_t          x2, y2;
	cvid_codebook     v4_codebook[256];
	cvid_codebook     v1_codebook[256];
};

struct CinepakContext
{
	std::vector<cvid_strip> strips;

	int avctx_width;
	int avctx_height;
	int width, height;

	uint8_t* frame_data0;
	int frame_linesize0;

	const uint8_t* data;
	int size;
	
public:
	CinepakContext(int avctx_width, int avctx_height);
	~CinepakContext();
	void DecodeFrame(const uint8_t* packet_data, const int packet_size);
	void DumpFrameTGA(const char* outFN);
};


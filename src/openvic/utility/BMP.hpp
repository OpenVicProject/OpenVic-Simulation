#pragma once

#include <cstdio>
#include <vector>

#include "openvic/types/Colour.hpp"

namespace OpenVic {
	class BMP {
#pragma pack(push)
#pragma pack(1)
		struct header_t {
			uint16_t signature;			// Signature: 0x4d42 (or 'B' 'M')
			uint32_t file_size;			// File size in bytes
			uint16_t reserved1;			// Not used
			uint16_t reserved2;			// Not used
			uint32_t offset;			// Offset to image data in bytes from beginning of file
			uint32_t dib_header_size;	// DIB header size in bytes
			int32_t width_px;			// Width of the image
			int32_t height_px;			// Height of image
			uint16_t num_planes;		// Number of colour planes
			uint16_t bits_per_pixel;	// Bits per pixel
			uint32_t compression;		// Compression type
			uint32_t image_size_bytes;	// Image size in bytes
			int32_t x_resolution_ppm;	// Pixels per meter
			int32_t y_resolution_ppm;	// Pixels per meter
			uint32_t num_colours;		// Number of colours
			uint32_t important_colours; // Important colours
		} header;
#pragma pack(pop)

		FILE* file = nullptr;
		bool header_validated = false;
		uint32_t palette_size = 0;
		std::vector<colour_t> palette;

	public:
		static constexpr uint32_t PALETTE_COLOUR_SIZE = sizeof(colour_t);

		BMP() = default;
		~BMP();

		bool open(char const* filepath);
		bool read_header();
		bool read_palette();
		void close();
		void reset();

		std::vector<colour_t> const& get_palette() const;
	};
}

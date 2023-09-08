#include "BMP.hpp"

#include <cstring>
#include <set>

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

BMP::~BMP() {
	close();
}

bool BMP::open(char const* filepath) {
	reset();
	errno = 0;
	file = fopen(filepath, "rb");
	if (file == nullptr || errno != 0) {
		Logger::error("Failed to open BMP file \"", filepath, "\" (errno = ", errno, ")");
		file = nullptr;
		return false;
	}
	return true;
}

bool BMP::read_header() {
	if (header_validated) {
		Logger::error("BMP header already validated!");
		return false;
	}
	if (file == nullptr) {
		Logger::error("Cannot read BMP header before opening a file");
		return false;
	}
	if (fseek(file, 0, SEEK_SET) != 0) {
		Logger::error("Failed to move to the beginning of the BMP file!");
		return false;
	}
	if (fread(&header, sizeof(header), 1, file) != 1) {
		Logger::error("Failed to read BMP header!");
		return false;
	}

	header_validated = true;

	// Validate constants
	static constexpr uint16_t BMP_SIGNATURE = 0x4d42;
	if (header.signature != BMP_SIGNATURE) {
		Logger::error("Invalid BMP signature: ", header.signature, " (must be ", BMP_SIGNATURE, ")");
		header_validated = false;
	}
	static constexpr uint32_t DIB_HEADER_SIZE = 40;
	if (header.dib_header_size != DIB_HEADER_SIZE) {
		Logger::error("Invalid BMP DIB header size: ", header.dib_header_size, " (must be ", DIB_HEADER_SIZE, ")");
		header_validated = false;
	}
	static constexpr uint16_t NUM_PLANES = 1;
	if (header.num_planes != NUM_PLANES) {
		Logger::error("Invalid BMP plane count: ", header.num_planes, " (must be ", NUM_PLANES, ")");
		header_validated = false;
	}
	static constexpr uint16_t COMPRESSION = 0; // Only support uncompressed BMPs
	if (header.compression != COMPRESSION) {
		Logger::error("Invalid BMP compression method: ", header.compression, " (must be ", COMPRESSION, ")");
		header_validated = false;
	}

	// Validate sizes and dimensions
	// TODO - image_size_bytes can be 0 for non-compressed BMPs
	if (header.file_size != header.offset + header.image_size_bytes) {
		Logger::error("Invalid BMP memory sizes: file size = ", header.file_size, " != ", header.offset + header.image_size_bytes,
			" = ", header.offset, " + ", header.image_size_bytes, " = image data offset + image data size");
		header_validated = false;
	}
	// TODO - support negative widths (i.e. horizontal flip)
	if (header.width_px <= 0) {
		Logger::error("Invalid BMP width: ", header.width_px, " (must be positive)");
		header_validated = false;
	}
	// TODO - support negative heights (i.e. vertical flip)
	if (header.height_px <= 0) {
		Logger::error("Invalid BMP height: ", header.height_px, " (must be positive)");
		header_validated = false;
	}
	// TODO - validate x_resolution_ppm
	// TODO - validate y_resolution_ppm

	// Validate colours
#define VALID_BITS_PER_PIXEL 1, 2, 4, 8, 16, 24, 32
#define STR(x) #x
	static const std::set<uint16_t> BITS_PER_PIXEL { VALID_BITS_PER_PIXEL };
	if (!BITS_PER_PIXEL.contains(header.bits_per_pixel)) {
		Logger::error("Invalid BMP bits per pixel: ", header.bits_per_pixel, " (must be one of " STR(VALID_BITS_PER_PIXEL) ")");
		header_validated = false;
	}
#undef VALID_BITS_PER_PIXEL
#undef STR
	static constexpr uint16_t PALETTE_BITS_PER_PIXEL_LIMIT = 8;
	if (header.num_colours != 0 && header.bits_per_pixel > PALETTE_BITS_PER_PIXEL_LIMIT) {
		Logger::error("Invalid BMP palette size: ", header.num_colours, " (should be 0 as bits per pixel is ", header.bits_per_pixel, " > 8)");
		header_validated = false;
	}
	// TODO - validate important_colours

	palette_size = header.bits_per_pixel > PALETTE_BITS_PER_PIXEL_LIMIT ? 0
		// Use header.num_colours if it's greater than 0 and at most 1 << header.bits_per_pixel
		: 0 < header.num_colours && header.num_colours - 1 >> header.bits_per_pixel == 0 ? header.num_colours
		: 1 << header.bits_per_pixel;

	const uint32_t expected_offset = palette_size * PALETTE_COLOUR_SIZE + sizeof(header);
	if (header.offset != expected_offset) {
		Logger::error("Invalid BMP image data offset: ", header.offset, " (should be ", expected_offset, ")");
		header_validated = false;
	}

	return header_validated;
}

bool BMP::read_palette() {
	if (file == nullptr) {
		Logger::error("Cannot read BMP palette before opening a file");
		return false;
	}
	if (!header_validated) {
		Logger::error("Cannot read palette before BMP header is validated!");
		return false;
	}
	if (palette_size == 0) {
		Logger::error("Cannot read BMP palette - header indicates this file doesn't have one");
		return false;
	}
	if (fseek(file, sizeof(header), SEEK_SET) != 0) {
		Logger::error("Failed to move to the palette in the BMP file!");
		return false;
	}
	palette.resize(palette_size);
	if (fread(palette.data(), palette_size * PALETTE_COLOUR_SIZE, 1, file) != 1) {
		Logger::error("Failed to read BMP header!");
		palette.clear();
		return false;
	}
	return true;
}

void BMP::close() {
	if (file != nullptr) {
		if (fclose(file) != 0)
			Logger::error("Failed to close BMP!");
		file = nullptr;
	}
}

void BMP::reset() {
	close();
	memset(&header, 0, sizeof(header));
	header_validated = false;
	palette_size = 0;
	palette.clear();
}

std::vector<colour_t> const& BMP::get_palette() const {
	return palette;
}

#include "BMP.hpp"

#include <set>

#include "Logger.hpp"

using namespace OpenVic;

BMP::~BMP() {
	close();
}

return_t BMP::open(char const* filepath) {
	reset();
	errno = 0;
	file = fopen(filepath, "rb");
	if (file == nullptr || errno != 0) {
		Logger::error("Failed to open BMP file \"", filepath, "\" (errno = ", errno, ")");
		file = nullptr;
		return FAILURE;
	}
	return SUCCESS;
}

return_t BMP::read_header() {
	if (header_validated) {
		Logger::error("BMP header already validated!");
		return FAILURE;
	}
	if (file == nullptr) {
		Logger::error("Cannot read BMP header before opening a file");
		return FAILURE;
	}
	if (fseek(file, 0, SEEK_SET) != 0) {
		Logger::error("Failed to move to the beginning of the BMP file!");
		return FAILURE;
	}
	if (fread(&header, sizeof(header), 1, file) != 1) {
		Logger::error("Failed to read BMP header!");
		return FAILURE;
	}
	return_t ret = SUCCESS;

	// Validate constants
	static constexpr uint16_t BMP_SIGNATURE = 0x4d42;
	if (header.signature != BMP_SIGNATURE) {
		Logger::error("Invalid BMP signature: ", header.signature, " (must be ", BMP_SIGNATURE, ")");
		ret = FAILURE;
	}
	static constexpr uint32_t DIB_HEADER_SIZE = 40;
	if (header.dib_header_size != DIB_HEADER_SIZE) {
		Logger::error("Invalid BMP DIB header size: ", header.dib_header_size, " (must be ", DIB_HEADER_SIZE, ")");
		ret = FAILURE;
	}
	static constexpr uint16_t NUM_PLANES = 1;
	if (header.num_planes != NUM_PLANES) {
		Logger::error("Invalid BMP plane count: ", header.num_planes, " (must be ", NUM_PLANES, ")");
		ret = FAILURE;
	}
	static constexpr uint16_t COMPRESSION = 0; // Only support uncompressed BMPs
	if (header.compression != COMPRESSION) {
		Logger::error("Invalid BMP compression method: ", header.compression, " (must be ", COMPRESSION, ")");
		ret = FAILURE;
	}

	// Validate sizes and dimensions
	// TODO - image_size_bytes can be 0 for non-compressed BMPs
	if (header.file_size != header.offset + header.image_size_bytes) {
		Logger::error("Invalid BMP memory sizes: file size = ", header.file_size, " != ", header.offset + header.image_size_bytes,
			" = ", header.offset, " + ", header.image_size_bytes, " = image data offset + image data size");
		ret = FAILURE;
	}
	// TODO - support negative widths (i.e. horizontal flip)
	if (header.width_px <= 0) {
		Logger::error("Invalid BMP width: ", header.width_px, " (must be positive)");
		ret = FAILURE;
	}
	// TODO - support negative heights (i.e. vertical flip)
	if (header.height_px <= 0) {
		Logger::error("Invalid BMP height: ", header.height_px, " (must be positive)");
		ret = FAILURE;
	}
	// TODO - validate x_resolution_ppm
	// TODO - validate y_resolution_ppm

	// Validate colours
#define VALID_BITS_PER_PIXEL 1, 2, 4, 8, 16, 24, 32
#define STR(x) #x
	static const std::set<uint16_t> BITS_PER_PIXEL { VALID_BITS_PER_PIXEL };
	if (!BITS_PER_PIXEL.contains(header.bits_per_pixel)) {
		Logger::error("Invalid BMP bits per pixel: ", header.bits_per_pixel, " (must be one of " STR(VALID_BITS_PER_PIXEL) ")");
		ret = FAILURE;
	}
#undef VALID_BITS_PER_PIXEL
#undef STR
	static constexpr uint16_t PALETTE_BITS_PER_PIXEL_LIMIT = 8;
	if (header.num_colours != 0 && header.bits_per_pixel > PALETTE_BITS_PER_PIXEL_LIMIT) {
		Logger::error("Invalid BMP palette size: ", header.num_colours, " (should be 0 as bits per pixel is ", header.bits_per_pixel, " > 8)");
		ret = FAILURE;
	}
	// TODO - validate important_colours

	palette_size = header.bits_per_pixel > PALETTE_BITS_PER_PIXEL_LIMIT ? 0
		// Use header.num_colours if it's greater than 0 and at most 1 << header.bits_per_pixel
		: 0 < header.num_colours && header.num_colours - 1 >> header.bits_per_pixel == 0 ? header.num_colours
		: 1 << header.bits_per_pixel;

	const uint32_t expected_offset = palette_size * PALETTE_COLOUR_SIZE + sizeof(header);
	if (header.offset != expected_offset) {
		Logger::error("Invalid BMP image data offset: ", header.offset, " (should be ", expected_offset, ")");
		ret = FAILURE;
	}

	header_validated = ret == SUCCESS;
	return ret;
}

return_t BMP::read_palette() {
	if (file == nullptr) {
		Logger::error("Cannot read BMP palette before opening a file");
		return FAILURE;
	}
	if (!header_validated) {
		Logger::error("Cannot read palette before BMP header is validated!");
		return FAILURE;
	}
	if (palette_size == 0) {
		Logger::error("Cannot read BMP palette - header indicates this file doesn't have one");
		return FAILURE;
	}
	if (fseek(file, sizeof(header), SEEK_SET) != 0) {
		Logger::error("Failed to move to the palette in the BMP file!");
		return FAILURE;
	}
	palette.resize(palette_size);
	if (fread(palette.data(), palette_size * PALETTE_COLOUR_SIZE, 1, file) != 1) {
		Logger::error("Failed to read BMP header!");
		palette.clear();
		return FAILURE;
	}
	return SUCCESS;
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

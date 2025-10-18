#include "BMP.hpp"

#include <cstring>
#include <span>

#include <fmt/std.h>

#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;

BMP::~BMP() {
	close();
}

bool BMP::open(fs::path const& filepath) {
	reset();
	file.open(filepath, std::ios::binary);
	if (file.fail()) {
		spdlog::error_s("Failed to open BMP file \"{}\"", filepath);
		close();
		return false;
	}
	return true;
}

bool BMP::read_header() {
	if (header_validated) {
		spdlog::error_s("BMP header already validated!");
		return false;
	}
	if (!file.is_open()) {
		spdlog::error_s("Cannot read BMP header before opening a file");
		return false;
	}
	file.seekg(0, std::ios::beg);
	if (file.fail()) {
		spdlog::error_s("Failed to move to the beginning of the BMP file!");
		return false;
	}
	file.read(reinterpret_cast<char*>(&header), sizeof(header));
	if (file.fail()) {
		spdlog::error_s("Failed to read BMP header!");
		return false;
	}

	header_validated = true;

	// Validate constants
	static constexpr uint16_t BMP_SIGNATURE = 0x4d42;
	if (header.signature != BMP_SIGNATURE) {
		spdlog::error_s(
			"Invalid BMP signature: {} (must be {})",
			header.signature, BMP_SIGNATURE
		);
		header_validated = false;
	}
	static constexpr uint32_t DIB_HEADER_SIZE = 40;
	if (header.dib_header_size != DIB_HEADER_SIZE) {
		spdlog::error_s(
			"Invalid BMP DIB header size: {} (must be {})",
			header.dib_header_size, DIB_HEADER_SIZE
		);
		header_validated = false;
	}
	static constexpr uint16_t NUM_PLANES = 1;
	if (header.num_planes != NUM_PLANES) {
		spdlog::error_s(
			"Invalid BMP plane count: {} (must be {})",
			header.num_planes, NUM_PLANES
		);
		header_validated = false;
	}
	static constexpr uint16_t COMPRESSION = 0; // Only support uncompressed BMPs
	if (header.compression != COMPRESSION) {
		spdlog::error_s(
			"Invalid BMP compression method: {} (must be {})",
			header.compression, COMPRESSION
		);
		header_validated = false;
	}

	// Validate sizes and dimensions
	if (header.image_size_bytes > 0 && header.file_size != header.offset + header.image_size_bytes) {
		spdlog::error_s(
			"Invalid BMP memory sizes: file size = {} != {} = {} + {} = image data offset + image data size",
			header.file_size, header.offset + header.image_size_bytes, header.offset, header.image_size_bytes
		);
		header_validated = false;
	}
	// TODO - support negative widths (i.e. horizontal flip)
	if (header.width_px <= 0) {
		spdlog::error_s("Invalid BMP width: {} (must be positive)", header.width_px);
		header_validated = false;
	}
	// TODO - support negative heights (i.e. vertical flip)
	if (header.height_px <= 0) {
		spdlog::error_s("Invalid BMP height: {} (must be positive)", header.height_px);
		header_validated = false;
	}
	// TODO - validate x_resolution_ppm
	// TODO - validate y_resolution_ppm

	// Validate colours
#define VALID_BITS_PER_PIXEL 1, 2, 4, 8, 16, 24, 32
#define STR(x) #x
	static const ordered_set<uint16_t> BITS_PER_PIXEL { VALID_BITS_PER_PIXEL };
	if (!BITS_PER_PIXEL.contains(header.bits_per_pixel)) {
		spdlog::error_s(
			"Invalid BMP bits per pixel: {} (must be one of " STR(VALID_BITS_PER_PIXEL) ")",
			header.bits_per_pixel
		);
		header_validated = false;
	}
#undef VALID_BITS_PER_PIXEL
#undef STR
	static constexpr uint16_t PALETTE_BITS_PER_PIXEL_LIMIT = 8;
	if (header.num_colours != 0 && header.bits_per_pixel > PALETTE_BITS_PER_PIXEL_LIMIT) {
		spdlog::error_s(
			"Invalid BMP palette size: {} (should be 0 as bits per pixel is {} > {})",
			header.num_colours, header.bits_per_pixel, PALETTE_BITS_PER_PIXEL_LIMIT
		);
		header_validated = false;
	}
	// TODO - validate important_colours

	palette_size = header.bits_per_pixel > PALETTE_BITS_PER_PIXEL_LIMIT ? 0
		// Use header.num_colours if it's greater than 0 and at most 1 << header.bits_per_pixel
		: (0 < header.num_colours && (header.num_colours - 1) >> header.bits_per_pixel == 0
		? header.num_colours : 1 << header.bits_per_pixel);

	const uint32_t expected_offset = palette_size * PALETTE_COLOUR_SIZE + sizeof(header);
	if (header.offset != expected_offset) {
		spdlog::error_s(
			"Invalid BMP image data offset: {} (should be {})",
			header.offset, expected_offset
		);
		header_validated = false;
	}

	return header_validated;
}

bool BMP::read_palette() {
	if (palette_read) {
		spdlog::error_s("BMP palette already read!");
		return false;
	}
	if (!file.is_open()) {
		spdlog::error_s("Cannot read BMP palette before opening a file");
		return false;
	}
	if (!header_validated) {
		spdlog::error_s("Cannot read palette before BMP header is validated!");
		return false;
	}
	if (palette_size == 0) {
		spdlog::error_s("Cannot read BMP palette - header indicates this file doesn't have one");
		return false;
	}
	file.seekg(sizeof(header), std::ios::beg);
	if (file.fail()) {
		spdlog::error_s("Failed to move to the palette in the BMP file!");
		return false;
	}
	palette.resize(palette_size);
	file.read(reinterpret_cast<char*>(palette.data()), palette_size * PALETTE_COLOUR_SIZE);
	if (file.fail()) {
		spdlog::error_s("Failed to read BMP palette!");
		palette.clear();
		return false;
	}
	palette_read = true;
	return palette_read;
}

void BMP::close() {
	if (file.is_open()) {
		file.close();
	}
}

void BMP::reset() {
	close();
	memset(&header, 0, sizeof(header));
	header_validated = false;
	palette_size = 0;
	palette.clear();
	pixel_data.clear();
}

int32_t BMP::get_width() const {
	return header.width_px;
}

int32_t BMP::get_height() const {
	return header.height_px;
}

uint16_t BMP::get_bits_per_pixel() const {
	return header.bits_per_pixel;
}

std::span<const BMP::palette_colour_t> BMP::get_palette() const {
	if (!palette_read) {
		spdlog::warn_s("Trying to get BMP palette before loading");
	}
	return palette;
}

bool BMP::read_pixel_data() {
	if (pixel_data_read) {
		spdlog::error_s("BMP pixel data already read!");
		return false;
	}
	if (!file.is_open()) {
		spdlog::error_s("Cannot read BMP pixel data before opening a file");
		return false;
	}
	if (!header_validated) {
		spdlog::error_s("Cannot read pixel data before BMP header is validated!");
		return false;
	}
	file.seekg(header.offset, std::ios::beg);
	if (file.fail()) {
		spdlog::error_s("Failed to move to the pixel data in the BMP file!");
		return false;
	}
	const size_t pixel_data_size = get_width() * get_height() * header.bits_per_pixel / CHAR_BIT;
	pixel_data.resize(pixel_data_size);
	file.read(reinterpret_cast<char*>(pixel_data.data()), pixel_data_size);
	if (file.fail()) {
		spdlog::error_s("Failed to read BMP pixel data!");
		pixel_data.clear();
		return false;
	}
	pixel_data_read = true;
	return pixel_data_read;
}

std::span<const uint8_t> BMP::get_pixel_data() const {
	if (!pixel_data_read) {
		spdlog::warn_s("Trying to get BMP pixel data before loading");
	}
	return pixel_data;
}

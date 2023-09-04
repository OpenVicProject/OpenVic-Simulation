#include "Pop.hpp"

#include <cassert>

#include "openvic/dataloader/NodeTools.hpp"
#include "openvic/map/Province.hpp"
#include "openvic/utility/Logger.hpp"

using namespace OpenVic;

Pop::Pop(PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size)
	: type { new_type },
	  culture { new_culture },
	  religion { new_religion },
	  size { new_size } {
	assert(size > 0);
}

PopType const& Pop::get_type() const {
	return type;
}

Culture const& Pop::get_culture() const {
	return culture;
}

Religion const& Pop::get_religion() const {
	return religion;
}

Pop::pop_size_t Pop::get_size() const {
	return size;
}

Pop::pop_size_t Pop::get_num_promoted() const {
	return num_promoted;
}

Pop::pop_size_t Pop::get_num_demoted() const {
	return num_demoted;
}

Pop::pop_size_t Pop::get_num_migrated() const {
	return num_migrated;
}

Pop::pop_size_t Pop::get_pop_daily_change() const {
	return Pop::get_num_promoted() - (Pop::get_num_demoted() + Pop::get_num_migrated());
}

PopType::PopType(const std::string_view new_identifier, colour_t new_colour,
	strata_t new_strata, sprite_t new_sprite,
	Pop::pop_size_t new_max_size, Pop::pop_size_t new_merge_max_size,
	bool new_state_capital_only, bool new_demote_migrant, bool new_is_artisan, bool new_is_slave)
	: HasIdentifierAndColour { new_identifier, new_colour, true },
	  strata { new_strata },
	  sprite { new_sprite },
	  max_size { new_max_size },
	  merge_max_size { new_merge_max_size },
	  state_capital_only { new_state_capital_only },
	  demote_migrant { new_demote_migrant },
	  is_artisan { new_is_artisan },
	  is_slave { new_is_slave } {
	assert(sprite > 0);
	assert(max_size > 0);
	assert(merge_max_size > 0);
}

PopType::sprite_t PopType::get_sprite() const {
	return sprite;
}

PopType::strata_t PopType::get_strata() const {
	return strata;
}

Pop::pop_size_t PopType::get_max_size() const {
	return max_size;
}

Pop::pop_size_t PopType::get_merge_max_size() const {
	return merge_max_size;
}

bool PopType::get_state_capital_only() const {
	return state_capital_only;
}

bool PopType::get_demote_migrant() const {
	return demote_migrant;
}

bool PopType::get_is_artisan() const {
	return is_artisan;
}

bool PopType::get_is_slave() const {
	return is_slave;
}

PopManager::PopManager() : pop_types { "pop types" } {}

return_t PopManager::add_pop_type(const std::string_view identifier, colour_t colour, PopType::strata_t strata, PopType::sprite_t sprite,
	Pop::pop_size_t max_size, Pop::pop_size_t merge_max_size, bool state_capital_only, bool demote_migrant, bool is_artisan, bool is_slave) {
	if (identifier.empty()) {
		Logger::error("Invalid pop type identifier - empty!");
		return FAILURE;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid pop type colour for ", identifier, ": ", PopType::colour_to_hex_string(colour));
		return FAILURE;
	}
	if (sprite <= 0) {
		Logger::error("Invalid pop type sprite index for ", identifier, ": ", sprite);
		return FAILURE;
	}
	if (max_size <= 0) {
		Logger::error("Invalid pop type max size for ", identifier, ": ", max_size);
		return FAILURE;
	}
	if (merge_max_size <= 0) {
		Logger::error("Invalid pop type merge max size for ", identifier, ": ", merge_max_size);
		return FAILURE;
	}
	return pop_types.add_item({ identifier, colour, strata, sprite, max_size, merge_max_size, state_capital_only, demote_migrant, is_artisan, is_slave });
}

void PopManager::lock_pop_types() {
	pop_types.lock();
}

PopType const* PopManager::get_pop_type_by_identifier(const std::string_view identifier) const {
	return pop_types.get_item_by_identifier(identifier);
}

return_t PopManager::load_pop_type_file(std::filesystem::path const& path, ast::NodeCPtr root) {

	// TODO - pop type loading

	if (pop_types.empty())
		return add_pop_type("test_pop_type", 0xFF0000, PopType::strata_t::POOR, 1, 1, 1, false, false, false, false);
	return SUCCESS;
}

return_t PopManager::load_pop_into_province(Province& province, ast::NodeCPtr root) const {
	static PopType const* type = get_pop_type_by_identifier("test_pop_type");
	Culture const* culture = nullptr;
	Religion const* religion = nullptr;
	Pop::pop_size_t size = 0;

	return_t ret = NodeTools::expect_assign(root, [this, &culture, &religion, &size](std::string_view, ast::NodeCPtr pop_node) -> return_t {
		return NodeTools::expect_dictionary_keys(pop_node, {
			{ "culture", { true, false, [this, &culture](ast::NodeCPtr node) -> return_t {
				return NodeTools::expect_identifier(node, [&culture, this](std::string_view identifier) -> return_t {
					culture = culture_manager.get_culture_by_identifier(identifier);
					if (culture != nullptr) return SUCCESS;
					Logger::error("Invalid pop culture: ", identifier);
					return FAILURE;
				});
			} } },
			{ "religion", { true, false, [this, &religion](ast::NodeCPtr node) -> return_t {
				return NodeTools::expect_identifier(node, [&religion, this](std::string_view identifier) -> return_t {
					religion = religion_manager.get_religion_by_identifier(identifier);
					if (religion != nullptr) return SUCCESS;
					Logger::error("Invalid pop religion: ", identifier);
					return FAILURE;
				});
			} } },
			{ "size", { true, false, [&size](ast::NodeCPtr node) -> return_t {
				return NodeTools::expect_uint(node, [&size](uint64_t val) -> return_t {
					if (val > 0) {
						size = val;
						return SUCCESS;
					} else {
						Logger::error("Invalid pop size: ", val);
						return FAILURE;
					}
				});
			} } },
			{ "militancy", { false, false, NodeTools::success_callback } },
			{ "rebel_type", { false, false, NodeTools::success_callback } }
		});
	});

	if (type != nullptr && culture != nullptr && religion != nullptr && size > 0) {
		if (province.add_pop({ *type, *culture, *religion, size }) != SUCCESS) ret = FAILURE;
	} else {
		Logger::error("Some pop arguments are invalid: type = ", type, ", culture = ", culture, ", religion = ", religion, ", size = ", size);
		ret = FAILURE;
	}
	return ret;
}

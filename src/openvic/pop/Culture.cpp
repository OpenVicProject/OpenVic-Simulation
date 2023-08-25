#include "Culture.hpp"

#include "openvic/dataloader/NodeTools.hpp"

using namespace OpenVic;

GraphicalCultureType::GraphicalCultureType(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

CultureGroup::CultureGroup(const std::string_view new_identifier,
	GraphicalCultureType const& new_unit_graphical_culture_type, bool new_is_overseas)
	: HasIdentifier { new_identifier },
	  unit_graphical_culture_type { new_unit_graphical_culture_type },
	  is_overseas { new_is_overseas } {}

GraphicalCultureType const& CultureGroup::get_unit_graphical_culture_type() const {
	return unit_graphical_culture_type;
}

Culture::Culture(const std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group,
	name_list_t const& new_first_names, name_list_t const& new_last_names)
	: HasIdentifierAndColour { new_identifier, new_colour, true },
	  group { new_group },
	  first_names { new_first_names },
	  last_names { new_last_names } {}

CultureGroup const& Culture::get_group() const {
	return group;
}

CultureManager::CultureManager()
	: graphical_culture_types { "graphical culture types" },
	  culture_groups { "culture groups" },
	  cultures { "cultures" } {}

return_t CultureManager::add_graphical_culture_type(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid culture group identifier - empty!");
		return FAILURE;
	}
	return graphical_culture_types.add_item({ identifier });
}

void CultureManager::lock_graphical_culture_types() {
	graphical_culture_types.lock();
}

GraphicalCultureType const* CultureManager::get_graphical_culture_type_by_identifier(const std::string_view identifier) const {
	return graphical_culture_types.get_item_by_identifier(identifier);
}

return_t CultureManager::add_culture_group(const std::string_view identifier, GraphicalCultureType const* graphical_culture_type, bool is_overseas) {
	if (!graphical_culture_types.is_locked()) {
		Logger::error("Cannot register culture groups until graphical culture types are locked!");
		return FAILURE;
	}
	if (identifier.empty()) {
		Logger::error("Invalid culture group identifier - empty!");
		return FAILURE;
	}
	if (graphical_culture_type == nullptr) {
		Logger::error("Null graphical culture type for ", identifier);
		return FAILURE;
	}
	return culture_groups.add_item({ identifier, *graphical_culture_type, is_overseas });
}

void CultureManager::lock_culture_groups() {
	culture_groups.lock();
}

CultureGroup const* CultureManager::get_culture_group_by_identifier(const std::string_view identifier) const {
	return culture_groups.get_item_by_identifier(identifier);
}

return_t CultureManager::add_culture(const std::string_view identifier, colour_t colour, CultureGroup const* group, Culture::name_list_t const& first_names, Culture::name_list_t const& last_names) {
	if (!culture_groups.is_locked()) {
		Logger::error("Cannot register cultures until culture groups are locked!");
		return FAILURE;
	}
	if (identifier.empty()) {
		Logger::error("Invalid culture identifier - empty!");
		return FAILURE;
	}
	if (group == nullptr) {
		Logger::error("Null culture group for ", identifier);
		return FAILURE;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid culture colour for ", identifier, ": ", Culture::colour_to_hex_string(colour));
		return FAILURE;
	}
	return cultures.add_item({ identifier, colour, *group, first_names, last_names });
}

void CultureManager::lock_cultures() {
	cultures.lock();
}

Culture const* CultureManager::get_culture_by_identifier(const std::string_view identifier) const {
	return cultures.get_item_by_identifier(identifier);
}

return_t CultureManager::load_graphical_culture_type_file(ast::NodeCPtr root) {
	const return_t ret = NodeTools::expect_list(root, [this](ast::NodeCPtr node) -> return_t {
		return NodeTools::expect_identifier(node, [this](std::string_view identifier) -> return_t {
			return add_graphical_culture_type(identifier);
		});
	}, 0, true);
	lock_graphical_culture_types();
	return ret;
}

return_t CultureManager::load_culture_file(ast::NodeCPtr root) {
	if (!graphical_culture_types.is_locked()) {
		Logger::error("Cannot load culture groups until graphical culture types are locked!");
		return FAILURE;
	}

	static const std::string default_unit_graphical_culture_type_identifier = "Generic";
	GraphicalCultureType const* const default_unit_graphical_culture_type = get_graphical_culture_type_by_identifier(default_unit_graphical_culture_type_identifier);
	if (default_unit_graphical_culture_type == nullptr) {
		Logger::error("Failed to find default unit graphical culture type: ", default_unit_graphical_culture_type_identifier);
	}

	return_t ret = NodeTools::expect_dictionary(root, [this, default_unit_graphical_culture_type](std::string_view key, ast::NodeCPtr value) -> return_t {

		GraphicalCultureType const* unit_graphical_culture_type = default_unit_graphical_culture_type;
		bool is_overseas = true;

		return_t ret = NodeTools::expect_dictionary_keys(value, {
			{ "leader", { true, false, NodeTools::success_callback } },
			{ "unit", { false, false, [this, &unit_graphical_culture_type](ast::NodeCPtr node) -> return_t {
				return NodeTools::expect_identifier(node, [this, &unit_graphical_culture_type](std::string_view identifier) -> return_t {
					unit_graphical_culture_type = get_graphical_culture_type_by_identifier(identifier);
					if (unit_graphical_culture_type != nullptr) return SUCCESS;
					Logger::error("Invalid unit graphical culture type: ", identifier);
					return FAILURE;
				});
			} } },
			{ "union", { false, false, NodeTools::success_callback } },
			{ "is_overseas", { false, false, [&is_overseas](ast::NodeCPtr node) -> return_t {
				return NodeTools::expect_bool(node, [&is_overseas](bool val) -> return_t {
					is_overseas = val;
					return SUCCESS;
				});
			} } }
		}, true);
		if (add_culture_group(key, unit_graphical_culture_type, is_overseas) != SUCCESS) ret = FAILURE;
		return ret;
	}, true);
	lock_culture_groups();
	if (NodeTools::expect_dictionary(root, [this](std::string_view culture_group_key, ast::NodeCPtr culture_group_value) -> return_t {

		CultureGroup const* culture_group = get_culture_group_by_identifier(culture_group_key);

		return NodeTools::expect_dictionary(culture_group_value, [this, culture_group](std::string_view key, ast::NodeCPtr value) -> return_t {
			if (key == "leader" || key == "unit" || key == "union" || key == "is_overseas") return SUCCESS;

			colour_t colour = NULL_COLOUR;
			Culture::name_list_t first_names, last_names;

			static const std::function<return_t(Culture::name_list_t&, ast::NodeCPtr)> read_name_list = [](Culture::name_list_t& names, ast::NodeCPtr node) -> return_t {
				return NodeTools::expect_list(node, [&names](ast::NodeCPtr val) -> return_t {
					return NodeTools::expect_identifier_or_string(val, [&names](std::string_view str) -> return_t {
						if (!str.empty()) {
							names.push_back(std::string { str });
							return SUCCESS;
						}
						Logger::error("Empty identifier or string");
						return FAILURE;
					});
				});
			};

			return_t ret = NodeTools::expect_dictionary_keys(value, {
				{ "color", { true, false, [&colour](ast::NodeCPtr node) -> return_t {
					return NodeTools::expect_colour(node, [&colour](colour_t val) -> return_t {
						colour = val;
						return SUCCESS;
					});
				} } },
				{ "first_names", { true, false, [&first_names](ast::NodeCPtr node) -> return_t {
					return read_name_list(first_names, node);
				} } },
				{ "last_names", { true, false, [&last_names](ast::NodeCPtr node) -> return_t {
					return read_name_list(last_names, node);
				} } },
				{ "radicalism", { false, false, NodeTools::success_callback } },
				{ "primary", { false, false, NodeTools::success_callback } }
			});
			if (add_culture(key, colour, culture_group, first_names, last_names) != SUCCESS) ret = FAILURE;
			return ret;
		});
	}, true) != SUCCESS) ret = FAILURE;
	lock_cultures();
	return ret;
}

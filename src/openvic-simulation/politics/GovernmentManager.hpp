#pragma once

#include <functional>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct GovernmentTypeManager {
	private:
		IdentifierRegistry<GovernmentType> IDENTIFIER_REGISTRY(government_type);
		string_set_t PROPERTY(flag_types);

	public:
		bool add_government_type(
			std::string_view identifier, memory::vector<std::reference_wrapper<const Ideology>>&& ideologies, bool elections, bool appoint_ruling_party,
			Timespan term_duration, std::string_view flag_type
		);

		bool load_government_types_file(IdeologyManager const& ideology_manager, ovdl::v2script::ast::Node const* root);

		bool is_valid_flag_type(std::string_view type) const;
	};
}

#pragma once

#include "openvic-simulation/defines/AIDefines.hpp"
#include "openvic-simulation/defines/CountryDefines.hpp"
#include "openvic-simulation/defines/DiplomacyDefines.hpp"
#include "openvic-simulation/defines/EconomyDefines.hpp"
#include "openvic-simulation/defines/GraphicsDefines.hpp"
#include "openvic-simulation/defines/MilitaryDefines.hpp"
#include "openvic-simulation/defines/PopsDefines.hpp"
#include "openvic-simulation/core/object/Date.hpp"
#include "openvic-simulation/core/Property.hpp"

namespace OpenVic {
	struct DefineManager {
	private:
		// Date
		Date PROPERTY(start_date);
		Date PROPERTY(end_date);

		// Other define groups
		AIDefines PROPERTY(ai_defines);
		CountryDefines PROPERTY(country_defines);
		DiplomacyDefines PROPERTY(diplomacy_defines);
		EconomyDefines PROPERTY(economy_defines);
		GraphicsDefines PROPERTY(graphics_defines);
		MilitaryDefines PROPERTY(military_defines);
		PopsDefines PROPERTY(pops_defines);

	public:
		DefineManager();

		constexpr bool in_game_period(Date date) const {
			return date.in_range(start_date, end_date);
		}

		bool load_defines_file(ast::NodeCPtr root);
	};
}

#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct DefineManager;

	struct GraphicsDefines {
		friend struct DefineManager;

	private:
		size_t PROPERTY(cities_sprawl_offset);
		size_t PROPERTY(cities_sprawl_width);
		size_t PROPERTY(cities_sprawl_height);
		size_t PROPERTY(cities_sprawl_iterations);
		size_t PROPERTY(cities_mesh_pool_size_for_country);
		size_t PROPERTY(cities_mesh_pool_size_for_culture);
		size_t PROPERTY(cities_mesh_pool_size_for_generic);
		size_t PROPERTY(cities_mesh_types_count);
		size_t PROPERTY(cities_mesh_sizes_count);
		size_t PROPERTY(cities_special_buildings_pool_size);
		size_t PROPERTY(cities_size_max_population_k);

		GraphicsDefines();

		std::string_view get_name() const;
		NodeTools::node_callback_t expect_defines();
	};
}

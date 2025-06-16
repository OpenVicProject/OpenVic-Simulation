#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct DefineManager;

	struct GraphicsDefines {
		friend struct DefineManager;

	private:
		size_t PROPERTY(cities_sprawl_offset, 0);
		size_t PROPERTY(cities_sprawl_width, 0);
		size_t PROPERTY(cities_sprawl_height, 0);
		size_t PROPERTY(cities_sprawl_iterations, 0);
		size_t PROPERTY(cities_mesh_pool_size_for_country, 0);
		size_t PROPERTY(cities_mesh_pool_size_for_culture, 0);
		size_t PROPERTY(cities_mesh_pool_size_for_generic, 0);
		size_t PROPERTY(cities_mesh_types_count, 0);
		size_t PROPERTY(cities_mesh_sizes_count, 0);
		size_t PROPERTY(cities_special_buildings_pool_size, 0);
		size_t PROPERTY(cities_size_max_population_k, 0);

		GraphicsDefines();

		std::string_view get_name() const;
		NodeTools::node_callback_t expect_defines();
	};
}

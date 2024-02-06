#include "ArtisanalProducer.hpp"

using namespace OpenVic;

ArtisanalProducer::ArtisanalProducer(
	ProductionType const& new_production_type, Good::good_map_t&& new_stockpile, const fixed_point_t new_current_production,
	Good::good_map_t&& new_current_needs
)
	: production_type { new_production_type }, stockpile { std::move(new_stockpile) },
	  current_production { new_current_production }, current_needs { std::move(new_current_needs) } {}

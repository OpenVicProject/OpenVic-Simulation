#pragma once

namespace OpenVic {
	struct ArtisanalProducerDeps;
	struct MarketInstance;
	struct PopsAggregateDeps;

	struct PopDeps {
		ArtisanalProducerDeps const& artisanal_producer_deps;
		MarketInstance& market_instance;
		PopsAggregateDeps const& pops_aggregate_deps;
	};
}
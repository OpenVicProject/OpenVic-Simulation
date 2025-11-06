#pragma once

namespace OpenVic {
	struct ArtisanalProducerDeps;
	struct MarketInstance;

	struct PopDeps {
		ArtisanalProducerDeps const& artisanal_producer_deps;
		MarketInstance& market_instance;
	};
}
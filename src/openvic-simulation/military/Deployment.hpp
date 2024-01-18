#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/military/LeaderTrait.hpp"
#include "openvic-simulation/military/Unit.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct Leader {
	private:
		std::string        PROPERTY(name);
		Unit::type_t       PROPERTY(type);
		Date               PROPERTY(date);
		LeaderTrait const* PROPERTY(personality);
		LeaderTrait const* PROPERTY(background);
		fixed_point_t      PROPERTY(prestige);
		std::string        PROPERTY(picture);

	public:
		Leader(
			std::string_view new_name, Unit::type_t new_type, Date new_date, LeaderTrait const* new_personality,
			LeaderTrait const* new_background, fixed_point_t new_prestige, std::string_view new_picture
		);
	};

	struct Regiment {
	private:
		std::string     PROPERTY(name);
		Unit const*     PROPERTY(type);
		Province const* PROPERTY(home);

	public:
		Regiment(std::string_view new_name, Unit const* new_type, Province const* new_home);
	};

	struct Ship {
	private:
		std::string PROPERTY(name);
		Unit const* PROPERTY(type);

	public:
		Ship(std::string_view new_name, Unit const* new_type);
	};

	struct Army {
	private:
		std::string           PROPERTY(name);
		Province const*       PROPERTY(location);
		std::vector<Regiment> PROPERTY(regiments);

	public:
		Army(std::string_view new_name, Province const* new_location, std::vector<Regiment>&& new_regiments);
	};

	struct Navy {
	private:
		std::string       PROPERTY(name);
		Province const*   PROPERTY(location);
		std::vector<Ship> PROPERTY(ships);

	public:
		Navy(std::string_view new_name, Province const* new_location, std::vector<Ship>&& new_ships);
	};

	struct Deployment : HasIdentifier {
		friend std::unique_ptr<Deployment> std::make_unique<Deployment>(
			std::string_view&&, std::vector<OpenVic::Army>&&, std::vector<OpenVic::Navy>&&, std::vector<OpenVic::Leader>&&
		);

	private:
		std::vector<Army>   PROPERTY(armies);
		std::vector<Navy>   PROPERTY(navies);
		std::vector<Leader> PROPERTY(leaders);

		Deployment(
			std::string_view new_path, std::vector<Army>&& new_armies, std::vector<Navy>&& new_navies,
			std::vector<Leader>&& new_leaders
		);

	public:
		Deployment(Deployment&&) = default;
	};

	struct DeploymentManager {
	private:
		IdentifierInstanceRegistry<Deployment> IDENTIFIER_REGISTRY(deployment);
		string_set_t missing_oob_files;

	public:
		bool add_deployment(
			std::string_view path, std::vector<Army>&& armies, std::vector<Navy>&& navies, std::vector<Leader>&& leaders
		);

		bool load_oob_file(
			GameManager const& game_manager, Dataloader const& dataloader, std::string_view history_path,
			Deployment const*& deployment, bool fail_on_missing
		);
		size_t get_missing_oob_file_count() const;
	};
} // namespace OpenVic

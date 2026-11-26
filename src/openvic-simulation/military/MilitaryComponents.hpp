#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"

namespace OpenVic {
	struct UnitType;
	struct RegimentType;
	struct ShipType;
	struct LeaderTrait;
	struct ProvinceInstance;
	struct CountryInstance;
	struct Pop;

	enum class unit_category_t : uint8_t {
		INVALID_UNIT_CATEGORY,
		INFANTRY,
		CAVALRY,
		SUPPORT,
		SPECIAL,
		BIG_SHIP,
		LIGHT_SHIP,
		TRANSPORT
	};
}

namespace OpenVic::ecs {

	using icon_t = uint32_t;

	struct UnitTypeStats {
		fixed_point_t max_strength;
		fixed_point_t default_organization;
		fixed_point_t max_speed;
		fixed_point_t supply_consumption;
		unit_category_t unit_category;
		bool starts_unlocked;
		Timespan build_time;
	};

	struct UnitTypeCosts {
		memory::vector<std::pair<EntityID, fixed_point_t>> build_cost;
		memory::vector<std::pair<EntityID, fixed_point_t>> supply_cost;
	};	

	struct UnitTypeTerrainModifiers {
		memory::vector<std::pair<EntityID, fixed_point_t>> terrain_modifiers;
	};

	struct UnitTypeDisplay {
		icon_t icon;
		uint32_t priority;
		bool has_floating_flag;
		fixed_point_t weighted_value;

		memory::string sprite;
		memory::string move_sound;
		memory::string select_sound;
	}

	struct RegimentTypeStats {
		regiment_allowed_cultures_t allowed_cultures;
		const fixed_point_t reconnaissance;
		const fixed_point_t attack;
			const fixed_point_t defence;
		const fixed_point_t discipline;
		const fixed_point_t support;
		const fixed_point_t maneuver;
		const fixed_point_t siege;
	};

	struct RegimentTypeDisplay {
		memory::string sprite_override;
		memory::string sprite_mount;
		memory::string sprite_mount_attach_node;
	};

	struct ShipTypeStats {
		bool can_sail;
		bool is_transport;
		bool is_capital;
		bool can_build_overseas;

		fixed_point_t hull;
		fixed_point_t gun_power;
		fixed_point_t fire_range;
		fixed_point_t evasion;
		fixed_point_t torpedo_attack;
		fixed_point_t colonial_points;
		fixed_point_t supply_consumption_score;

		uint32_t min_port_level;
		int32_t limit_per_port;
	};

	struct ShipTypeDisplay {
		icon_t icon;
	};

	struct UnitStats {
		fixed_point_t organization;
		fixed_point_t max_organization;
		fixed_point_t strength;
	};

	struct UnitIdentity {
		memory::string name;
		EntityID unit_type_id;
	};
	
	struct RegimentPopLink {
		EntityID pop = INVALID_ENTITY_ID;
		bool mobilized;
	};

	struct IsNaval {};
	struct IsLand {};

	struct GroupMember {
		EntityID group = INVALID_ENTITY_ID;
	};

	struct GroupIdentity {
		memory::string name;
	};

	struct GroupLocation {
		EntityID province = INVALID_ENTITY_ID; 
	};
	
	struct GroupCountry {
		EntityID country = INVALID_ENTITY_ID;
	};

	struct GroupLeader {
		EntityID leader = INVALID_ENTITY_ID;
	};

	struct GroupStats {
		//use total to make it distinct from something like UnitStats 
		fixed_point_t total_max_organization;
		fixed_point_t total_organization;
		fixed_point_t total_max_strength;
		fixed_point_t total_strength;
	};

#ifdef NOTYET
	struct NavyStats {
		//maybe navies need different stats in the future?
		fixed_point_t total_max_organization;
		fixed_point_t total_organization;
		fixed_point_t total_max_strength;
		fixed_point_t total_strength;
	};

	struct ArmyStats {
		//maybe armies need different stats in the future?
		fixed_point_t total_max_organization;
		fixed_point_t total_organization;
		fixed_point_t total_max_strength;
		fixed_point_t total_strength;
	};
#endif

	struct GroupMovement {
		memory::vector<EntityID> path;
		fixed_point_t movement_progress;
	};


	//used for checking and applying modifiers per terrain performantly
	struct GroupTerrainState {
		bool dirty; 
		fixed_point_t cached_modifier = fixed_point_t::_0;
	};

	struct ArmyExtra {
		EntityID transport_navy = INVALID_ENTITY_ID;
		uint8_t dig_in_level;
		bool is_exiled; 
	};

	struct NavyExtra {
		memory::vector<EntityID> carried_armies;
	};

	struct GroupCombatState {
		bool in_combat;
	};

	struct GroupMovementState {
		bool is_moving;
	};

	struct LeaderIdentity {
		memory::string name;
		uint32_t personality;
		uint32_t background;
		unit_branch_t branch;
		//date
		fixed_point_t prestige;
		memory::string picture;
	};

	struct LeaderCountry {
		EntityID country = INVALID_ENTITY_ID; 		
	};
	
	struct LeaderUsability {
		bool usable;
	};

	//back reference to the leader's army 
	//pre attached at creation so lookups are just plainly read
	//substitute for a longer search algorithm
	struct LeaderGroup {
		EntityID group = INVALID_ENTITY_ID;
	};

	struct UnitDeployment {
		memory::string name;
		province_index_t index;
		regiment_type_index_t type;
	};

	struct WarParticipant {
		EntityID war = INVALID_ENTITY_ID;
		EntityID country = INVALID_ENTITY_ID;
		bool is_attacker;
	};

	struct WarIdentity {
		memory::string name;
	};

	struct WarSideGroups {
		memory::vector<EntityID> attacker_groups;
		memory::vector<EntityID> defender_groups;
	};
	//TODO IMPLEMENT THE METHODS!
	//
	
	bool are_countries_at_war(World const& world, EntityID country_a, EntityID country_b) {
		//STUBBED
		//TODO: AFTER COUNTRY STRUCTS AND STUFF IS DONE, IMPLEMNT

		return country_a != country_b;
	}

	void detect_combat(World& world) {
		memory::unordered_map<uint64_t, WarSideGroups> war_groups = get_war_groups(world);	
		for (auto const& [war_id, sides] : war_groups) {
			EntityID const war = EntityID::from_uint64(war_id);
			for (EntityID attacker_group : sides.attacker_groups) {
				GroupLocation const* attacker_loc = world.get_component<GroupLocation>(attacker_group);
				if (attacker_loc == nullptr)
					continue;

				for (EntityID defender_group : sides.defender_groups) {
					GroupLocation const* defender_loc = world.get_component<GroupLocation>(defender_group);
					if (defender_loc != nullptr && attacker_loc->province == defender_loc->province) {
						if (GroupCombatState* state = world.get_component<GroupCombatState>(attacker_group)) 
							state->in_combat = true;
						if (GroupCombatState* defender_state = world.get_component<GroupCombatState>(defender_group)) {
							defender_state->in_combat = true;
					}
				}	
			}
		}
	}

	memory::vector<EntityID> get_groups_for_country(World const& world, EntityID country) {
		memory::vector<EntityID> groups;
		world.for_each_with_entity<GroupCountry>([&](EntityID group_id, GroupCountry const& gc) {
			if (gc.country == country) {
				groups.push_back(group_id);
			}
		});
		return groups;
	}

	memory::unordered_map<uint64_t, WarSideGroups> get_war_groups(World const& world) {
		memory::unordered_map<uint64_t, WarSideGroups> war_groups;

		world.for_each_with_entity<WarParticipant>([&](EntityID, WarParticipant const& participant) {
			memory::vector<EntityID> country_groups = get_groups_for_country(world, participant.country);
			WarSideGroups& sides = war_groups[participant.war.to_uint64()];

			memory::vector<EntityID>& target = participant.is_attacker ? sides.attacker_groups : sides.defender_groups;
			target.insert(target.end(), country_groups.begin(), country_groups.end());
		});

		return war_groups;
	}
	
	EntityID create_army(CommandBuffer& cmd, memory::string name, EntityID location, EntityID country) {
		return cmd.create_entity(
			GroupIdentity{ std::move(name) },
			GroupLocation{ location },
			GroupCountry{ country },
			GroupLeader{ INVALID_ENTITY_ID },
			GroupStats{},
			GroupMovement{},
			GroupTerrainState{ false },
			ArmyExtra{ INVALID_ENTITY_ID, 0, false },
			GroupCombatState{ false }
		);
	} 

	EntityID create_navy(CommandBuffer& cmd, memory::string name, EntityID location, EntityID country) {
		return cmd.create_entity(
			GroupIdentity{ std::move(name) },
			GroupLocation{ location },
			GroupCountry{ country },
			GroupLeader{ INVALID_ENTITY_ID },
			GroupStats{},
			GroupMovement{},
			GroupTerrainState{ false },
			NavyExtra{},
			GroupCombatState{ false }
		);
	} 

	EntityID create_regiment(CommandBuffer& cmd, World const& world, EntityID unit_type_entity, EntityID army_id, memory::string name) {
		UnitTypeStats const* type_stats = world.get_component<UnitTypeStats>(unit_type_entity);

		return cmd.create_entity(
			UnitIdentity{ std::move(name), unit_type_entity },
			RegimentPopLink{ INVALID_ENTITY_ID, false },
			UnitStats{
				.organization = type_stats->default_organization;
				.max_organization = types_stats->default_organization;
				.strength = types_stats->max_organization;
			},
			GroupMember{ army_id },
			IsLand{}
		);
	}

	EntityID create_ship(CommandBuffer& cmd, World const& world, EntityID unit_type_entity, EntityID ship_id, memory::string name) {
		UnitTypeStats const* type_stats = world.get_component<UnitTypeStats>(unit_type_entity);

		return cmd.create_entity(
			UnitIdentity{ std::move(name), unit_type_entity },
			RegimentPopLink{ INVALID_ENTITY_ID, false },
			UnitStats{
				.organization = type_stats->default_organization;
				.max_organization = types_stats->default_organization;
				.strength = types_stats->max_organization;
			},
			GroupMember{ ship_id },
			IsNaval{}
		);
	}


	EntityID get_leader_army(World const& world, EntityID leader_id) {
		LeaderGroup const* group = world.get_component<LeaderGroup>(leader_id);
		return group != nullptr ? group->group : INVALID_ENTITY_ID;
	}

	void assign_leader(World& world, EntityID leader_id, EntityID army_id) {
		LeaderGroup* leader_group = world.get_component<LeaderGroup>(leader_id);
		if (leader_group == nullptr) {
			return;
		}

		if (leader_group->group.is_valid() && world.is_alive(leader_group->group)) {
			if (GroupLeader* old_group = world.get_component<GroupLeader>(leader_group->group)) {
				old_group->leader = INVALID_ENTITY_ID;
			}
		}

		leader_group->group = army_id;

		if (GroupLeader* new_group = world.get_component<GroupLeader>(army_id)) {
			new_group->leader = leader_id;
		}
	}

	void create_land_leader(CommandBuffer& cmd, World const& world, EntityID country, uint32_t personality, uint32_t background, memory::string name /* maybe creation_date??? */) {
		//TODO: get last names of accepted culture, then assign randomly 
		//TODO: update prestige somehow
		//TODO: do personalities and background generation somehow
		//TODO: do pictures 
		name = "Ben"; //:)
		return cmd.create_entity(
			LeaderIdentity{
				.name = name;
				.prestige = 0;	
				.personality = 0;
				.background = 0;
				.picture = "";
			},
			LeaderCountry{ country },
			LeaderGroup{ INVALID_ENTITY_ID },
			IsLand{}
		);
	}

	void create_naval_leader(CommandBuffer& cmd, World const& world, EntityID country, uint32_t personality, uint32_t background, memory::string name /* maybe creation_date??? */) { //TODO: get last names of accepted culture, then assign randomly 
		//TODO: update prestige somehow
		//TODO: do personalities and background generation somehow
		//TODO: do pictures 
		name = "Ben"; //:)
		return cmd.create_entity(
			LeaderIdentity{
				.name = name;
				.prestige = 0;	
				.personality = 0;
				.background = 0;
				.picture = "";
			},
			LeaderCountry{ country },
			LeaderGroup{ INVALID_ENTITY_ID },
			IsNaval{}
		);
	}

	
	EntityID create_unit_type_entity(World& world, ParsedUnitTypeDef const& def) {
		if (def.is_naval) {
			return world.create_entity(
				def.stats,
				def.costs,
				UnitTypeTerrainModifiers{},
				def.display,
				def.ship_stats,
				def.ship_display,
				IsNaval{}
			);
		}
		return world.create_entity(
			def.stats,
			def.costs,
			UnitTypeTerrainModifiers{},
			def.display,
			def.regiment_stats,
			def.regiment_display,
			IsLand{}
		);
	}

	void apply_terrain_modifiers(World& world, EntityID unit_type_entity, ParsedUnitTypeDef const& def, memory::unordered_map<memory::string, EntityID> const& terrain_name_to_entity) {
		memory::vector<std::pair<EntityID, fixed_point_t>> modifiers;
		for (auto const& [terrain_name, value] : def.terrain_modifiers_by_name) {
			auto it = terrain_name_to_entity.find(terrain_name);
			if (it != terrain_name_to_entity.end()) {
				modifiers.emplace_back(it->second, value);
			}
		}
		set_unit_terrain_modifiers(world, unit_type_entity, std::move(modifiers));
	}

	bool load_unit_types(World& world, ovdl::v2script::Parser const& parser, memory::unordered_map<memory::string, EntityID> const& terrain_name_to_entity) {
		memory::vector<ParsedUnitTypeDef> defs;
		bool ret = parse_unit_type_definitions_file(parser, defs);

		for (ParsedUnitTypeDef const& def : defs) {
			EntityID unit_type_entity = create_unit_type_entity(world, def);
			apply_terrain_modifiers(world, unit_type_entity, def, terrain_name_to_entity);
		}

		return ret;
	}

	void tick_military(World& world) {
		recompute_group_stats(world);
		tick_military_terrain(world);
	}
	
	struct ParsedUnitTypeDef {
		memory::string identifier;
		bool is_naval;
		UnitTypeStats stats;
		UnitTypeCosts costs;
		UnitTypeDisplay display;
		RegimentTypeStats regiment_stats;
		RegimentTypeDisplay regiment_display;
		ShipTypeStats ship_stats;
		ShipTypeDisplay ship_display;
		memory::vector<std::pair<memory::string, fixed_point_t>> terrain_modifiers_by_name;
	};

	struct ParsedTerrainTypeDef {
		memory::string identifier;
	};

	bool parse_terrain_types_file(ovdl::v2script::Parser const& parser, memory::vector<ParsedTerrainTypeDef>& out_defs) {
		return NodeTools::expect_dictionary(
			[&](std::string_view key, ast::NodeCPtr value) -> bool {
				out_defs.push_back(ParsedTerrainTypeDef{ memory::string{ key } });
				return true;
			}
		)(parser.get_file_node());
	}

	bool load_unit_types(World& world, ovdl::v2script::Parser const& parser, memory::unordered_map<memory::string, EntityID> const& terrain_name_to_entity) {
		memory::vector<ParsedUnitTypeDef> defs;
		bool ret = parse_unit_type_definitions_file(parser, defs);

		for (ParsedUnitTypeDef const& def : defs) {
			EntityID unit_type_entity = create_unit_type_entity(world, def);
			apply_terrain_modifiers(world, unit_type_entity, def, terrain_name_to_entity);
		}

		return ret;
	}

	bool load_military_definitions(World& world, ovdl::v2script::Parser const& terrain_parser, ovdl::v2script::Parser const& unit_type_parser) {
		memory::vector<ParsedTerrainTypeDef> terrain_defs;
		bool ret = parse_terrain_types_file(terrain_parser, terrain_defs);

		memory::unordered_map<memory::string, EntityID> terrain_name_to_entity;
		for (ParsedTerrainTypeDef const& def : terrain_defs) {
			EntityID id = world.create_entity(TerrainIdentity{ def.identifier });
			terrain_name_to_entity.emplace(def.identifier, id);
		}

		ret &= load_unit_types(world, unit_type_parser, terrain_name_to_entity);
		return ret;
	}

	void run_simulation_day(World& world, Date today) {
		world.tick_systems(today);
		recompute_group_stats(world);
		recompute_terrain_modifiers(world);
	}

	//so long of a function here
	bool parse_unit_type_definitions_file(ovdl::v2script::Parser const& parser, memory::vector<ParsedUnitTypeDef>& out_defs) {
		using namespace std::string_view_literals;
		auto type_symbol = parser.find_intern("type"sv);

		if (!type_symbol) 
			return false;	

		return NodeTools::expect_dictionary([&](std::string_view key, ast::NodeCPtr value) -> bool {
			unit_branch_t branch = unit_branch_t::INVALID_BRANCH;

			bool ret = NodeTools::expect_key(type_symbol, UnitTypeManager::expect_branch_identifier(NodeTools::assign_variable_callback(branch)))(value);

			if (branch != unit_branch_t::LAND && branch != unit_branch_t::NAVAL) 
				return false;

			ParsedUnitTypeDef def{};
			def.identifier = memory::string{ key };
			def.is_naval = branch == unit_branch_t::NAVAL;

			NodeTools::key_map_t key_map{};
			ret &= NodeTools::add_key_map_entries(key_map,
				"icon", ONE_EXACTLY, NodeTools::expect_uint(NodeTools::assign_variable_callback(def.display.icon)),
				"type", ONE_EXACTLY, NodeTools::success_callback,
				"active", ZERO_OR_ONE, NodeTools::expect_bool(NodeTools::assign_variable_callback(def.stats.starts_unlocked)),
				"floating_flag", ONE_EXACTLY, NodeTools::expect_bool(NodeTools::assign_variable_callback(def.display.has_floating_flag)),
				"priority", ONE_EXACTLY, NodeTools::expect_uint(NodeTools::assign_variable_callback(def.display.priority)),
				"max_strength", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.stats.max_strength)),
				"default_organisation", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.stats.default_organization)),
				"maximum_speed", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.stats.max_speed)),
				"weighted_value", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.display.weighted_value)),
				"build_time", ONE_EXACTLY, NodeTools::expect_days(NodeTools::assign_variable_callback(def.stats.build_time)),
				"supply_consumption", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.stats.supply_consumption))
			);

			auto add_terrain_modifier = [&def](std::string_view terrain_key, ast::NodeCPtr terrain_value) -> bool {
				return NodeTools::expect_fixed_point(
					[&def, terrain_key](fixed_point_t modifier) -> bool {
						def.terrain_modifiers_by_name.emplace_back(memory::string{ terrain_key }, modifier);
						return true;
					}
				)(terrain_value);
			};

			if (def.is_naval) {
				ret &= NodeTools::add_key_map_entries(key_map,
					"naval_icon", ONE_EXACTLY, NodeTools::expect_uint(NodeTools::assign_variable_callback(def.ship_display.icon)),
					"sail", ZERO_OR_ONE, NodeTools::expect_bool(NodeTools::assign_variable_callback(def.ship_stats.can_sail)),
					"transport", ZERO_OR_ONE, NodeTools::expect_bool(NodeTools::assign_variable_callback(def.ship_stats.is_transport)),
					"capital", ZERO_OR_ONE, NodeTools::expect_bool(NodeTools::assign_variable_callback(def.ship_stats.is_capital)),
					"colonial_points", ZERO_OR_ONE, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.ship_stats.colonial_points)),
					"can_build_overseas", ZERO_OR_ONE, NodeTools::expect_bool(NodeTools::assign_variable_callback(def.ship_stats.can_build_overseas)),
					"min_port_level", ONE_EXACTLY, NodeTools::expect_uint(NodeTools::assign_variable_callback(def.ship_stats.min_port_level)),
					"limit_per_port", ONE_EXACTLY, NodeTools::expect_int(NodeTools::assign_variable_callback(def.ship_stats.limit_per_port)),
					"supply_consumption_score", ZERO_OR_ONE, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.ship_stats.supply_consumption_score)),
					"hull", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.ship_stats.hull)),
					"gun_power", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.ship_stats.gun_power)),
					"fire_range", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.ship_stats.fire_range)),
					"evasion", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.ship_stats.evasion)),
					"torpedo_attack", ZERO_OR_ONE, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.ship_stats.torpedo_attack))
				);
			} else {
				ret &= NodeTools::add_key_map_entries(key_map,
					"sprite_override", ZERO_OR_ONE, NodeTools::expect_identifier(NodeTools::assign_variable_callback_string(def.regiment_display.sprite_override)),
					"sprite_mount", ZERO_OR_ONE, NodeTools::expect_identifier(NodeTools::assign_variable_callback_string(def.regiment_display.sprite_mount)),
					"sprite_mount_attach_node", ZERO_OR_ONE, NodeTools::expect_identifier(NodeTools::assign_variable_callback_string(def.regiment_display.sprite_mount_attach_node)),
					"reconnaissance", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.regiment_stats.reconnaissance)),
					"attack", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.regiment_stats.attack)),
					"defence", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.regiment_stats.defence)),
					"discipline", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.regiment_stats.discipline)),
					"support", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.regiment_stats.support)),
					"maneuver", ONE_EXACTLY, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.regiment_stats.maneuver)),
					"siege", ZERO_OR_ONE, NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(def.regiment_stats.siege))
				);
			}

			ret &= NodeTools::expect_dictionary_key_map_and_default(key_map, add_terrain_modifier)(value);

			out_defs.push_back(std::move(def));
			return ret;
		}
	(parser.get_file_node());
}
	
	struct GroupStatsAccum {
                fixed_point_t organization = fixed_point_t::_0;
                fixed_point_t max_organization = fixed_point_t::_0;
                fixed_point_t strength = fixed_point_t::_0;
                fixed_point_t max_strength = fixed_point_t::_0;
        };

        void recompute_group_stats(World& world) {
                std::unordered_map<uint64_t, GroupStatsAccum> totals;

                world.for_each_chunk<UnitStats, UnitIdentity, GroupMember>(
                        [&](ChunkView<UnitStats, UnitIdentity, GroupMember> view) {
                                UnitStats const* OV_RESTRICT stats = view.array<UnitStats>();
                                UnitIdentity const* OV_RESTRICT identity = view.array<UnitIdentity>();
                                GroupMember const* OV_RESTRICT members = view.array<GroupMember>();

                                for (std::size_t i = 0; i < view.count(); ++i) {
                                        GroupStatsAccum& accum = totals[members[i].group.to_uint64()];
                                        accum.organization += stats[i].organization;
                                        accum.max_organization += stats[i].max_organization;
                                        accum.strength += stats[i].strength;

                                        if (UnitTypeStats const* type_stats = world.get_component<UnitTypeStats>(identity[i].unit_type)) {
                                                accum.max_strength += type_stats->max_strength;
                                        }
                                }
                        }
                );

                world.for_each_with_entity<GroupStats>([&](EntityID id, GroupStats& stats) {
                        auto const it = totals.find(id.to_uint64());
                        GroupStatsAccum const accum = it != totals.end() ? it->second : GroupStatsAccum{};
                        stats.total_organization = accum.organization;
                        stats.total_max_organization = accum.max_organization;
                        stats.total_strength = accum.strength;
                        stats.total_max_strength = accum.max_strength;
                });
        }

	void on_group_arrives(World& world, EntityID group_id, EntityID new_province) { 
		if (GroupTerrainState* state = world.get_component<GroupTerrainState>(group_id))
			state->dirty = true;
	}
	        
        inline fixed_point_t get_unit_terrain_modifier(World const& world, EntityID unit_type, EntityID terrain) {
                UnitTypeTerrainModifiers const* modifiers = world.get_component<UnitTypeTerrainModifiers>(unit_type);
                if (modifiers == nullptr) {
                	return fixed_point_t::_0;
                }

                for (std::pair<EntityID, fixed_point_t> const& entry : modifiers->terrain_modifiers) {
                        if (entry.first == terrain) {
                                return entry.second;
                        }
                }

                return fixed_point_t::_0;
        }

        inline fixed_point_t get_regiment_terrain_modifier(World const& world, EntityID regiment, EntityID terrain) {
                UnitIdentity const* identity = world.get_component<UnitIdentity>(regiment);
                if (identity == nullptr) {
                        return fixed_point_t::_0;
                }

                return get_unit_terrain_modifier(world, identity->unit_type, terrain);
        }

	void recompute_terrain_modifiers(World& world) {
		std::unordered_map<uint64_t, fixed_point_t> totals;

		world.for_each_chunk<UnitIdentity, GroupMember>([&](ChunkView<UnitIdentity, GroupMember> view) {
			UnitIdentity const* OV_RESTRICT identity = view.array<UnitIdentity>();
			GroupMember const* OV_RESTRICT members = view.array<GroupMember>();

			for (std::size_t i = 0; i < view.count(); ++i) {
				GroupLocation const* loc = world.get_component<GroupLocation>(members[i].group);
				if (loc == nullptr) {
					continue;
				}
				totals[members[i].group.to_uint64()] += get_unit_terrain_modifier(world, identity[i].unit_type_id, loc->province);
			}
		});

		world.for_each_with_entity<GroupTerrainState>([&](EntityID group_id, GroupTerrainState& state) {
			if (!state.dirty) {
				return;
			}
			auto it = totals.find(group_id.to_uint64());
			state.cached_modifier = it != totals.end() ? it->second : fixed_point_t::_0;
			state.dirty = false;
		});
	}

	//ecs_checksums
	inline uint64_t ecs_checksum(UnitIdentity const& value, uint64_t seed) {
        	uint64_t h = fold_uint64(value.name.size(), seed);
        	h = fnv1a_64_bytes(value.name.data(), value.name.size(), h);
       		return fold_uint64(value.unit_type.to_uint64(), h);
	}	

	inline uint64_t ecs_checksum(RegimentPopLink const& value, uint64_t seed) {
		uint64_t h = fold_uint64(value.pop.to_uint64(), seed);
		return fold_uint64(value.mobilized ? 1u : 0u, h);
	}

	inline uint64_t ecs_checksum(GroupIdentity const& value, uint64_t seed) {
		uint64_t h = fold_uint64(value.name.size(), seed);
		return fnv1a_64_bytes(value.name.data(), value.name.size(), h);
	}

	inline uint64_t ecs_checksum(GroupMovement const& value, uint64_t seed) {
		uint64_t h = fold_uint64(value.path.size(), seed);
		for (EntityID const& eid : value.path) {
			h = fold_uint64(eid.to_uint64(), h);
		}
		return checksum_value(value.movement_progress, h);
	}

	inline uint64_t ecs_checksum(ArmyExtra const& value, uint64_t seed) {
		uint64_t h = fold_uint64(value.transport_navy.to_uint64(), seed);
		h = fold_uint64(value.dig_in_level, h);
		return fold_uint64(value.is_exiled ? 1u : 0u, h);
	}

	inline uint64_t ecs_checksum(NavyExtra const& value, uint64_t seed) {
		uint64_t h = fold_uint64(value.carried_armies.size(), seed);
		for (EntityID const& eid : value.carried_armies) {
			h = fold_uint64(eid.to_uint64(), h);
		}
		return h;
	}
	
	inline uint64_t ecs_checksum(LeaderIdentity const& value, uint64_t seed) {
		uint64_t h = fold_uint64(value.name.size(), seed);
		h = fnv1a_64_bytes(value.name.data(), value.name.size(), h);
		h = fold_uint64(value.personality, h);
		h = fold_uint64(value.background, h);
		h = checksum_value(value.prestige, h);
		h = fold_uint64(value.picture.size(), h);
		return fnv1a_64_bytes(value.picture.data(), value.picture.size(), h);
	}
}

ECS_COMPONENT(OpenVic::ecs::UnitStats, "OpenVic::UnitStats")
ECS_COMPONENT(OpenVic::ecs::UnitIdentity, "OpenVic::UnitIdentity")
ECS_COMPONENT(OpenVic::ecs::RegimentPopLink, "OpenVic::RegimentPopLink")
ECS_COMPONENT(OpenVic::ecs::IsNaval, "OpenVic::IsNaval")
ECS_COMPONENT(OpenVic::ecs::IsLand, "OpenVic::IsLand")
ECS_COMPONENT(OpenVic::ecs::GroupMember, "OpenVic::GroupMember")
ECS_COMPONENT(OpenVic::ecs::GroupIdentity, "OpenVic::GroupIdentity")
ECS_COMPONENT(OpenVic::ecs::GroupLocation, "OpenVic::GroupLocation")
ECS_COMPONENT(OpenVic::ecs::GroupCountry, "OpenVic::GroupCountry")
ECS_COMPONENT(OpenVic::ecs::GroupLeader, "OpenVic::GroupLeader")
ECS_COMPONENT(OpenVic::ecs::GroupStats, "OpenVic::GroupStats")
ECS_COMPONENT(OpenVic::ecs::GroupMovement, "OpenVic::GroupMovement")
ECS_COMPONENT(OpenVic::ecs::GroupTerrainState, "OpenVic::GroupTerrainState")
ECS_COMPONENT(OpenVic::ecs::ArmyExtra, "OpenVic::ArmyExtra")
ECS_COMPONENT(OpenVic::ecs::NavyExtra, "OpenVic::NavyExtra")
ECS_COMPONENT(OpenVic::ecs::InCombat, "OpenVic::InCombat")
ECS_COMPONENT(OpenVic::ecs::Moving, "OpenVic::Moving")
ECS_COMPONENT(OpenVic::ecs::LeaderIdentity, "OpenVic::LeaderIdentity")
ECS_COMPONENT(OpenVic::ecs::LeaderCountry, "OpenVic::LeaderCountry")
ECS_COMPONENT(OpenVic::ecs::LeaderUsability, "OpenVic::LeaderUsability")
ECS_COMPONENT(OpenVic::ecs::LeaderGroup, "OpenVic::LeaderGroup")
ECS_COMPONENT(OpenVic::ecs::UnitDeployment, "OpenVic::UnitDeployment")

static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::UnitStats>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::UnitIdentity>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::RegimentPopLink>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::GroupMember>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::GroupIdentity>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::GroupLocation>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::GroupCountry>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::GroupLeader>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::GroupStats>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::GroupMovement>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::GroupTerrainState>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::ArmyExtra>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::NavyExtra>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::LeaderIdentity>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::LeaderCountry>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::LeaderUsability>);
static_assert(OpenVic::ecs::is_checksummable_v<OpenVic::ecs::LeaderGroup>);


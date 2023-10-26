#pragma once

#include <map>
#include <vector>

#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/economy/Building.hpp"
#include "openvic-simulation/history/Bookmark.hpp"

namespace OpenVic {
    struct ProvinceHistoryManager;

    struct ProvinceHistory {
        friend struct ProvinceHistoryManager;

    private:
        Country const* owner;
        Country const* controller;
        std::vector<Country const*> cores;
        Good const* rgo;
        uint8_t life_rating;
        TerrainType const* terrain_type;
        std::map<Building const*, uint8_t> buildings;
        std::map<Ideology const*, uint8_t> party_loyalties;

        ProvinceHistory(
            Country const* new_owner,
            Country const* new_controller,
            std::vector<Country const*>&& new_cores,
            Good const* new_rgo,
            uint8_t new_life_rating,
            TerrainType const* new_terrain_type,
            std::map<Building const*, uint8_t>&& new_buildings,
            std::map<Ideology const*, uint8_t>&& new_party_loyalties
        );

    public:
        Country const* get_owner() const;
        Country const* get_controller() const;
        const std::vector<Country const*>& get_cores() const;
        bool is_core_of(Country const* country) const;
        Good const* get_rgo() const;
        uint8_t get_life_rating() const;
        TerrainType const* get_terrain_type() const;
        const std::map<Building const*, uint8_t>& get_buildings() const;
        const std::map<Ideology const*, uint8_t>& get_party_loyalties() const;
    };

    struct ProvinceHistoryManager {
    private:
        std::map<Province const*, std::map<Date, ProvinceHistory>> province_histories;
        bool locked = false;

        inline bool _load_province_history_entry(GameManager& game_manager, std::string_view province, Date const& date, ast::NodeCPtr root);
    
    public:
        ProvinceHistoryManager() {}

        bool add_province_history_entry(
            Province const* province,
            Date date,
            Country const* owner,
            Country const* controller,
            std::vector<Country const*>&& cores,
            Good const* rgo,
            uint8_t life_rating,
            TerrainType const* terrain_type,
            std::map<Building const*, uint8_t>&& buildings,
            std::map<Ideology const*, uint8_t>&& party_loyalties,
            bool updated_cores,
            bool updated_buildings,
            bool updated_loyalties
        );

        void lock_province_histories();
        bool is_locked() const;

        /* Returns history of province at date, if date doesn't have an entry returns closest entry before date. Return can be nullptr if an error occurs. */
        ProvinceHistory const* get_province_history(Province const* province, Date entry) const;
        /* Returns history of province at bookmark date. Return can be nullptr if an error occurs. */
        inline ProvinceHistory const* get_province_history(Province const* province, Bookmark const* bookmark) const;

        bool load_province_history_file(GameManager& game_manager, std::string_view name, ast::NodeCPtr root);
    };
} // namespace OpenVic

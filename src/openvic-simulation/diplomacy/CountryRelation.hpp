#pragma once

#include <compare>
#include <tuple>

#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	struct CountryInstance;

	struct CountryRelationInstanceProxy {
		std::string_view country_id;

		CountryRelationInstanceProxy(std::string_view id);
		CountryRelationInstanceProxy(CountryInstance const* country);

		operator std::string_view() const;
	};

	struct CountryRelationPair : std::pair<std::string_view, std::string_view> {
		using base_type = std::pair<std::string_view, std::string_view>;
		using base_type::base_type;

		inline constexpr auto operator<=>(CountryRelationPair const& rhs) const {
			using ordering = decltype(OpenVic::utility::three_way(std::tie(first, second), std::tie(rhs.first, rhs.second)));

			auto tied = std::tie(first, second);
			if (tied == std::tie(rhs.first, rhs.second)) {
				return ordering::equivalent;
			} else if (tied == std::tie(rhs.second, rhs.first)) {
				return ordering::equivalent;
			}

			auto& [lhs_left, lhs_right] = *this;
			auto& [rhs_left, rhs_right] = rhs;

			auto& lhs_comp_first = lhs_left > lhs_right ? lhs_left : lhs_right;
			auto& lhs_comp_second = lhs_left < lhs_right ? lhs_left : lhs_right;

			auto& rhs_comp_first = rhs_left > rhs_right ? rhs_left : rhs_right;
			auto& rhs_comp_second = rhs_left < rhs_right ? rhs_left : rhs_right;

			return OpenVic::utility::three_way(
				std::tie(lhs_comp_first, lhs_comp_second), std::tie(rhs_comp_first, rhs_comp_second)
			);
		}

		inline constexpr bool operator==(CountryRelationPair const& rhs) const {
			auto tied = std::tie(first, second);
			return tied == std::tie(rhs.first, rhs.second) || tied == std::tie(rhs.second, rhs.first);
		}
	};
}

namespace std {
	template<>
	struct hash<OpenVic::CountryRelationPair> {
		size_t operator()(OpenVic::CountryRelationPair const& pair) const {
			std::size_t seed1(0);
			OpenVic::utility::hash_combine(seed1, pair.first);
			OpenVic::utility::hash_combine(seed1, pair.second);

			std::size_t seed2(0);
			OpenVic::utility::hash_combine(seed2, pair.second);
			OpenVic::utility::hash_combine(seed2, pair.first);

			return std::min(seed1, seed2);
		}
	};
}

namespace OpenVic {
	using country_relation_value_t = int16_t;

	struct CountryRelationManager {
	private:
		// TODO: reference of manager responsible for storing CountryInstances
		ordered_map<CountryRelationPair, country_relation_value_t> country_relations;

	public:
		CountryRelationManager(/* TODO: Country Instance Manager Reference */);

		bool add_country(CountryRelationInstanceProxy country);
		bool remove_country(CountryRelationInstanceProxy country);

		country_relation_value_t
		get_country_relation(CountryRelationInstanceProxy country, CountryRelationInstanceProxy recepient) const;
		country_relation_value_t*
		get_country_relation_ptr(CountryRelationInstanceProxy country, CountryRelationInstanceProxy recepient);
		bool set_country_relation(
			CountryRelationInstanceProxy country, CountryRelationInstanceProxy recepient, country_relation_value_t value
		);
	};
}

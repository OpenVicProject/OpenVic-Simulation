#pragma once

#include <algorithm>
#include <bit>
#include <compare>
#include <cstdint>
#include <tuple>
#include <type_traits>

#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	struct CountryInstance;

	using CountryInstancePair = std::pair<CountryInstance const*, CountryInstance const*>;

	struct UnorderedCountryInstancePair : CountryInstancePair {
		using base_type = CountryInstancePair;
		using base_type::base_type;

		inline constexpr auto operator<=>(UnorderedCountryInstancePair const& rhs) const {
			const uintptr_t left1 = std::bit_cast<uintptr_t>(first), //
				left2 = std::bit_cast<uintptr_t>(second), //
				right1 = std::bit_cast<uintptr_t>(rhs.first), //
				right2 = std::bit_cast<uintptr_t>(rhs.second);

			using ordering = decltype(OpenVic::utility::three_way(std::tie(left1, left2), std::tie(right1, right2)));

			auto tied = std::tie(left1, left2);
			if (tied == std::tie(right1, right2)) {
				return ordering::equivalent;
			} else if (tied == std::tie(right2, right1)) {
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

		inline constexpr bool operator==(UnorderedCountryInstancePair const& rhs) const {
			const uintptr_t left1 = std::bit_cast<uintptr_t>(first), //
				left2 = std::bit_cast<uintptr_t>(second), //
				right1 = std::bit_cast<uintptr_t>(rhs.first), //
				right2 = std::bit_cast<uintptr_t>(rhs.second);

			auto tied = std::tie(left1, left2);
			return tied == std::tie(right1, right2) || tied == std::tie(right2, right1);
		}
	};
}

namespace std {
	template<>
	struct hash<OpenVic::CountryInstancePair> {
		size_t operator()(OpenVic::CountryInstancePair const& pair) const {
			const uintptr_t first = std::bit_cast<uintptr_t>(pair.first), //
				second = std::bit_cast<uintptr_t>(pair.second);

			std::size_t seed1(0);
			OpenVic::utility::hash_combine(seed1, first);
			OpenVic::utility::hash_combine(seed1, second);

			return seed1;
		}
	};

	template<>
	struct hash<OpenVic::UnorderedCountryInstancePair> {
		size_t operator()(OpenVic::UnorderedCountryInstancePair const& pair) const {
			const uintptr_t first = std::bit_cast<uintptr_t>(pair.first), //
				second = std::bit_cast<uintptr_t>(pair.second);

			std::size_t seed1(0);
			OpenVic::utility::hash_combine(seed1, first);
			OpenVic::utility::hash_combine(seed1, second);

			std::size_t seed2(0);
			OpenVic::utility::hash_combine(seed2, second);
			OpenVic::utility::hash_combine(seed2, first);

			return std::min(seed1, seed2);
		}
	};
}

namespace OpenVic {
	struct CountryRelationManager {
		using relation_value_type = int16_t;
		class influence_value_type {
			static constexpr uint8_t MAX_VALUE = 100;
			uint8_t value;

		public:
			constexpr influence_value_type() = default;
			constexpr influence_value_type(uint8_t value) : value(std::min(value, MAX_VALUE)) {}

#define BIN_OPERATOR(OP) \
	template<typename T> \
	friend inline constexpr influence_value_type operator OP(influence_value_type const& lhs, T const& rhs) { \
		return std::min<uint8_t>(lhs.value OP rhs, MAX_VALUE); \
	} \
	template<typename T> \
	friend inline constexpr influence_value_type operator OP(T const& lhs, influence_value_type const& rhs) { \
		return std::min<uint8_t>(lhs OP rhs.value, MAX_VALUE); \
	}

			BIN_OPERATOR(+);
			BIN_OPERATOR(-);
			BIN_OPERATOR(*);
			BIN_OPERATOR(/);
			BIN_OPERATOR(%);

#undef BIN_OPERATOR

			template<typename T>
			friend inline constexpr bool operator==(influence_value_type const& lhs, T const& rhs) {
				return lhs.value == rhs;
			}
			template<typename T>
			friend inline constexpr auto operator<=>(influence_value_type const& lhs, T const& rhs) {
				return lhs.value <=> rhs;
			}


#define BIN_OPERATOR_EQ(OP) \
	template<typename T> \
	friend inline constexpr influence_value_type& operator OP##=(influence_value_type & lhs, T const& rhs) { \
		lhs.value = std::min<uint8_t>(lhs.value OP rhs, MAX_VALUE); \
		return lhs; \
	} \
	friend inline constexpr influence_value_type& operator OP##=( \
		influence_value_type & lhs, influence_value_type const& rhs \
	) { \
		return lhs OP## = rhs.value; \
	}

			BIN_OPERATOR_EQ(+);
			BIN_OPERATOR_EQ(-);
			BIN_OPERATOR_EQ(*);
			BIN_OPERATOR_EQ(/);
			BIN_OPERATOR_EQ(%);

#undef BIN_OPERATOR_EQ
		};

		enum class OpinionType : int8_t {
			Hostile = -2, //
			Opposed,
			Neutral,
			Cordial,
			Friendly,
			Sphere
		};

	private:
#define RELATION_PAIR_MAP(PAIR_TYPE, VALUE_TYPE, NAME, FUNC_NAME, RECEPIENT_CONST) \
	deque_ordered_map<PAIR_TYPE, VALUE_TYPE> NAME; \
\
public: \
	VALUE_TYPE get_##FUNC_NAME(CountryInstance const* country, CountryInstance const* recepient) const; \
	VALUE_TYPE& assign_or_get_##FUNC_NAME( \
		CountryInstance* country, CountryInstance RECEPIENT_CONST* recepient, VALUE_TYPE default_value = {} \
	); \
	bool set_##FUNC_NAME(CountryInstance* country, CountryInstance RECEPIENT_CONST* recepient, VALUE_TYPE value); \
	decltype(NAME)::values_container_type const& get_##FUNC_NAME##_values() const; \
\
private:

		RELATION_PAIR_MAP(UnorderedCountryInstancePair, relation_value_type, relations, country_relation, );
		RELATION_PAIR_MAP(UnorderedCountryInstancePair, bool, alliances, country_alliance, );
		RELATION_PAIR_MAP(UnorderedCountryInstancePair, bool, at_war, country_at_war, );

		RELATION_PAIR_MAP(CountryInstancePair, bool, military_access, country_military_access, const);
		RELATION_PAIR_MAP(CountryInstancePair, bool, war_subsidies, country_war_subsidies, const);
		RELATION_PAIR_MAP(CountryInstancePair, bool, command_units, country_command_units, const);
		RELATION_PAIR_MAP(CountryInstancePair, bool, vision, country_vision, const);
		RELATION_PAIR_MAP(CountryInstancePair, OpinionType, opinions, country_opinion, const);
		RELATION_PAIR_MAP(CountryInstancePair, influence_value_type, influence, country_influence, const);
		RELATION_PAIR_MAP(CountryInstancePair, bool, discredits, country_discredit, const);
		RELATION_PAIR_MAP(CountryInstancePair, bool, embassy_bans, country_embassy_ban, const);

#undef RELATION_PAIR_MAP
	};
}

constexpr OpenVic::CountryRelationManager::OpinionType& operator++(OpenVic::CountryRelationManager::OpinionType& type) {
	using underlying_type = std::underlying_type_t<OpenVic::CountryRelationManager::OpinionType>;
	type = static_cast<OpenVic::CountryRelationManager::OpinionType>(static_cast<underlying_type>(type) + 1);
	type = static_cast<underlying_type>(type) >
			static_cast<underlying_type>(OpenVic::CountryRelationManager::OpinionType::Sphere) //
		? OpenVic::CountryRelationManager::OpinionType::Sphere
		: type;
	return type;
}

constexpr OpenVic::CountryRelationManager::OpinionType& operator++(OpenVic::CountryRelationManager::OpinionType& type, int) {
	OpenVic::CountryRelationManager::OpinionType result = type;
	++type;
	return type;
}

constexpr OpenVic::CountryRelationManager::OpinionType& operator--(OpenVic::CountryRelationManager::OpinionType& type) {
	using underlying_type = std::underlying_type_t<OpenVic::CountryRelationManager::OpinionType>;
	type = static_cast<OpenVic::CountryRelationManager::OpinionType>(static_cast<underlying_type>(type) - 1);
	type = static_cast<underlying_type>(type) <
			static_cast<underlying_type>(OpenVic::CountryRelationManager::OpinionType::Hostile) //
		? OpenVic::CountryRelationManager::OpinionType::Sphere
		: type;
	return type;
}

constexpr OpenVic::CountryRelationManager::OpinionType& operator--(OpenVic::CountryRelationManager::OpinionType& type, int) {
	OpenVic::CountryRelationManager::OpinionType result = type;
	--type;
	return type;
}

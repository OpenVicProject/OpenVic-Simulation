#pragma once

#include <algorithm>
#include <bit>
#include <compare>
#include <cstdint>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "openvic-simulation/types/Date.hpp"
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
			static constexpr uint16_t MAX_VALUE = 100; // TODO: implement defines.diplomacy.MAX_INFLUENCE
			uint16_t value;

		public:
			constexpr influence_value_type() {};
			constexpr influence_value_type(uint16_t value) : value(std::min(value, MAX_VALUE)) {}

#define BIN_OPERATOR(OP) \
	template<typename T> \
	friend inline constexpr influence_value_type operator OP(influence_value_type const& lhs, T const& rhs) { \
		return std::min<uint16_t>(lhs.value OP rhs, MAX_VALUE); \
	} \
	template<typename T> \
	friend inline constexpr influence_value_type operator OP(T const& lhs, influence_value_type const& rhs) { \
		return std::min<uint16_t>(lhs OP rhs.value, MAX_VALUE); \
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
		lhs.value = std::min<uint16_t>(lhs.value OP rhs, MAX_VALUE); \
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

			explicit constexpr operator decltype(value)() const {
				return value;
			}
		};

		enum class OpinionType : int8_t {
			Hostile = -2, //
			Opposed,
			Neutral,
			Cordial,
			Friendly,
			Sphere
		};

		using influence_priority_value_type = uint8_t;

	private:
#define RELATION_PAIR_MAP(PAIR_TYPE, VALUE_TYPE, NAME, FUNC_NAME, RECIPIENT_CONST) \
	vector_ordered_map<PAIR_TYPE, VALUE_TYPE> NAME; \
\
public: \
	VALUE_TYPE get_##FUNC_NAME(CountryInstance const* country, CountryInstance const* recipient) const; \
	VALUE_TYPE& assign_or_get_##FUNC_NAME( \
		CountryInstance* country, CountryInstance RECIPIENT_CONST* recipient, VALUE_TYPE default_value = {} \
	); \
	bool set_##FUNC_NAME(CountryInstance* country, CountryInstance RECIPIENT_CONST* recipient, VALUE_TYPE value); \
	decltype(NAME)::values_container_type const& get_##FUNC_NAME##_values() const; \
\
private:

		RELATION_PAIR_MAP(UnorderedCountryInstancePair, relation_value_type, relations, country_relation, );
		RELATION_PAIR_MAP(UnorderedCountryInstancePair, bool, alliances, country_alliance, );
		RELATION_PAIR_MAP(UnorderedCountryInstancePair, bool, at_war, at_war_with, );

		RELATION_PAIR_MAP(CountryInstancePair, bool, military_access, has_military_access_to, const);
		RELATION_PAIR_MAP(CountryInstancePair, bool, war_subsidies, war_subsidies_to, const);
		RELATION_PAIR_MAP(CountryInstancePair, bool, command_units, commands_units, const);
		RELATION_PAIR_MAP(CountryInstancePair, bool, vision, has_vision, const);
		RELATION_PAIR_MAP(CountryInstancePair, OpinionType, opinions, country_opinion, const);
		RELATION_PAIR_MAP(CountryInstancePair, influence_value_type, influence, influence_with, const);
		RELATION_PAIR_MAP(CountryInstancePair, influence_priority_value_type, influence_priority, influence_priority_with, const);

		vector_ordered_map<CountryInstancePair, Date> discredits;
		vector_ordered_map<CountryInstancePair, Date> embassy_bans;

	public:
		std::optional<Date> get_discredited_date(CountryInstance const* country, CountryInstance const* recipient) const;
		Date& assign_or_get_discredited_date(CountryInstance* country, CountryInstance const* recipient, Date default_value);
		bool set_discredited_date(CountryInstance* country, CountryInstance const* recipient, Date value);
		decltype(discredits.values_container()) get_discredited_date_values() const;

		std::optional<Date> get_embassy_banned_date(CountryInstance const* country, CountryInstance const* recipient) const;
		Date& assign_or_get_embassy_banned_date( //
			CountryInstance* country, CountryInstance const* recipient, Date default_value
		);
		bool set_embassy_banned_date(CountryInstance* country, CountryInstance const* recipient, Date value);
		decltype(embassy_bans.values_container()) get_embassy_banned_date_values() const;

#undef RELATION_PAIR_MAP

		static constexpr std::string_view get_opinion_identifier(OpinionType type) {
			switch (type) {
				using namespace std::string_view_literals;
				using enum OpinionType;
			case Hostile:  return "REL_HOSTILE"sv;
			case Opposed:  return "REL_OPPOSED"sv;
			case Neutral:  return "REL_NEUTRAL"sv;
			case Cordial:  return "REL_CORDIAL"sv;
			case Friendly: return "REL_FRIENDLY"sv;
			case Sphere:   return "REL_SPHERE_OF_INFLUENCE"sv;
			}
		}
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

constexpr OpenVic::CountryRelationManager::OpinionType operator++(OpenVic::CountryRelationManager::OpinionType& type, int) {
	OpenVic::CountryRelationManager::OpinionType result = type;
	++type;
	return result;
}

constexpr OpenVic::CountryRelationManager::OpinionType& operator--(OpenVic::CountryRelationManager::OpinionType& type) {
	using underlying_type = std::underlying_type_t<OpenVic::CountryRelationManager::OpinionType>;
	type = static_cast<OpenVic::CountryRelationManager::OpinionType>(static_cast<underlying_type>(type) - 1);
	type = static_cast<underlying_type>(type) <
			static_cast<underlying_type>(OpenVic::CountryRelationManager::OpinionType::Hostile) //
		? OpenVic::CountryRelationManager::OpinionType::Hostile
		: type;
	return type;
}

constexpr OpenVic::CountryRelationManager::OpinionType operator--(OpenVic::CountryRelationManager::OpinionType& type, int) {
	OpenVic::CountryRelationManager::OpinionType result = type;
	--type;
	return result;
}

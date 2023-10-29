#pragma once

#include <concepts>
#include <memory>

#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct DefineManager;

	struct Define : HasIdentifier {
		friend struct DefineManager;

		enum class Type : unsigned char { None, Country, Economy, Military, Diplomacy, Pops, Ai, Graphics };

	private:
		std::string HASID_PROPERTY(value);
		Type HASID_PROPERTY(type);

		Define(std::string_view new_identifier, std::string&& new_value, Type new_type);

	public:
		Define(Define&&) = default;

		fixed_point_t get_value_as_fp() const;
		int64_t get_value_as_int() const;
		uint64_t get_value_as_uint() const;
	};

	struct DefineManager {
	private:
		IdentifierRegistry<Define> defines;

		std::unique_ptr<Date> start_date = nullptr;
		std::unique_ptr<Date> end_date = nullptr;

	public:
		DefineManager();

		bool add_define(std::string_view name, std::string&& value, Define::Type type);
		bool add_date_define(std::string_view name, Date date);
		IDENTIFIER_REGISTRY_ACCESSORS(define);

		const Date& get_start_date() const;
		const Date& get_end_date() const;

		bool load_defines_file(ast::NodeCPtr root);
	};
}

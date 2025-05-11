#pragma once

#include <optional>
#include <span>
#include <string_view>

#include <fmt/color.h>
#include <fmt/core.h>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/FunctionRef.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Getters.hpp"

#include <function2/function2.hpp>

namespace OpenVic {
	struct ProvinceInstance;
	struct CountryInstance;
	struct Event;
	struct UnitType;
	struct Invention;
	struct WargoalType;
	struct Technology;
	struct InstanceManager;

	struct ConsoleInstance {
		struct Argument {
			ConsoleInstance& console;
			std::span<std::string_view> arguments;
		};

		using execute_command_func_t = FunctionRef<bool(Argument&)>;

		using write_func_t = fu2::function_view<void(OpenVic::colour_t, std::string_view)>;

		ConsoleInstance(InstanceManager& instance_manager);
		ConsoleInstance(InstanceManager& instance_manager, write_func_t&& write_func);

		void write(std::string_view message);
		void set_write_func(write_func_t&& new_write_func);
		static void default_write_func(OpenVic::colour_t colour, std::string_view message);

		void vwrite(fmt::string_view fmt, fmt::format_args args) {
			write(memory::fmt::vformat(fmt, args));
		}

		void vwrite(colour_t colour, fmt::string_view fmt, fmt::format_args args) {
			std::swap(colour, current_colour);
			write(memory::fmt::vformat(fmt, args));
			std::swap(colour, current_colour);
		}

		template<typename... T>
		FMT_INLINE void write(fmt::format_string<T...> fmt, T&&... args) {
			vwrite(fmt, fmt::make_format_args(args...));
		}

		template<typename... T>
		FMT_INLINE void writeln(fmt::format_string<T...> fmt, T&&... args) {
			write("{}\n", memory::fmt::vformat(fmt, fmt::make_format_args(args...)));
		}

		template<typename... T>
		FMT_INLINE void write(colour_t colour, fmt::format_string<T...> fmt, T&&... args) {
			vwrite(colour, fmt, fmt::make_format_args(args...));
		}

		template<typename... T>
		FMT_INLINE void writeln(colour_t colour, fmt::format_string<T...> fmt, T&&... args) {
			write(colour, "{}\n", memory::fmt::vformat(fmt, fmt::make_format_args(args...)));
		}

		template<typename... T>
		FMT_INLINE void write_error(fmt::format_string<T...> fmt, T&&... args) {
			write(colour_t::from_floats(1, 0, 0), "{}\n", memory::fmt::vformat(fmt, fmt::make_format_args(args...)));
		}

		bool execute(std::string_view command);

		bool add_command(memory::string&& identifier, execute_command_func_t execute);

		bool validate_argument_size(std::span<std::string_view> arguments, size_t size);
		std::optional<int64_t> validate_integer(std::string_view value_string);
		std::optional<uint64_t> validate_unsigned_integer(std::string_view value_string);
		std::optional<bool> validate_boolean(std::string_view value_string);
		ProvinceInstance* validate_province_index(std::string_view value_string);
		CountryInstance* validate_country_tag(std::string_view value_string);
		Event const* validate_event_id(std::string_view value_string);
		UnitType const* validate_unit(std::string_view value_string);
		std::string_view validate_filename(std::string_view value_string);
		std::string_view validate_shader_effect(std::string_view value_string);
		std::string_view validate_texture_name(std::string_view value_string);
		std::optional<Date> validate_date(std::string_view value_string);
		Invention const* validate_invention(std::string_view value_string);
		WargoalType const* validate_cb_type(std::string_view value_string);
		Technology const* validate_tech_name(std::string_view value_string);

	private:
		string_map_t<execute_command_func_t> commands;

		write_func_t write_func;

		InstanceManager& instance_manager;

		OpenVic::colour_t PROPERTY(current_colour);

		bool setup_commands();

	public:
		InstanceManager& get_instance_manager() {
			return instance_manager;
		}

		InstanceManager const& get_instance_manager() const {
			return instance_manager;
		}
	};
}

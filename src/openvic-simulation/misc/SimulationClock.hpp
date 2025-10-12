#pragma once

#include <chrono>
#include <vector>

#include "openvic-simulation/utility/Getters.hpp"

#include <function2/function2.hpp>

namespace OpenVic {
	/* Conditionally advances game depending on speed and pause state. */
	class SimulationClock {
	public:
		using tick_function_t = fu2::function_base<true, true, fu2::capacity_can_hold<void*>, false, false, void()>;
		using update_function_t = fu2::function_base<true, true, fu2::capacity_can_hold<void*>, false, false, void()>;

		using clock_t = std::chrono::high_resolution_clock;
		using time_point_t = clock_t::time_point;
		using duration_t = clock_t::duration;

		static constexpr size_t NUM_SPEEDS = 5;
		using speed_t = int8_t;
		using speed_rates_t = std::array<duration_t, NUM_SPEEDS>;

		/* Minimum number of milliseconds before the simulation advances
		 * (in descending duration order, hence increasing speed order). */
		static constexpr speed_rates_t GAME_SPEEDS = []() constexpr -> speed_rates_t {
			using namespace std::chrono_literals;
			return {
				3s, 2s, 1s, 100ms, 1ms
			};
		}();
		static constexpr speed_t MIN_SPEED = 0, MAX_SPEED = std::size(GAME_SPEEDS) - 1;

	private:
		/* Advance simulation (triggered while unpaused at interval determined by speed). */
		tick_function_t tick_function;
		/* Refresh game state (triggered with every call to conditionally_advance_game). */
		update_function_t update_function;

		time_point_t next_tick_time;
		speed_t PROPERTY_CUSTOM_NAME(current_speed, get_simulation_speed, 0);
		bool PROPERTY_CUSTOM_PREFIX(paused, is, true);

	public:

		SimulationClock(
			tick_function_t new_tick_function, update_function_t new_update_function
		);

		time_point_t get_now() const;

		void set_paused(bool new_paused);
		void toggle_paused();

		void set_simulation_speed(speed_t speed);
		void increase_simulation_speed();
		void decrease_simulation_speed();
		bool can_increase_simulation_speed() const;
		bool can_decrease_simulation_speed() const;

		void conditionally_advance_game();
		void force_advance_game(); //for debug only
		void reset();
	};
}

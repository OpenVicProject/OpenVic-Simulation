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
		using speed_t = int8_t;

		/* Minimum number of miliseconds before the simulation advances
		 * (in descending duration order, hence increasing speed order). */
		static constexpr std::chrono::milliseconds GAME_SPEEDS[] {
			std::chrono::milliseconds { 3000 }, std::chrono::milliseconds { 2000 }, std::chrono::milliseconds { 1000 },
			std::chrono::milliseconds { 100 }, std::chrono::milliseconds { 1 }
		};
		static constexpr speed_t MIN_SPEED = 0, MAX_SPEED = std::size(GAME_SPEEDS) - 1;

	private:
		using time_point_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

		/* Advance simulation (triggered while unpaused at interval determined by speed). */
		tick_function_t tick_function;
		/* Refresh game state (triggered with every call to conditionally_advance_game). */
		update_function_t update_function;

		time_point_t last_tick_time;
		speed_t PROPERTY_CUSTOM_NAME(current_speed, get_simulation_speed, 0);
		bool PROPERTY_CUSTOM_PREFIX(paused, is, true);

	public:

		SimulationClock(
			tick_function_t new_tick_function, update_function_t new_update_function
		);

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

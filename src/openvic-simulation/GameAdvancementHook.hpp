#pragma once

#include <chrono>
#include <functional>
#include <vector>
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	// Conditionally advances game with provided behaviour
	// Class governs game speed and pause state
	class GameAdvancementHook {
	public:
		using AdvancementFunction = std::function<void()>;
		using RefreshFunction = std::function<void()>;
		using speed_t = int8_t;

		// Minimum number of miliseconds before the simulation advances
		static const std::vector<std::chrono::milliseconds> GAME_SPEEDS;

	private:
		using time_point_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

		time_point_t last_polled_time;
		// A function pointer that advances the simulation, intended to be a capturing
		// lambda or something similar. May need to be reworked later
		AdvancementFunction trigger_function;
		RefreshFunction refresh_function;
		speed_t PROPERTY_CUSTOM_NAME(current_speed, get_simulation_speed);

	public:
		bool is_paused;

		GameAdvancementHook(
			AdvancementFunction tick_function, RefreshFunction update_function, bool start_paused = true, speed_t starting_speed = 0
		);

		void set_simulation_speed(speed_t speed);
		void increase_simulation_speed();
		void decrease_simulation_speed();
		bool can_increase_simulation_speed() const;
		bool can_decrease_simulation_speed() const;
		GameAdvancementHook& operator++();
		GameAdvancementHook& operator--();
		void conditionally_advance_game();
		void reset();
	};
}

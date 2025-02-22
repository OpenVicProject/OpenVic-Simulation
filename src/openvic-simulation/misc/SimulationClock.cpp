#include "SimulationClock.hpp"

#include <algorithm>

using namespace OpenVic;

SimulationClock::SimulationClock(
	tick_function_t new_tick_function, update_function_t new_update_function
) : tick_function { new_tick_function ? std::move(new_tick_function) : []() {} },
	update_function { new_update_function ? std::move(new_update_function) : []() {} } {
	reset();
}

void SimulationClock::set_paused(bool new_paused) {
	if (paused != new_paused) {
		toggle_paused();
	}
}

void SimulationClock::toggle_paused() {
	paused = !paused;
}

void SimulationClock::set_simulation_speed(speed_t speed) {
	speed = std::clamp(speed, MIN_SPEED, MAX_SPEED);
	if (current_speed != speed) {
		current_speed = speed;
	}
}

void SimulationClock::increase_simulation_speed() {
	set_simulation_speed(current_speed + 1);
}

void SimulationClock::decrease_simulation_speed() {
	set_simulation_speed(current_speed - 1);
}

bool SimulationClock::can_increase_simulation_speed() const {
	return current_speed < MAX_SPEED;
}

bool SimulationClock::can_decrease_simulation_speed() const {
	return current_speed > MIN_SPEED;
}

void SimulationClock::conditionally_advance_game() {
	if (!paused) {
		const time_point_t current_time = std::chrono::high_resolution_clock::now();
		const std::chrono::milliseconds time_since_last_tick =
			std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_tick_time);
		if (time_since_last_tick >= GAME_SPEEDS[current_speed]) {
			last_tick_time = current_time;
			tick_function();
		}
	}
	update_function();
}

void SimulationClock::reset() {
	paused = true;
	current_speed = 0;
	last_tick_time = std::chrono::high_resolution_clock::now();
}

#include "GameAdvancementHook.hpp"

using namespace OpenVic;

const std::vector<std::chrono::milliseconds> GameAdvancementHook::GAME_SPEEDS = {
	std::chrono::milliseconds { 3000 }, std::chrono::milliseconds { 2000 }, std::chrono::milliseconds { 1000 },
	std::chrono::milliseconds { 100 }, std::chrono::milliseconds { 1 }
};

GameAdvancementHook::GameAdvancementHook(
	AdvancementFunction tick_function, RefreshFunction update_function, bool start_paused, speed_t starting_speed
)
	: trigger_function { tick_function }, refresh_function { update_function }, is_paused { start_paused } {
	last_polled_time = std::chrono::high_resolution_clock::now();
	set_simulation_speed(starting_speed);
}

void GameAdvancementHook::set_simulation_speed(speed_t speed) {
	if (speed < 0) {
		current_speed = 0;
	} else if (speed >= GAME_SPEEDS.size()) {
		current_speed = GAME_SPEEDS.size() - 1;
	} else {
		current_speed = speed;
	}
}

void GameAdvancementHook::increase_simulation_speed() {
	set_simulation_speed(current_speed + 1);
}

void GameAdvancementHook::decrease_simulation_speed() {
	set_simulation_speed(current_speed - 1);
}

bool GameAdvancementHook::can_increase_simulation_speed() const {
	return current_speed + 1 < GAME_SPEEDS.size();
}

bool GameAdvancementHook::can_decrease_simulation_speed() const {
	return current_speed > 0;
}

GameAdvancementHook& GameAdvancementHook::operator++() {
	increase_simulation_speed();
	return *this;
};

GameAdvancementHook& GameAdvancementHook::operator--() {
	decrease_simulation_speed();
	return *this;
};

void GameAdvancementHook::conditionally_advance_game() {
	if (!is_paused) {
		time_point_t currentTime = std::chrono::high_resolution_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - last_polled_time) >= GAME_SPEEDS[current_speed]) {
			last_polled_time = currentTime;
			if (trigger_function) {
				trigger_function();
			}
		}
	}
	if (refresh_function) {
		refresh_function();
	}
}

void GameAdvancementHook::reset() {
	is_paused = true;
	current_speed = 0;
}

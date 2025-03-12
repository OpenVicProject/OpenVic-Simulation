#pragma once

#include <string_view>

#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {

	class Requirement {

		// Loaded during construction
		std::string PROPERTY(id);
		std::string PROPERTY(text);
		std::string PROPERTY(acceptance_criteria);
		bool PROPERTY(pass);
		bool PROPERTY_RW(tested);

		// Initialised and used during script execution
		std::string PROPERTY(target_value);
		std::string PROPERTY(actual_value);

	public:
		Requirement(std::string_view in_id, std::string_view in_text, std::string_view in_acceptance_criteria) {
			id = in_id;
			text = in_text;
			acceptance_criteria = in_acceptance_criteria;
			pass = false; // Explicitly false to begin
			tested = false;
		}

		void set_pass(bool in_pass);
		void set_target_value(std::string_view new_target_value);
		void set_actual_value(std::string_view new_actual_value);
	};
}

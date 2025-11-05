#pragma once
#include <string>
#include "openvic-simulation/core/Property.hpp"

namespace OpenVic {

	class Requirement {

		// Loaded during construction
		memory::string PROPERTY(id);
		memory::string PROPERTY(text);
		memory::string PROPERTY(acceptance_criteria);
		bool PROPERTY(pass);
		bool PROPERTY_RW(tested);

		// Initialised and used during script execution
		memory::string PROPERTY(target_value);
		memory::string PROPERTY(actual_value);

	public:
		Requirement(memory::string in_id, memory::string in_text, memory::string in_acceptance_criteria) {
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

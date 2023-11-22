#pragma once
#include <string>
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {

	class Requirement {

		// Loaded during construction
		std::string PROPERTY_RW(id);
		std::string PROPERTY_RW(text);
		std::string PROPERTY_RW(acceptance_criteria);
		bool PROPERTY(pass);
		bool PROPERTY_RW(tested);

		// Initialised and used during script execution
		std::string PROPERTY_RW(target_value);
		std::string PROPERTY_RW(actual_value);

	public:
		Requirement(std::string in_id, std::string in_text, std::string in_acceptance_criteria) {
			id = in_id;
			text = in_text;
			acceptance_criteria = in_acceptance_criteria;
			pass = false; // Explicitly false to begin
			tested = false;
		}

		void set_pass(bool in_pass);
	};
}

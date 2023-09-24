#pragma once
#include <string>

namespace OpenVic {

	class Requirement {

		// Loaded during construction
		std::string id;
		std::string text;
		std::string acceptance_criteria;
		bool pass = false;	// Explicitly false to begin

		// Initialised and used during script execution
		std::string target_value;
		std::string actual_value;

	public:

		Requirement(std::string in_id, std::string in_text, std::string in_acceptance_criteria) {
			id = in_id;
			text = in_text;
			acceptance_criteria = in_acceptance_criteria;
		}

		// Getters
		std::string get_id();
		std::string get_text();
		std::string get_acceptance_criteria();
		bool get_pass();
		std::string get_target_value();
		std::string get_actual_value();

		// Setters
		void set_id(std::string in_id);
		void set_text(std::string in_text);
		void set_acceptance_criteria(std::string in_acceptance_criteria);
		void set_pass(bool in_pass);
		void set_target_value(std::string in_target_value);
		void set_actual_value(std::string in_actual_value);
	};
}

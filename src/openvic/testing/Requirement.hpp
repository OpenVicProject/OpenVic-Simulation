#pragma once
#include <string>

namespace OpenVic {

	class Requirement {

		std::string id;
		std::string text;
		std::string acceptance_criteria;
		bool pass = false;	// Explicitly false to begin

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

		// Setters
		void set_id(std::string in_id);
		void set_text(std::string in_text);
		void set_acceptance_criteria(std::string in_acceptance_criteria);
		void set_pass(bool in_pass);
	};
}
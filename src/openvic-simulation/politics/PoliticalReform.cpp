#include "PoliticalReform.hpp"

using namespace OpenVic;

PoliticalReformGroup::PoliticalReformGroup(const std::string_view new_identifier, bool ordered) 
	: HasIdentifier { new_identifier }, ordered { ordered } {}

bool PoliticalReformGroup::is_ordered() const {
	return ordered;
}

PoliticalReform::PoliticalReform(const std::string_view new_identifier, PoliticalReformGroup const& new_group, size_t ordinal)
	: HasIdentifier { new_identifier }, group { new_group }, ordinal { ordinal } {}

size_t PoliticalReform::get_ordinal() const {
	return ordinal;
}

PoliticalReformManager::PoliticalReformManager() : political_reform_groups { "political reform groups" }, political_reforms { "political reforms" } {}

bool PoliticalReformManager::add_political_reform_group(const std::string_view identifier, bool ordered) {
	if (identifier.empty()) {
		Logger::error("Invalid political reform group identifier - empty!");
		return false;
	}

	return political_reform_groups.add_item({ identifier, ordered });
}

bool PoliticalReformManager::add_political_reform(const std::string_view identifier, PoliticalReformGroup const* group, size_t ordinal) {
	if (identifier.empty()) {
		Logger::error("Invalid political reform identifier - empty!");
		return false;
	}

	if (group == nullptr) {
		Logger::error("Null political reform group for ", identifier);
		return false;
	}

	return political_reforms.add_item({ identifier, *group, ordinal });
}
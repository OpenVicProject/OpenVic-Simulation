#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/history/diplomacy/AllianceHistory.hpp"
#include "openvic-simulation/history/diplomacy/ReparationsHistory.hpp"
#include "openvic-simulation/history/diplomacy/SubjectHistory.hpp"
#include "openvic-simulation/history/diplomacy/WarHistory.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct CountryDefinitionManager;
	struct DefinitionManager;

	struct DiplomaticHistoryManager {
	private:
		memory::vector<AllianceHistory> alliances;
		memory::vector<ReparationsHistory> reparations;
		memory::vector<SubjectHistory> subjects;
		memory::vector<WarHistory> wars;
		bool locked = false;

		template<typename HistoryType>
		static memory::vector<HistoryType const*> filter_by_date(
			std::span<const HistoryType> items,
			const Date date
		) {
			memory::vector<HistoryType const*> ret;
			for (auto const& item : items) {
				if (item.period.is_date_in_period(date)) {
					ret.push_back(&item);
				}
			}
			return ret;
		}

	public:
		DiplomaticHistoryManager() {}

		void reserve_more_wars(size_t size);
		void lock_diplomatic_history();
		[[nodiscard]] constexpr bool is_locked() const {
			return locked;
		}

		[[nodiscard]] memory::vector<AllianceHistory const*> get_alliances(Date date) const {
			return filter_by_date<AllianceHistory>(alliances, date);
		}
		[[nodiscard]] memory::vector<ReparationsHistory const*> get_reparations(Date date) const {
			return filter_by_date<ReparationsHistory>(reparations, date);
		}
		[[nodiscard]] memory::vector<SubjectHistory const*> get_subjects(Date date) const{
			return filter_by_date<SubjectHistory>(subjects, date);
		}
		/* Returns all wars that begin before date. NOTE: Some wargoals may be added or countries may join after date,
		 * should be checked for by functions that use get_wars() */
		[[nodiscard]] memory::vector<WarHistory const*> get_wars(Date date) const;

		bool load_diplomacy_history_file(CountryDefinitionManager const& country_definition_manager, ast::NodeCPtr root);
		bool load_war_history_file(DefinitionManager const& definition_manager, ast::NodeCPtr root);
	};
}

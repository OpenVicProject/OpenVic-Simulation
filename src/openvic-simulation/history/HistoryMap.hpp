#pragma once

#include <system_error>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {

	struct HistoryEntry {
	private:
		Date PROPERTY(date);

	protected:
		HistoryEntry(Date new_date);
	};

	struct DefinitionManager;

	namespace _HistoryMapHelperFuncs {
		/* Helper functions to avoid cyclic dependency issues */
		Date _get_start_date(DefinitionManager const& definition_manager);
		Date _get_end_date(DefinitionManager const& definition_manager);
	}

	template<std::derived_from<HistoryEntry> _Entry, typename... Args>
	struct HistoryMap {
		using entry_type = _Entry;

	private:
		ordered_map<Date, memory::unique_ptr<entry_type>> PROPERTY(entries);

		bool _try_load_history_entry(
			DefinitionManager const& definition_manager, Args... args, Date date, ast::NodeCPtr root
		) {
			entry_type *const entry = _get_or_make_entry(definition_manager, date);
			if (entry != nullptr) {
				return _load_history_entry(definition_manager, args..., *entry, root);
			} else {
				return false;
			}
		}

	protected:
		constexpr HistoryMap() {};

		virtual memory::unique_ptr<entry_type> _make_entry(Date date) const = 0;

		virtual bool _load_history_entry(
			DefinitionManager const& definition_manager, Args... args, entry_type& entry, ast::NodeCPtr root
		) = 0;

		bool _load_history_file(DefinitionManager const& definition_manager, Args... args, ast::NodeCPtr root) {
			return _try_load_history_entry(
				definition_manager, args..., _HistoryMapHelperFuncs::_get_start_date(definition_manager), root
			);
		}

		bool _load_history_sub_entry_callback(
			DefinitionManager const& definition_manager,
			Args... args,
			Date date,
			ast::NodeCPtr root,
			NodeTools::template_key_map_t<StringMapCaseSensitive> const& key_map,
			std::string_view key,
			ast::NodeCPtr value
		) {
			/* Date blocks (loaded into the corresponding HistoryEntry) */
			Date::from_chars_result result;
			const Date sub_date { Date::from_string(key, &result) };
			if (result.ec == std::errc{}) {
				if (sub_date < date) {
					spdlog::warn_s("History entry {} defined before parent entry date {}", sub_date, date);
				}
				if (sub_date == date) {
					spdlog::warn_s("History entry {} defined with same date as parent entry", sub_date);
				}
				if (_try_load_history_entry(definition_manager, args..., sub_date, value)) {
					return true;
				} else {
					spdlog::error_s("Failed to load history entry at date {}", sub_date);
					return false;
				}
			}
			
			return NodeTools::map_key_value_invalid_callback<NodeTools::template_key_map_t<StringMapCaseSensitive>>(key_map, key, value);
		}

		/* Returns history entry at specific date, if date doesn't have an entry creates one, if that fails returns nullptr. */
		entry_type* _get_or_make_entry(DefinitionManager const& definition_manager, Date date) {
			const Date end_date = _HistoryMapHelperFuncs::_get_end_date(definition_manager);
			if (date > end_date) {
				spdlog::error_s("History entry {} defined after end date {}", date, end_date);
				return nullptr;
			}
			typename decltype(entries)::iterator it = entries.find(date);
			if (it == entries.end()) {
				const std::pair<typename decltype(entries)::iterator, bool> result = entries.emplace(date, _make_entry(date));
				if (result.second) {
					it = result.first;
				} else {
					spdlog::error_s("Failed to create history entry at date {}", date);
					return nullptr;
				}
			}
			return it->second.get();
		}

	public:
		void sort_entries() {
			memory::vector<Date> keys;
			keys.reserve(entries.size());
			for (typename decltype(entries)::value_type const& entry : entries) {
				keys.push_back(entry.first);
			}
			std::sort(keys.begin(), keys.end());
			ordered_map<Date, memory::unique_ptr<entry_type>> new_entries;
			for (Date const& key : keys) {
				new_entries.emplace(key, std::move(entries[key]));
			}
			entries = std::move(new_entries);
		}

		/* Returns history entry at specific date, if date doesn't have an entry returns nullptr. */
		entry_type const* get_entry(Date date) const {
			typename decltype(entries)::const_iterator it = entries.find(date);
			if (it != entries.end()) {
				return it->second.get();
			}
			return nullptr;
		}
	};
}

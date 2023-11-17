#pragma once

#include <map>
#include <memory>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Date.hpp"

namespace OpenVic {

	struct HistoryEntry {
	private:
		Date PROPERTY(date);

	protected:
		HistoryEntry(Date new_date);
	};

	struct GameManager;

	/* Helper functions to avoid cyclic dependency issues */
	Date _get_start_date(GameManager const& game_manager);
	Date _get_end_date(GameManager const& game_manager);

	template<std::derived_from<HistoryEntry> _Entry, typename... Args>
	struct HistoryMap {
		using entry_type = _Entry;

	private:
		std::map<Date, std::unique_ptr<entry_type>> PROPERTY(entries);

		bool _try_load_history_entry(GameManager const& game_manager, Args... args, Date date, ast::NodeCPtr root) {
			const Date end_date = _get_end_date(game_manager);
			if (date > end_date) {
				Logger::error("History entry ", date, " defined after end date ", end_date);
				return false;
			}
			typename decltype(entries)::iterator it = entries.find(date);
			if (it == entries.end()) {
				const std::pair<typename decltype(entries)::iterator, bool> result = entries.emplace(date, _make_entry(date));
				if (result.second) {
					it = result.first;
				} else {
					Logger::error("Failed to create history entry at date ", date);
					return false;
				}
			}
			return _load_history_entry(game_manager, args..., *it->second, root);
		}

	protected:
		HistoryMap() = default;

		virtual std::unique_ptr<entry_type> _make_entry(Date date) const = 0;

		virtual bool _load_history_entry(
			GameManager const& game_manager, Args... args, entry_type& entry, ast::NodeCPtr root
		) = 0;

		bool _load_history_file(GameManager const& game_manager, Args... args, ast::NodeCPtr root) {
			return _try_load_history_entry(game_manager, args..., _get_start_date(game_manager), root);
		}

		bool _load_history_sub_entry_callback(
			GameManager const& game_manager, Args... args, Date date, ast::NodeCPtr root, std::string_view key,
			ast::NodeCPtr value, NodeTools::key_value_callback_t default_callback = NodeTools::key_value_invalid_callback
		) {
			/* Date blocks (loaded into the corresponding HistoryEntry) */
			bool is_date = false;
			const Date sub_date { Date::from_string(key, &is_date, true) };
			if (is_date) {
				if (sub_date < date) {
					Logger::error("History entry ", sub_date, " defined before parent entry date ", date);
					return false;
				}
				if (sub_date == date) {
					Logger::warning("History entry ", sub_date, " defined with same date as parent entry");
				}
				if (_try_load_history_entry(game_manager, args..., sub_date, value)) {
					return true;
				} else {
					Logger::error("Failed to load history entry at date ", sub_date);
					return false;
				}
			}

			return default_callback(key, value);
		}

	public:
		/* Returns history entry at specific date, if date doesn't have an entry returns nullptr. */
		entry_type const* get_entry(Date date) const {
			typename decltype(entries)::const_iterator it = entries.find(date);
			if (it != entries.end()) {
				return it->second.get();
			}
			return nullptr;
		}
		/* Returns history entries up to date as an ordered list of entries. */
		std::vector<entry_type const*> get_entries_up_to(Date end) const {
			std::vector<entry_type const*> ret;
			for (typename decltype(entries)::value_type const& entry : entries) {
				if (entry.first <= end) {
					ret.push_back(entry.second.get());
				}
			}
			std::sort(ret.begin(), ret.end(), [](entry_type const* lhs, entry_type const* rhs) -> bool {
				return lhs->get_date() < rhs->get_date();
			});
			return ret;
		}
	};
}

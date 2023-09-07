#pragma once

#include <map>
#include <vector>

#include "openvic/types/Colour.hpp"
#include "openvic/utility/Logger.hpp"

namespace OpenVic {
	/*
	 * Base class for objects with a non-empty string identifier,
	 * uniquely named instances of which can be entered into an
	 * IdentifierRegistry instance.
	 */
	class HasIdentifier {
		const std::string identifier;

	protected:
		HasIdentifier(const std::string_view new_identifier);

	public:
		HasIdentifier(HasIdentifier const&) = delete;
		HasIdentifier(HasIdentifier&&) = default;
		HasIdentifier& operator=(HasIdentifier const&) = delete;
		HasIdentifier& operator=(HasIdentifier&&) = delete;

		std::string const& get_identifier() const;
	};

	/*
	 * Base class for objects with associated colour information.
	 */
	class HasColour {
		const colour_t colour;

	protected:
		HasColour(const colour_t new_colour, bool can_be_null);

	public:
		HasColour(HasColour const&) = delete;
		HasColour(HasColour&&) = default;
		HasColour& operator=(HasColour const&) = delete;
		HasColour& operator=(HasColour&&) = delete;

		colour_t get_colour() const;
		std::string colour_to_hex_string() const;
	};

	/*
	 * Base class for objects with a unique string identifier
	 * and associated colour information.
	 */
	class HasIdentifierAndColour : public HasIdentifier, public HasColour {
	protected:
		HasIdentifierAndColour(const std::string_view new_identifier, const colour_t new_colour, bool can_be_null);

	public:
		HasIdentifierAndColour(HasIdentifierAndColour const&) = delete;
		HasIdentifierAndColour(HasIdentifierAndColour&&) = default;
		HasIdentifierAndColour& operator=(HasIdentifierAndColour const&) = delete;
		HasIdentifierAndColour& operator=(HasIdentifierAndColour&&) = delete;
	};

	using distribution_t = std::map<HasIdentifierAndColour const*, float>;

	distribution_t::value_type get_largest_item(distribution_t const& dist);

	/*
	 * Template for a list of objects with unique string identifiers that can
	 * be locked to prevent any further additions. The template argument T is
	 * the type of object that the registry will store, and the second part ensures
	 * that HasIdentifier is a base class of T.
	 */
	template<typename T>
	requires(std::derived_from<T, HasIdentifier>)
	class IdentifierRegistry {
		using identifier_index_map_t = std::map<std::string, size_t, std::less<void>>;

		const std::string name;
		std::vector<T> items;
		bool locked = false;
		identifier_index_map_t identifier_index_map;

	public:
		IdentifierRegistry(const std::string_view new_name) : name { new_name } {}
		bool add_item(T&& item) {
			if (locked) {
				Logger::error("Cannot add item to the ", name, " registry - locked!");
				return false;
			}
			T const* old_item = get_item_by_identifier(item.get_identifier());
			if (old_item != nullptr) {
				Logger::error("Cannot add item to the ", name, " registry - an item with the identifier \"", item.get_identifier(), "\" already exists!");
				return false;
			}
			identifier_index_map[item.get_identifier()] = items.size();
			items.push_back(std::move(item));
			return true;
		}
		void lock(bool log = true) {
			if (locked) {
				Logger::error("Failed to lock ", name, " registry - already locked!");
			} else {
				locked = true;
				if (log) Logger::info("Locked ", name, " registry after registering ", size(), " items");
			}
		}
		bool is_locked() const {
			return locked;
		}
		void reset() {
			identifier_index_map.clear();
			items.clear();
			locked = false;
		}
		size_t size() const {
			return items.size();
		}
		bool empty() const {
			return items.empty();
		}
		void reserve(size_t size) {
			if (locked) {
				Logger::error("Failed to reserve space for ", size, " items in ", name, " registry - already locked!");
			} else {
				items.reserve(size);
			}
		}
		T* get_item_by_identifier(const std::string_view identifier) {
			const identifier_index_map_t::const_iterator it = identifier_index_map.find(identifier);
			if (it != identifier_index_map.end()) return &items[it->second];
			return nullptr;
		}
		T const* get_item_by_identifier(const std::string_view identifier) const {
			const identifier_index_map_t::const_iterator it = identifier_index_map.find(identifier);
			if (it != identifier_index_map.end()) return &items[it->second];
			return nullptr;
		}
		T* get_item_by_index(size_t index) {
			return index < items.size() ? &items[index] : nullptr;
		}
		T const* get_item_by_index(size_t index) const {
			return index < items.size() ? &items[index] : nullptr;
		}
		std::vector<T>& get_items() {
			return items;
		}
		std::vector<T> const& get_items() const {
			return items;
		}
	};
}

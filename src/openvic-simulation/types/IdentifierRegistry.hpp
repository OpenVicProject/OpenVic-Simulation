#pragma once

#include <map>
#include <vector>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/utility/Logger.hpp"

#define REF_GETTERS(var) \
	constexpr decltype(var)& get_##var() { return var; } \
	constexpr decltype(var) const& get_##var() const { return var; }

namespace OpenVic {
	/*
	 * Base class for objects with a non-empty string identifier,
	 * uniquely named instances of which can be entered into an
	 * IdentifierRegistry instance.
	 */
	class HasIdentifier {
		const std::string identifier;

	protected:
		HasIdentifier(std::string_view new_identifier);

	public:
		HasIdentifier(HasIdentifier const&) = delete;
		HasIdentifier(HasIdentifier&&) = default;
		HasIdentifier& operator=(HasIdentifier const&) = delete;
		HasIdentifier& operator=(HasIdentifier&&) = delete;

		std::string_view get_identifier() const;
	};

	std::ostream& operator<<(std::ostream& stream, HasIdentifier const& obj);
	std::ostream& operator<<(std::ostream& stream, HasIdentifier const* obj);

	/*
	 * Base class for objects with associated colour information.
	 */
	class HasColour {
		const colour_t colour;

	protected:
		HasColour(const colour_t new_colour, bool can_be_null, bool can_have_alpha);

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
		HasIdentifierAndColour(std::string_view new_identifier, const colour_t new_colour, bool can_be_null, bool can_have_alpha);

	public:
		HasIdentifierAndColour(HasIdentifierAndColour const&) = delete;
		HasIdentifierAndColour(HasIdentifierAndColour&&) = default;
		HasIdentifierAndColour& operator=(HasIdentifierAndColour const&) = delete;
		HasIdentifierAndColour& operator=(HasIdentifierAndColour&&) = delete;
	};

	using distribution_t = std::map<HasIdentifierAndColour const*, float>;

	distribution_t::value_type get_largest_item(distribution_t const& dist);

	template<typename T>
	using get_identifier_func_t = std::string_view(T::*)(void) const;

	template<typename _Base, std::derived_from<_Base> _Type, get_identifier_func_t<_Base> get_identifier,
		typename _Storage, _Type* (*get_ptr)(_Storage&), _Type const* (*get_cptr)(_Storage const&)>
	class UniqueKeyRegistry {

		const std::string name;
		const bool log_lock;
		std::vector<_Storage> items;
		bool locked = false;
		string_map_t<size_t> identifier_index_map;

	public:
		using value_type = _Type;
		using storage_type = _Storage;

		UniqueKeyRegistry(std::string_view new_name, bool new_log_lock = true)
			: name { new_name }, log_lock { new_log_lock } {}

		std::string_view get_name() const {
			return name;
		}

		bool add_item(storage_type&& item, bool fail_on_duplicate = true) {
			if (locked) {
				Logger::error("Cannot add item to the ", name, " registry - locked!");
				return false;
			}
			value_type const* new_item = (*get_cptr)(item);
			const std::string_view new_identifier = (new_item->*get_identifier)();
			value_type const* old_item = get_item_by_identifier(new_identifier);
			if (old_item != nullptr) {
#define DUPLICATE_MESSAGE "Cannot add item to the ", name, " registry - an item with the identifier \"", new_identifier, "\" already exists!"
				if (fail_on_duplicate) {
					Logger::error(DUPLICATE_MESSAGE);
					return false;
				} else {
					Logger::warning(DUPLICATE_MESSAGE);
					return true;
				}
#undef DUPLICATE_MESSAGE
			}
			identifier_index_map.emplace(new_identifier, items.size());
			items.push_back(std::move(item));
			return true;
		}

		void lock() {
			if (locked) {
				Logger::error("Failed to lock ", name, " registry - already locked!");
			} else {
				locked = true;
				if (log_lock) Logger::info("Locked ", name, " registry after registering ", size(), " items");
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

		value_type* get_item_by_identifier(std::string_view identifier) {
			const typename decltype(identifier_index_map)::const_iterator it = identifier_index_map.find(identifier);
			if (it != identifier_index_map.end()) return (*get_ptr)(items[it->second]);
			return nullptr;
		}

		value_type const* get_item_by_identifier(std::string_view identifier) const {
			const typename decltype(identifier_index_map)::const_iterator it = identifier_index_map.find(identifier);
			if (it != identifier_index_map.end()) return (*get_cptr)(items[it->second]);
			return nullptr;
		}

		bool has_identifier(std::string_view identifier) const {
			return get_item_by_identifier(identifier) != nullptr;
		}

		value_type* get_item_by_index(size_t index) {
			return index < items.size() ? &items[index] : nullptr;
		}

		value_type const* get_item_by_index(size_t index) const {
			return index < items.size() ? &items[index] : nullptr;
		}

		bool has_index(size_t index) const {
			return get_item_by_index(index) != nullptr;
		}

		REF_GETTERS(items)

		std::vector<std::string_view> get_item_identifiers() const {
			std::vector<std::string_view> identifiers;
			identifiers.reserve(items.size());
			for (typename decltype(identifier_index_map)::value_type const& entry : identifier_index_map) {
				identifiers.push_back(entry.first);
			}
			return identifiers;
		}

		NodeTools::callback_t<std::string_view> expect_item_identifier(NodeTools::callback_t<value_type&> callback) {
			return [this, callback](std::string_view identifier) -> bool {
				value_type* item = get_item_by_identifier(identifier);
				if (item != nullptr) return callback(*item);
				Logger::error("Invalid ", name, ": ", identifier);
				return false;
			};
		}

		NodeTools::callback_t<std::string_view> expect_item_identifier(NodeTools::callback_t<value_type const&> callback) const {
			return [this, callback](std::string_view identifier) -> bool {
				value_type const* item = get_item_by_identifier(identifier);
				if (item != nullptr) return callback(*item);
				Logger::error("Invalid ", name, ": ", identifier);
				return false;
			};
		}

		NodeTools::node_callback_t expect_item_dictionary(NodeTools::callback_t<value_type&, ast::NodeCPtr> callback) {
			return NodeTools::expect_dictionary([this, callback](std::string_view key, ast::NodeCPtr value) -> bool {
				value_type* item = get_item_by_identifier(key);
				if (item != nullptr) {
					return callback(*item, value);
				}
				Logger::error("Invalid ", name, " identifier: ", key);
				return false;
			});
		}

		NodeTools::node_callback_t expect_item_dictionary(NodeTools::callback_t<value_type const&, ast::NodeCPtr> callback) const {
			return NodeTools::expect_dictionary([this, callback](std::string_view key, ast::NodeCPtr value) -> bool {
				value_type const* item = get_item_by_identifier(key);
				if (item != nullptr) {
					return callback(*item, value);
				}
				Logger::error("Invalid ", name, " identifier: ", key);
				return false;
			});
		}

		NodeTools::node_callback_t expect_item_decimal_map(NodeTools::callback_t<std::map<value_type const*, fixed_point_t>&&> callback) const {
			return [this, callback](ast::NodeCPtr node) -> bool {
				std::map<value_type const*, fixed_point_t> map;
				bool ret = expect_item_dictionary([&map](value_type const& key, ast::NodeCPtr value) -> bool {
					fixed_point_t val;
					const bool ret = NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(val))(value);
					map.emplace(&key, val);
					return ret;
				})(node);
				ret &= callback(std::move(map));
				return ret;
			};
		}
	};

	template<typename T>
	[[nodiscard]] inline constexpr T* _addressof(T& v) noexcept {
		return std::addressof<T>(v);
	}

	template<typename T>
	const T* _addressof(const T&&) = delete;

	template<typename _Base, std::derived_from<_Base> _Type, get_identifier_func_t<_Base> get_identifier>
	using ValueRegistry = UniqueKeyRegistry<_Base, _Type, get_identifier, _Type, _addressof<_Type>, _addressof<const _Type>>;

	template<typename _Type>
	constexpr _Type* get_ptr(std::unique_ptr<_Type>& storage) {
		return storage.get();
	}
	template<typename _Type>
	constexpr _Type const* get_cptr(std::unique_ptr<_Type> const& storage) {
		return storage.get();
	}

	template<typename _Base, std::derived_from<_Base> _Type, get_identifier_func_t<_Base> get_identifier>
	using InstanceRegistry = UniqueKeyRegistry<_Base, _Type, get_identifier, std::unique_ptr<_Type>,
		get_ptr<_Type>, get_cptr<_Type>>;

	template<std::derived_from<HasIdentifier> _Type>
	using IdentifierRegistry = ValueRegistry<HasIdentifier, _Type, &HasIdentifier::get_identifier>;

	template<std::derived_from<HasIdentifier> _Type>
	using IdentifierInstanceRegistry = InstanceRegistry<HasIdentifier, _Type, &HasIdentifier::get_identifier>;

#define IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(singular, plural) \
	void lock_##plural() { plural.lock(); } \
	bool plural##_are_locked() const { return plural.is_locked(); } \
	decltype(plural)::value_type const* get_##singular##_by_identifier(std::string_view identifier) const { \
		return plural.get_item_by_identifier(identifier); } \
	bool has_##singular##_identifier(std::string_view identifier) const { \
		return plural.has_identifier(identifier); } \
	size_t get_##singular##_count() const { \
		return plural.size(); } \
	std::vector<decltype(plural)::storage_type> const& get_##plural() const { \
		return plural.get_items(); } \
	std::vector<std::string_view> get_##singular##_identifiers() const { \
		return plural.get_item_identifiers(); } \
	NodeTools::callback_t<std::string_view> expect_##singular##_identifier(NodeTools::callback_t<decltype(plural)::value_type const&> callback) const { \
		return plural.expect_item_identifier(callback); } \
	NodeTools::node_callback_t expect_##singular##_dictionary(NodeTools::callback_t<decltype(plural)::value_type const&, ast::NodeCPtr> callback) const { \
		return plural.expect_item_dictionary(callback); } \
	NodeTools::node_callback_t expect_##singular##_decimal_map(NodeTools::callback_t<std::map<decltype(plural)::value_type const*, fixed_point_t>&&> callback) const { \
		return plural.expect_item_decimal_map(callback); }

#define IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_PLURAL(singular, plural) \
	decltype(plural)::value_type* get_##singular##_by_identifier(std::string_view identifier) { \
		return plural.get_item_by_identifier(identifier); } \
	NodeTools::callback_t<std::string_view> expect_##singular##_identifier(NodeTools::callback_t<decltype(plural)::value_type&> callback) { \
		return plural.expect_item_identifier(callback); } \
	NodeTools::node_callback_t expect_##singular##_dictionary(NodeTools::callback_t<decltype(plural)::value_type&, ast::NodeCPtr> callback) { \
		return plural.expect_item_dictionary(callback); }

#define IDENTIFIER_REGISTRY_ACCESSORS(name) IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(name, name##s)
#define IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(name) IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_PLURAL(name, name##s)
}

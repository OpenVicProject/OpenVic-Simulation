#pragma once

#include <concepts>
#include <map>
#include <type_traits>
#include <vector>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Logger.hpp"

namespace OpenVic {

	constexpr bool valid_basic_identifier_char(char c) {
		return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || c == '_';
	}
	constexpr bool valid_basic_identifier(std::string_view identifier) {
		return std::all_of(identifier.begin(), identifier.end(), valid_basic_identifier_char);
	}
	constexpr std::string_view extract_basic_identifier_prefix(std::string_view identifier) {
		size_t len = 0;
		while (len < identifier.size() && valid_basic_identifier_char(identifier[len])) {
			++len;
		}
		return { identifier.data(), len };
	}

	/*
	 * Base class for objects with a non-empty string identifier. Uniquely named instances of a type derived from this class
	 * can be entered into an IdentifierRegistry instance.
	 */
	class HasIdentifier {
		const std::string PROPERTY(identifier);

	protected:
		HasIdentifier(std::string_view new_identifier);

	public:
		HasIdentifier(HasIdentifier const&) = delete;
		HasIdentifier(HasIdentifier&&) = default;
		HasIdentifier& operator=(HasIdentifier const&) = delete;
		HasIdentifier& operator=(HasIdentifier&&) = delete;
	};

	std::ostream& operator<<(std::ostream& stream, HasIdentifier const& obj);
	std::ostream& operator<<(std::ostream& stream, HasIdentifier const* obj);

	/*
	 * Base class for objects with associated colour information.
	 */
	class HasColour {
		const colour_t PROPERTY(colour);

	protected:
		HasColour(colour_t new_colour, bool cannot_be_null, bool can_have_alpha);

	public:
		HasColour(HasColour const&) = delete;
		HasColour(HasColour&&) = default;
		HasColour& operator=(HasColour const&) = delete;
		HasColour& operator=(HasColour&&) = delete;

		std::string colour_to_hex_string() const;
	};

	/*
	 * Base class for objects with a unique string identifier and associated colour information.
	 */
	class HasIdentifierAndColour : public HasIdentifier, public HasColour {
	protected:
		HasIdentifierAndColour(std::string_view new_identifier, colour_t new_colour, bool cannot_be_null, bool can_have_alpha);

	public:
		HasIdentifierAndColour(HasIdentifierAndColour const&) = delete;
		HasIdentifierAndColour(HasIdentifierAndColour&&) = default;
		HasIdentifierAndColour& operator=(HasIdentifierAndColour const&) = delete;
		HasIdentifierAndColour& operator=(HasIdentifierAndColour&&) = delete;
	};

	/* Callbacks for trying to add duplicate keys via UniqueKeyRegistry::add_item */
	static bool duplicate_fail_callback(std::string_view registry_name, std::string_view duplicate_identifier) {
		Logger::error(
			"Failure adding item to the ", registry_name, " registry - an item with the identifier \"", duplicate_identifier,
			"\" already exists!"
		);
		return false;
	}
	static bool duplicate_warning_callback(std::string_view registry_name, std::string_view duplicate_identifier) {
		Logger::warning(
			"Warning adding item to the ", registry_name, " registry - an item with the identifier \"", duplicate_identifier,
			"\" already exists!"
		);
		return true;
	}
	static bool duplicate_ignore_callback(std::string_view registry_name, std::string_view duplicate_identifier) {
		return true;
	}

	/* _GetIdentifier - takes _Type const* and returns std::string_view
	 * _GetPointer - takes _Storage [const]& and returns T [const]*
	 */
	template<typename _Type, typename _Storage, typename _GetIdentifier, typename _GetPointer>
	class UniqueKeyRegistry {

		const std::string name;
		const bool log_lock;
		std::vector<_Storage> items;
		bool locked = false;
		string_map_t<size_t> identifier_index_map;

		_GetIdentifier GetIdentifier;
		_GetPointer GetPointer;

	public:
		using value_type = _Type;
		using storage_type = _Storage;

		UniqueKeyRegistry(
			std::string_view new_name, bool new_log_lock = true, _GetIdentifier new_GetIdentifier = {},
			_GetPointer new_GetPointer = {}
		) : name { new_name }, log_lock { new_log_lock }, GetIdentifier { new_GetIdentifier }, GetPointer { new_GetPointer } {}

		std::string_view get_name() const {
			return name;
		}

		bool add_item(
			storage_type&& item,
			NodeTools::callback_t<std::string_view, std::string_view> duplicate_callback = duplicate_fail_callback
		) {
			if (locked) {
				Logger::error("Cannot add item to the ", name, " registry - locked!");
				return false;
			}
			const std::string_view new_identifier = GetIdentifier(GetPointer(item));
			if (duplicate_callback &&
				duplicate_callback.target<bool(std::string_view, std::string_view)>() == duplicate_ignore_callback) {
				if (has_identifier(new_identifier)) {
					return true;
				}
			} else {
				value_type const* old_item = get_item_by_identifier(new_identifier);
				if (old_item != nullptr) {
					return duplicate_callback(name, new_identifier);
				}
			}
			const std::pair<string_map_t<size_t>::iterator, bool> ret =
				identifier_index_map.emplace(std::move(new_identifier), items.size());
			items.emplace_back(std::move(item));
			return ret.second && ret.first->second + 1 == items.size();
		}

		void lock() {
			if (locked) {
				Logger::error("Failed to lock ", name, " registry - already locked!");
			} else {
				locked = true;
				if (log_lock) {
					Logger::info("Locked ", name, " registry after registering ", size(), " items");
				}
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

#define GETTERS(CONST) \
	value_type CONST* get_item_by_identifier(std::string_view identifier) CONST { \
		const typename decltype(identifier_index_map)::const_iterator it = identifier_index_map.find(identifier); \
		if (it != identifier_index_map.end()) { \
			return GetPointer(items[it->second]); \
		} \
		return nullptr; \
	} \
	value_type CONST* get_item_by_index(size_t index) CONST { \
		return index < items.size() ? &items[index] : nullptr; \
	} \
	NodeTools::callback_t<std::string_view> expect_item_str( \
		NodeTools::callback_t<value_type CONST&> callback, bool warn \
	) CONST { \
		return [this, callback, warn](std::string_view identifier) -> bool { \
			value_type CONST* item = get_item_by_identifier(identifier); \
			if (item != nullptr) { \
				return callback(*item); \
			} \
			if (!warn) { \
				Logger::error("Invalid ", name, ": ", identifier); \
				return false; \
			} else { \
				Logger::warning("Invalid ", name, ": ", identifier); \
				return true; \
			} \
		}; \
	} \
	NodeTools::node_callback_t expect_item_identifier(NodeTools::callback_t<value_type CONST&> callback, bool warn) CONST { \
		return NodeTools::expect_identifier(expect_item_str(callback, warn)); \
	} \
	NodeTools::node_callback_t expect_item_dictionary( \
		NodeTools::callback_t<value_type CONST&, ast::NodeCPtr> callback \
	) CONST { \
		return NodeTools::expect_dictionary([this, callback](std::string_view key, ast::NodeCPtr value) -> bool { \
			return expect_item_str(std::bind(callback, std::placeholders::_1, value), false)(key); \
		}); \
	}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4003)
#endif
		GETTERS()
		GETTERS(const)

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#undef GETTERS

		bool has_identifier(std::string_view identifier) const {
			return identifier_index_map.contains(identifier);
		}

		bool has_index(size_t index) const {
			return index < size();
		}

		REF_GETTERS(items)

		std::vector<std::string_view> get_item_identifiers() const {
			std::vector<std::string_view> identifiers(items.size());
			for (typename decltype(identifier_index_map)::value_type const& entry : identifier_index_map) {
				identifiers.push_back(entry.first);
			}
			return identifiers;
		}

		NodeTools::node_callback_t expect_item_decimal_map(
			NodeTools::callback_t<fixed_point_map_t<value_type const*>&&> callback
		) const {
			return [this, callback](ast::NodeCPtr node) -> bool {
				fixed_point_map_t<value_type const*> map;
				bool ret = expect_item_dictionary([&map](value_type const& key, ast::NodeCPtr value) -> bool {
					fixed_point_t val;
					const bool ret = NodeTools::expect_fixed_point(NodeTools::assign_variable_callback(val))(value);
					map.emplace(&key, std::move(val));
					return ret;
				})(node);
				ret &= callback(std::move(map));
				return ret;
			};
		}
	};

	/* Standard value storage */
	template<typename T>
	struct _addressof {
		constexpr T* operator()(T& item) const {
			return std::addressof(item);
		}
		constexpr T const* operator()(T const& item) const {
			return std::addressof(item);
		}
	};

	template<typename _Type, typename _GetIdentifier>
	using ValueRegistry = UniqueKeyRegistry<_Type, _Type, _GetIdentifier, _addressof<_Type>>;

	/* std::unique_ptr dynamic storage */
	template<typename T>
	struct _uptr_get {
		constexpr T* operator()(std::unique_ptr<T>& item) const {
			return item.get();
		}
		constexpr T const* operator()(std::unique_ptr<T> const& item) const {
			return item.get();
		}
	};

	template<typename _Type, typename _GetIdentifier>
	using InstanceRegistry = UniqueKeyRegistry<_Type, std::unique_ptr<_Type>, _GetIdentifier, _uptr_get<_Type>>;

	/* HasIdentifier versions */
	template<std::derived_from<HasIdentifier> T>
	struct _get_identifier {
		constexpr std::string_view operator()(T const* item) const {
			return item->get_identifier();
		}
	};

	template<std::derived_from<HasIdentifier> _Type>
	using IdentifierRegistry = ValueRegistry<_Type, _get_identifier<_Type>>;

	template<std::derived_from<HasIdentifier> _Type>
	using IdentifierInstanceRegistry = InstanceRegistry<_Type, _get_identifier<_Type>>;

#define IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(singular, plural) \
	void lock_##plural() { \
		plural.lock(); \
	} \
	bool plural##_are_locked() const { \
		return plural.is_locked(); \
	} \
	decltype(plural)::value_type const* get_##singular##_by_identifier(std::string_view identifier) const { \
		return plural.get_item_by_identifier(identifier); \
	} \
	bool has_##singular##_identifier(std::string_view identifier) const { \
		return plural.has_identifier(identifier); \
	} \
	size_t get_##singular##_count() const { \
		return plural.size(); \
	} \
	std::vector<decltype(plural)::storage_type> const& get_##plural() const { \
		return plural.get_items(); \
	} \
	std::vector<std::string_view> get_##singular##_identifiers() const { \
		return plural.get_item_identifiers(); \
	} \
	NodeTools::callback_t<std::string_view> expect_##singular##_str( \
		NodeTools::callback_t<decltype(plural)::value_type const&> callback, bool warn = false \
	) const { \
		return plural.expect_item_str(callback, warn); \
	} \
	NodeTools::node_callback_t expect_##singular##_identifier( \
		NodeTools::callback_t<decltype(plural)::value_type const&> callback, bool warn = false \
	) const { \
		return plural.expect_item_identifier(callback, warn); \
	} \
	NodeTools::node_callback_t expect_##singular##_dictionary( \
		NodeTools::callback_t<decltype(plural)::value_type const&, ast::NodeCPtr> callback \
	) const { \
		return plural.expect_item_dictionary(callback); \
	} \
	NodeTools::node_callback_t expect_##singular##_decimal_map( \
		NodeTools::callback_t<fixed_point_map_t<decltype(plural)::value_type const*>&&> callback \
	) const { \
		return plural.expect_item_decimal_map(callback); \
	}

#define IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_PLURAL(singular, plural) \
	decltype(plural)::value_type* get_##singular##_by_identifier(std::string_view identifier) { \
		return plural.get_item_by_identifier(identifier); \
	} \
	NodeTools::callback_t<std::string_view> expect_##singular##_str( \
		NodeTools::callback_t<decltype(plural)::value_type&> callback, bool warn = false \
	) { \
		return plural.expect_item_str(callback, warn); \
	} \
	NodeTools::node_callback_t expect_##singular##_identifier( \
		NodeTools::callback_t<decltype(plural)::value_type&> callback, bool warn = false \
	) { \
		return plural.expect_item_identifier(callback, warn); \
	} \
	NodeTools::node_callback_t expect_##singular##_dictionary( \
		NodeTools::callback_t<decltype(plural)::value_type&, ast::NodeCPtr> callback \
	) { \
		return plural.expect_item_dictionary(callback); \
	}

#define IDENTIFIER_REGISTRY_ACCESSORS(name) IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(name, name##s)
#define IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(name) IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_PLURAL(name, name##s)
}

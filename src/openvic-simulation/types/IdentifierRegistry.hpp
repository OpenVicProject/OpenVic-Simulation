#pragma once

#include <vector>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Logger.hpp"

namespace OpenVic {
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
	static constexpr bool duplicate_ignore_callback(std::string_view registry_name, std::string_view duplicate_identifier) {
		return true;
	}

	/* Registry Value Info - the type that is being registered, and a unique identifier string getter. */
	template<typename ValueInfo>
	concept RegistryValueInfo = requires(typename ValueInfo::value_type const& item) {
		{ ValueInfo::get_identifier(item) } -> std::same_as<std::string_view>;
	};
	template <std::derived_from<HasIdentifier> Value>
	struct RegistryValueInfoHasIdentifier {
		using value_type = Value;

		static constexpr std::string_view get_identifier(value_type const& item) {
			return item.get_identifier();
		}
	};

	/* Registry Item Info - how individual elements of the registered type are stored, and type from item getters. */
	template<template<typename> typename ItemInfo, typename Value>
	concept RegistryItemInfo = requires(
		typename ItemInfo<Value>::item_type& item, typename ItemInfo<Value>::item_type const& const_item
	) {
		{ ItemInfo<Value>::get_value(item) } -> std::same_as<Value&>;
		{ ItemInfo<Value>::get_value(const_item) } -> std::same_as<Value const&>;
	};
	template<typename Value>
	struct RegistryItemInfoValue {
		using item_type = Value;

		static constexpr Value& get_value(item_type& item) {
			return item;
		}
		static constexpr Value const& get_value(item_type const& item) {
			return item;
		}
	};
	template<typename Value>
	struct RegistryItemInfoInstance {
		using item_type = std::unique_ptr<Value>;

		static constexpr Value& get_value(item_type& item) {
			return *item.get();
		}
		static constexpr Value const& get_value(item_type const& item) {
			return *item.get();
		}
	};

	/* Registry Storage Info - how items are stored and indexed, and item-index conversion functions. */
	template<template<typename> typename StorageInfo, typename Item>
	concept RegistryStorageInfo =
		std::same_as<typename StorageInfo<Item>::storage_type::value_type, Item> &&
		requires(
			typename StorageInfo<Item>::storage_type& items, typename StorageInfo<Item>::storage_type const& const_items,
			typename StorageInfo<Item>::index_type index
		) {
			{ StorageInfo<Item>::get_back_index(items) } -> std::same_as<typename StorageInfo<Item>::index_type>;
			{ StorageInfo<Item>::get_item_from_index(items, index) } -> std::same_as<Item&>;
			{ StorageInfo<Item>::get_item_from_index(const_items, index) } -> std::same_as<Item const&>;
		};
	template<typename Item>
	struct RegistryStorageInfoVector {
		using storage_type = std::vector<Item>;
		using index_type = std::size_t;

		static constexpr index_type get_back_index(storage_type& items) {
			return items.size() - 1;
		}
		static constexpr Item& get_item_from_index(storage_type& items, index_type index) {
			return items[index];
		}
		static constexpr Item const& get_item_from_index(storage_type const& items, index_type index) {
			return items[index];
		}
	};
	template<typename Item>
	struct RegistryStorageInfoDeque {
		using storage_type = std::deque<Item>;
		using index_type = Item*;

		static constexpr index_type get_back_index(storage_type& items) {
			return std::addressof(items.back());
		}
		static constexpr Item& get_item_from_index(storage_type& items, index_type index) {
			return *index;
		}
		static constexpr Item const& get_item_from_index(storage_type const& items, index_type index) {
			return *index;
		}
	};

	/* Registry Identifier Map Info - how unique identifier strings are compared when looking up entries. */
	template<typename IdentifierMapInfo>
	concept RegistryIdentifierMapInfo = requires(std::string_view identifier) {
		{ typename IdentifierMapInfo::hash {}(identifier) } -> std::same_as<std::size_t>;
		{ typename IdentifierMapInfo::equal {}(identifier, identifier) } -> std::same_as<bool>;
	};
	struct RegistryIdentifierMapInfoCaseSensitive {
		using hash = container_hash<std::string>;
		using equal = std::equal_to<>;
	};
	struct RegistryIdentifierMapInfoCaseInsensitive {
		using hash = case_insensitive_string_hash;
		using equal = case_insensitive_string_equal;
	};

	template<
		RegistryValueInfo ValueInfo, /* The type that is being registered and that has unique string identifiers */
		template<typename> typename _ItemInfo, /* How the type is being stored, usually either by value or std::unique_ptr */
		template<typename> typename _StorageInfo = RegistryStorageInfoVector, /* How items are stored, including indexing type */
		RegistryIdentifierMapInfo IdentifierMapInfo = RegistryIdentifierMapInfoCaseSensitive /* Identifier map parameters */
	>
	requires (
		RegistryItemInfo<_ItemInfo, typename ValueInfo::value_type> &&
		RegistryStorageInfo<_StorageInfo, typename _ItemInfo<typename ValueInfo::value_type>::item_type>
	)
	class UniqueKeyRegistry {
	public:
		using value_type = typename ValueInfo::value_type;
		using ItemInfo = _ItemInfo<value_type>;
		using item_type = typename ItemInfo::item_type;

	private:
		using StorageInfo = _StorageInfo<item_type>;
		using index_type = typename StorageInfo::index_type;
		using identifier_index_map_t =
			string_map_t<index_type, typename IdentifierMapInfo::hash, typename IdentifierMapInfo::equal>;

	public:
		using storage_type = typename StorageInfo::storage_type;

	private:
		const std::string name;
		const bool log_lock;
		storage_type PROPERTY_REF(items);
		bool locked = false;
		identifier_index_map_t identifier_index_map;

	public:
		constexpr UniqueKeyRegistry(std::string_view new_name, bool new_log_lock = true)
			: name { new_name }, log_lock { new_log_lock } {}

		constexpr std::string_view get_name() const {
			return name;
		}

		constexpr bool add_item(
			item_type&& item, NodeTools::Callback<std::string_view, std::string_view> auto duplicate_callback
		) {
			if (locked) {
				Logger::error("Cannot add item to the ", name, " registry - locked!");
				return false;
			}

			const std::string_view new_identifier = ValueInfo::get_identifier(ItemInfo::get_value(item));
			value_type const* old_item = get_item_by_identifier(new_identifier);
			if (old_item != nullptr) {
				return duplicate_callback(name, new_identifier);
			}

			items.emplace_back(std::move(item));
			identifier_index_map.emplace(std::move(new_identifier), StorageInfo::get_back_index(items));
			return true;
		}

		constexpr bool add_item(item_type&& item) {
			return add_item(std::move(item), duplicate_fail_callback);
		}

		constexpr void lock() {
			if (locked) {
				Logger::error("Failed to lock ", name, " registry - already locked!");
			} else {
				locked = true;
				if (log_lock) {
					Logger::info("Locked ", name, " registry after registering ", size(), " items");
				}
			}
		}

		constexpr bool is_locked() const {
			return locked;
		}

		constexpr void reset() {
			identifier_index_map.clear();
			items.clear();
			locked = false;
		}

		constexpr std::size_t size() const {
			return items.size();
		}

		constexpr bool empty() const {
			return items.empty();
		}

		constexpr void reserve(std::size_t size) {
			if (locked) {
				Logger::error("Failed to reserve space for ", size, " items in ", name, " registry - already locked!");
			} else {
				items.reserve(size);
				identifier_index_map.reserve(size);
			}
		}

		static constexpr NodeTools::KeyValueCallback auto key_value_invalid_callback(std::string_view name) {
			return [name](std::string_view key, ast::NodeCPtr) {
				Logger::error("Invalid ", name, ": ", key);
				return false;
			};
		}

#define GETTERS(CONST) \
	constexpr value_type CONST* get_item_by_identifier(std::string_view identifier) CONST { \
		const typename decltype(identifier_index_map)::const_iterator it = identifier_index_map.find(identifier); \
		if (it != identifier_index_map.end()) { \
			return std::addressof(ItemInfo::get_value(StorageInfo::get_item_from_index(items, it->second))); \
		} \
		return nullptr; \
	} \
	constexpr value_type CONST* get_item_by_index(std::size_t index) CONST { \
		return index < items.size() ? std::addressof(ItemInfo::get_value(items[index])) : nullptr; \
	} \
	constexpr NodeTools::Callback<std::string_view> auto expect_item_str( \
		NodeTools::Callback<value_type CONST&> auto callback, bool warn \
	) CONST { \
		return [this, callback, warn](std::string_view identifier) -> bool { \
			value_type CONST* item = get_item_by_identifier(identifier); \
			if (item != nullptr) { \
				return callback(*item); \
			} \
			return NodeTools::warn_or_error(warn, "Invalid ", name, ": ", identifier); \
		}; \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_identifier( \
		NodeTools::Callback<value_type CONST&> auto callback, bool warn \
	) CONST { \
		return NodeTools::expect_identifier(expect_item_str(callback, warn)); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_assign_and_default( \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return NodeTools::expect_assign( \
			[this, default_callback, callback](std::string_view key, ast::NodeCPtr value) -> bool { \
				value_type CONST* item = get_item_by_identifier(key); \
				if (item != nullptr) { \
					return callback(*item, value); \
				} else {\
					return default_callback(key, value); \
				} \
			} \
		); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_assign( \
		NodeTools::Callback<value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_assign_and_default(key_value_invalid_callback(name), callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary_and_length_and_default( \
		NodeTools::LengthCallback auto length_callback, \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return NodeTools::expect_list_and_length( \
			length_callback, expect_item_assign_and_default(default_callback, callback) \
		); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary_and_length( \
		NodeTools::LengthCallback auto length_callback, \
		NodeTools::Callback<value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_dictionary_and_length_and_default( \
			length_callback, \
			key_value_invalid_callback(name), \
			callback \
		); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary_and_default( \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_dictionary_and_length_and_default( \
			NodeTools::default_length_callback, \
			default_callback, \
			callback \
		); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary( \
		NodeTools::Callback<value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_dictionary_and_length_and_default( \
			NodeTools::default_length_callback, \
			key_value_invalid_callback(name), \
			callback \
		); \
	} \
	template<NodeTools::Reservable T> \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary_reserve_length_and_default( \
		T& t, \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_dictionary_and_length_and_default( \
			NodeTools::reserve_length_callback(t), \
			default_callback, \
			callback \
		); \
	} \
	template<NodeTools::Reservable T> \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary_reserve_length( \
		T& t, \
		NodeTools::Callback<value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_dictionary_and_length_and_default( \
			NodeTools::reserve_length_callback(t), \
			key_value_invalid_callback(name), \
			callback \
		); \
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

		constexpr bool has_identifier(std::string_view identifier) const {
			return identifier_index_map.contains(identifier);
		}

		constexpr bool has_index(std::size_t index) const {
			return index < size();
		}

		std::vector<std::string_view> get_item_identifiers() const {
			std::vector<std::string_view> identifiers;
			identifiers.reserve(items.size());
			for (typename identifier_index_map_t::value_type const& entry : identifier_index_map) {
				identifiers.push_back(entry.first);
			}
			return identifiers;
		}

		constexpr NodeTools::NodeCallback auto expect_item_decimal_map(
			NodeTools::Callback<fixed_point_map_t<value_type const*>&&> auto callback
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

	/* Item Specialisations */
	template<
		RegistryValueInfo ValueInfo, template<typename> typename StorageInfo = RegistryStorageInfoVector,
		RegistryIdentifierMapInfo IdentifierMapInfo = RegistryIdentifierMapInfoCaseSensitive
	>
	requires RegistryStorageInfo<StorageInfo, typename RegistryItemInfoValue<typename ValueInfo::value_type>::item_type>
	using ValueRegistry = UniqueKeyRegistry<ValueInfo, RegistryItemInfoValue, StorageInfo, IdentifierMapInfo>;

	template<
		RegistryValueInfo ValueInfo, template<typename> typename StorageInfo = RegistryStorageInfoVector,
		RegistryIdentifierMapInfo IdentifierMapInfo = RegistryIdentifierMapInfoCaseSensitive
	>
	requires RegistryStorageInfo<StorageInfo, typename RegistryItemInfoInstance<typename ValueInfo::value_type>::item_type>
	using InstanceRegistry = UniqueKeyRegistry<ValueInfo, RegistryItemInfoInstance, StorageInfo, IdentifierMapInfo>;

	/* HasIdentifier Specialisations */
	template<
		std::derived_from<HasIdentifier> Value, template<typename> typename StorageInfo = RegistryStorageInfoVector,
		RegistryIdentifierMapInfo IdentifierMapInfo = RegistryIdentifierMapInfoCaseSensitive
	>
	using IdentifierRegistry = ValueRegistry<RegistryValueInfoHasIdentifier<Value>, StorageInfo, IdentifierMapInfo>;

	template<
		std::derived_from<HasIdentifier> Value, template<typename> typename StorageInfo = RegistryStorageInfoVector,
		RegistryIdentifierMapInfo IdentifierMapInfo = RegistryIdentifierMapInfoCaseSensitive
	>
	using IdentifierInstanceRegistry = InstanceRegistry<RegistryValueInfoHasIdentifier<Value>, StorageInfo, IdentifierMapInfo>;

	/* Case-Insensitive HasIdentifier Specialisations */
	template<std::derived_from<HasIdentifier> Value, template<typename> typename StorageInfo = RegistryStorageInfoVector>
	using CaseInsensitiveIdentifierRegistry =
		IdentifierRegistry<Value, StorageInfo, RegistryIdentifierMapInfoCaseInsensitive>;

	template<std::derived_from<HasIdentifier> Value, template<typename> typename StorageInfo = RegistryStorageInfoVector>
	using CaseInsensitiveIdentifierInstanceRegistry =
		IdentifierInstanceRegistry<Value, StorageInfo, RegistryIdentifierMapInfoCaseInsensitive>;

/* Macros to generate declaration and constant accessor methods for a UniqueKeyRegistry member variable. */

#define IDENTIFIER_REGISTRY(name) \
	IDENTIFIER_REGISTRY_CUSTOM_PLURAL(name, name##s)

#define IDENTIFIER_REGISTRY_CUSTOM_PLURAL(singular, plural) \
	IDENTIFIER_REGISTRY_FULL_CUSTOM(singular, plural, plural, plural, 0)

#define IDENTIFIER_REGISTRY_CUSTOM_INDEX_OFFSET(name, index_offset) \
	IDENTIFIER_REGISTRY_FULL_CUSTOM(name, name##s, name##s, name##s, index_offset)

#define IDENTIFIER_REGISTRY_FULL_CUSTOM(singular, plural, registry, debug_name, index_offset) \
	registry { #debug_name };\
public: \
	constexpr void lock_##plural() { \
		registry.lock(); \
	} \
	constexpr bool plural##_are_locked() const { \
		return registry.is_locked(); \
	} \
	constexpr bool has_##singular##_identifier(std::string_view identifier) const { \
		return registry.has_identifier(identifier); \
	} \
	constexpr std::size_t get_##singular##_count() const { \
		return registry.size(); \
	} \
	constexpr bool plural##_empty() const { \
		return registry.empty(); \
	} \
	std::vector<std::string_view> get_##singular##_identifiers() const { \
		return registry.get_item_identifiers(); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_decimal_map( \
		NodeTools::Callback<fixed_point_map_t<decltype(registry)::value_type const*>&&> auto callback \
	) const { \
		return registry.expect_item_decimal_map(callback); \
	} \
	IDENTIFIER_REGISTRY_INTERNAL_SHARED(singular, plural, registry, index_offset, const) \
private:

/* Macros to generate non-constant accessor methods for a UniqueKeyRegistry member variable. */

#define IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(name) \
	IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_PLURAL(name, name##s)

#define IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_PLURAL(singular, plural) \
	IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_FULL_CUSTOM(singular, plural, plural, plural, 0)

#define IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_INDEX_OFFSET(name, index_offset) \
	IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_FULL_CUSTOM(name, name##s, name##s, name##s, index_offset)

#define IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_FULL_CUSTOM(singular, plural, registry, debug_name, index_offset) \
	IDENTIFIER_REGISTRY_INTERNAL_SHARED(singular, plural, registry, index_offset,)

#define IDENTIFIER_REGISTRY_INTERNAL_SHARED(singular, plural, registry, index_offset, const_kw) \
	constexpr decltype(registry)::value_type const_kw* get_##singular##_by_identifier(std::string_view identifier) const_kw { \
		return registry.get_item_by_identifier(identifier); \
	} \
	constexpr decltype(registry)::value_type const_kw* get_##singular##_by_index(std::size_t index) const_kw { \
		return index >= index_offset ? registry.get_item_by_index(index - index_offset) : nullptr; \
	} \
	constexpr decltype(registry)::storage_type const_kw& get_##plural() const_kw { \
		return registry.get_items(); \
	} \
	constexpr NodeTools::Callback<std::string_view> auto expect_##singular##_str( \
		NodeTools::Callback<decltype(registry)::value_type const_kw&> auto callback, bool warn = false \
	) const_kw { \
		return registry.expect_item_str(callback, warn); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_identifier( \
		NodeTools::Callback<decltype(registry)::value_type const_kw&> auto callback, bool warn = false \
	) const_kw { \
		return registry.expect_item_identifier(callback, warn); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_assign_and_default( \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<decltype(registry)::value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_assign_and_default(default_callback, callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_assign( \
		NodeTools::Callback<decltype(registry)::value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_assign(callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_dictionary_and_length_and_default( \
		NodeTools::LengthCallback auto length_callback, \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<decltype(registry)::value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_dictionary_and_length_and_default(length_callback, default_callback, callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_dictionary_and_default( \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<decltype(registry)::value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_dictionary_and_default(default_callback, callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_dictionary( \
		NodeTools::Callback<decltype(registry)::value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_dictionary(callback); \
	} \
	template<NodeTools::Reservable T> \
	constexpr NodeTools::NodeCallback auto expect_##singular##_dictionary_reserve_length_and_default( \
		T& t, \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<decltype(registry)::value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_dictionary_reserve_length_and_default(t, default_callback, callback); \
	} \
	template<NodeTools::Reservable T> \
	constexpr NodeTools::NodeCallback auto expect_##singular##_dictionary_reserve_length( \
		T& t, \
		NodeTools::Callback<decltype(registry)::value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_dictionary_reserve_length(t, callback); \
	}
}

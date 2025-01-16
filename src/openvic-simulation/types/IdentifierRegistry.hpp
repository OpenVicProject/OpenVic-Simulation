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
	concept RegistryValueInfo = requires(
		typename ValueInfo::internal_value_type& item, typename ValueInfo::internal_value_type const& const_item
	) {
		{ ValueInfo::get_identifier(item) } -> std::same_as<std::string_view>;
		{ ValueInfo::get_external_value(item) } -> std::same_as<typename ValueInfo::external_value_type&>;
		{ ValueInfo::get_external_value(const_item) } -> std::same_as<typename ValueInfo::external_value_type const&>;
	};
	template<HasGetIdentifier Value>
	struct RegistryValueInfoHasGetIdentifier {
		using internal_value_type = Value;
		using external_value_type = Value;

		static constexpr std::string_view get_identifier(internal_value_type const& item) {
			return item.get_identifier();
		}
		static constexpr external_value_type& get_external_value(internal_value_type& item) {
			return item;
		}
		static constexpr external_value_type const& get_external_value(internal_value_type const& item) {
			return item;
		}
	};
	template<RegistryValueInfo ValueInfo>
	struct RegistryValueInfoPointer {
		using internal_value_type = typename ValueInfo::internal_value_type*;
		using external_value_type = typename ValueInfo::external_value_type;

		static constexpr std::string_view get_identifier(internal_value_type const& item) {
			return ValueInfo::get_identifier(*item);
		}
		static constexpr external_value_type& get_external_value(internal_value_type& item) {
			return ValueInfo::get_external_value(*item);
		}
		static constexpr external_value_type const& get_external_value(internal_value_type const& item) {
			return ValueInfo::get_external_value(*item);
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

	template<
		RegistryValueInfo ValueInfo, /* The type that is being registered and that has unique string identifiers */
		template<typename> typename _ItemInfo, /* How the type is being stored, usually either by value or std::unique_ptr */
		template<typename> typename _StorageInfo = RegistryStorageInfoVector, /* How items are stored, including indexing type */
		StringMapCase Case = StringMapCaseSensitive /* Identifier map parameters */
	>
	requires(
		RegistryItemInfo<_ItemInfo, typename ValueInfo::internal_value_type> &&
		RegistryStorageInfo<_StorageInfo, typename _ItemInfo<typename ValueInfo::internal_value_type>::item_type>
	)
	class UniqueKeyRegistry {
	public:
		using internal_value_type = typename ValueInfo::internal_value_type;
		using external_value_type = typename ValueInfo::external_value_type;
		using ItemInfo = _ItemInfo<internal_value_type>;
		using item_type = typename ItemInfo::item_type;

	private:
		using StorageInfo = _StorageInfo<item_type>;
		using index_type = typename StorageInfo::index_type;
		using identifier_index_map_t = template_string_map_t<index_type, Case>;

	public:
		using storage_type = typename StorageInfo::storage_type;
		static constexpr bool storage_type_reservable = Reservable<storage_type>;

	private:
		std::string PROPERTY(name);
		const bool log_lock;
		storage_type PROPERTY_REF(items);
		bool PROPERTY_CUSTOM_PREFIX(locked, is);
		identifier_index_map_t identifier_index_map;

	public:
		constexpr UniqueKeyRegistry(std::string_view new_name, bool new_log_lock = true)
			: name { new_name }, log_lock { new_log_lock }, locked { false } {}

		constexpr bool add_item(
			item_type&& item, NodeTools::Callback<std::string_view, std::string_view> auto duplicate_callback
		) {
			if (locked) {
				Logger::error("Cannot add item to the ", name, " registry - locked!");
				return false;
			}

			const std::string_view new_identifier = ValueInfo::get_identifier(ItemInfo::get_value(item));
			if (has_identifier(new_identifier)) {
				return duplicate_callback(name, new_identifier);
			}

			items.emplace_back(std::move(item));

			const index_type index = StorageInfo::get_back_index(items);

			/* Get item's identifier via index rather than using new_identifier, as it may have been invalidated item's move. */
			identifier_index_map.emplace(
				ValueInfo::get_identifier(ItemInfo::get_value(StorageInfo::get_item_from_index(items, index))),
				StorageInfo::get_back_index(items)
			);

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
			if constexpr (storage_type_reservable) {
				if (locked) {
					Logger::error("Failed to reserve space for ", size, " items in ", name, " registry - already locked!");
				} else {
					items.reserve(size);
					identifier_index_map.reserve(size);
				}
			} else {
				Logger::error("Cannot reserve space for ", size, " ", name, " - storage_type not reservable!");
			}
		}

		constexpr void reserve_more(std::size_t size) {
			OpenVic::reserve_more(*this, size);
		}

		static constexpr NodeTools::KeyValueCallback auto key_value_invalid_callback(std::string_view name) {
			return [name](std::string_view key, ast::NodeCPtr) {
				Logger::error("Invalid ", name, ": ", key);
				return false;
			};
		}

#define GETTERS(CONST) \
	constexpr external_value_type CONST* get_item_by_identifier(std::string_view identifier) CONST { \
		const typename decltype(identifier_index_map)::const_iterator it = identifier_index_map.find(identifier); \
		if (it != identifier_index_map.end()) { \
			return std::addressof( \
				ValueInfo::get_external_value(ItemInfo::get_value(StorageInfo::get_item_from_index(items, it->second))) \
			); \
		} \
		return nullptr; \
	} \
	template<std::derived_from<external_value_type> T> \
	requires requires(external_value_type const& value) { \
		{ value.get_type() } -> std::same_as<std::string_view>; \
		{ T::get_type_static() } -> std::same_as<std::string_view>; \
	} \
	constexpr T CONST* get_cast_item_by_identifier(std::string_view identifier) CONST { \
		external_value_type CONST* item = get_item_by_identifier(identifier); \
		if (item != nullptr) { \
			if (item->get_type() == T::get_type_static()) { \
				return reinterpret_cast<T CONST*>(item); \
			} \
			Logger::error( \
				"Invalid type for item \"", identifier, "\": ", item->get_type(), " (expected ", T::get_type_static(), ")" \
			); \
		} \
		return nullptr; \
	} \
	constexpr external_value_type CONST* get_item_by_index(std::size_t index) CONST { \
		if (index < items.size()) { \
			return std::addressof(ValueInfo::get_external_value(ItemInfo::get_value(items[index]))); \
		} else { \
			return nullptr; \
		} \
	} \
	constexpr NodeTools::Callback<std::string_view> auto expect_item_str( \
		NodeTools::Callback<external_value_type CONST&> auto callback, bool allow_empty, bool warn \
	) CONST { \
		return [this, callback, allow_empty, warn](std::string_view identifier) -> bool { \
			if (identifier.empty()) { \
				if (allow_empty) { \
					return true; \
				} else { \
					Logger::warn_or_error(warn, "Invalid ", name, " identifier: empty!"); \
					return warn; \
				} \
			} \
			external_value_type CONST* item = get_item_by_identifier(identifier); \
			if (item != nullptr) { \
				return callback(*item); \
			} \
			Logger::warn_or_error(warn, "Invalid ", name, " identifier: ", identifier); \
			return warn; \
		}; \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_identifier( \
		NodeTools::Callback<external_value_type CONST&> auto callback, bool warn \
	) CONST { \
		return NodeTools::expect_identifier(expect_item_str(callback, false, warn)); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_string( \
		NodeTools::Callback<external_value_type CONST&> auto callback, bool allow_empty, bool warn \
	) CONST { \
		return NodeTools::expect_string(expect_item_str(callback, allow_empty, warn), allow_empty); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_identifier_or_string( \
		NodeTools::Callback<external_value_type CONST&> auto callback, bool allow_empty, bool warn \
	) CONST { \
		return NodeTools::expect_identifier_or_string(expect_item_str(callback, allow_empty, warn), allow_empty); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_assign_and_default( \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<external_value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return NodeTools::expect_assign( \
			[this, default_callback, callback](std::string_view key, ast::NodeCPtr value) -> bool { \
				external_value_type CONST* item = get_item_by_identifier(key); \
				if (item != nullptr) { \
					return callback(*item, value); \
				} else { \
					return default_callback(key, value); \
				} \
			} \
		); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_assign( \
		NodeTools::Callback<external_value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_assign_and_default(key_value_invalid_callback(name), callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary_and_length_and_default( \
		NodeTools::LengthCallback auto length_callback, \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<external_value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return NodeTools::expect_list_and_length( \
			length_callback, expect_item_assign_and_default(default_callback, callback) \
		); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary_and_length( \
		NodeTools::LengthCallback auto length_callback, \
		NodeTools::Callback<external_value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_dictionary_and_length_and_default( \
			length_callback, \
			key_value_invalid_callback(name), \
			callback \
		); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary_and_default( \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<external_value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_dictionary_and_length_and_default( \
			NodeTools::default_length_callback, \
			default_callback, \
			callback \
		); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary( \
		NodeTools::Callback<external_value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_dictionary_and_length_and_default( \
			NodeTools::default_length_callback, \
			key_value_invalid_callback(name), \
			callback \
		); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary_reserve_length_and_default( \
		Reservable auto& reservable, \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<external_value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_dictionary_and_length_and_default( \
			NodeTools::reserve_length_callback(reservable), \
			default_callback, \
			callback \
		); \
	} \
	constexpr NodeTools::NodeCallback auto expect_item_dictionary_reserve_length( \
		Reservable auto& reservable, \
		NodeTools::Callback<external_value_type CONST&, ast::NodeCPtr> auto callback \
	) CONST { \
		return expect_item_dictionary_and_length_and_default( \
			NodeTools::reserve_length_callback(reservable), \
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

		/* Parses a dictionary with item keys and decimal number values (in the form of fixed point values),
		 * with the resulting map move-returned via `callback`. The values can be transformed by providing
		 * a fixed point to fixed point function fixed_point_functor, which will be applied to ever parsed value. */
		template<NodeTools::FunctorConvertible<fixed_point_t, fixed_point_t> FixedPointFunctor = std::identity>
		constexpr NodeTools::NodeCallback auto expect_item_decimal_map(
			NodeTools::Callback<fixed_point_map_t<external_value_type const*>&&> auto callback,
			FixedPointFunctor fixed_point_functor = {}
		) const {
			return [this, callback, fixed_point_functor](ast::NodeCPtr node) -> bool {
				fixed_point_map_t<external_value_type const*> map;

				bool ret = expect_item_dictionary(
					[&map, fixed_point_functor](external_value_type const& key, ast::NodeCPtr value) -> bool {
						return NodeTools::expect_fixed_point(
							[&map, fixed_point_functor, &key](fixed_point_t val) -> bool {
								map.emplace(&key, fixed_point_functor(val));
								return true;
							}
						)(value);
					}
				)(node);

				ret &= callback(std::move(map));

				return ret;
			};
		}
	};

	/* Item Specialisations */
	template<
		RegistryValueInfo ValueInfo, template<typename> typename StorageInfo = RegistryStorageInfoVector,
		StringMapCase Case = StringMapCaseSensitive
	>
	requires
		RegistryStorageInfo<StorageInfo, typename RegistryItemInfoValue<typename ValueInfo::internal_value_type>::item_type>
	using ValueRegistry = UniqueKeyRegistry<ValueInfo, RegistryItemInfoValue, StorageInfo, Case>;

	template<
		RegistryValueInfo ValueInfo, template<typename> typename StorageInfo = RegistryStorageInfoVector,
		StringMapCase Case = StringMapCaseSensitive
	>
	requires
		RegistryStorageInfo<StorageInfo, typename RegistryItemInfoInstance<typename ValueInfo::internal_value_type>::item_type>
	using InstanceRegistry = UniqueKeyRegistry<ValueInfo, RegistryItemInfoInstance, StorageInfo, Case>;

	/* HasGetIdentifier Specialisations */
	template<
		HasGetIdentifier Value, template<typename> typename StorageInfo = RegistryStorageInfoVector,
		StringMapCase Case = StringMapCaseSensitive
	>
	using IdentifierRegistry = ValueRegistry<RegistryValueInfoHasGetIdentifier<Value>, StorageInfo, Case>;

	template<
		HasGetIdentifier Value, template<typename> typename StorageInfo = RegistryStorageInfoVector,
		StringMapCase Case = StringMapCaseSensitive
	>
	using IdentifierPointerRegistry =
		ValueRegistry<RegistryValueInfoPointer<RegistryValueInfoHasGetIdentifier<Value>>, StorageInfo, Case>;

	template<
		HasGetIdentifier Value, template<typename> typename StorageInfo = RegistryStorageInfoVector,
		StringMapCase Case = StringMapCaseSensitive
	>
	using IdentifierInstanceRegistry = InstanceRegistry<RegistryValueInfoHasGetIdentifier<Value>, StorageInfo, Case>;

	/* Case-Insensitive HasGetIdentifier Specialisations */
	template<HasGetIdentifier Value, template<typename> typename StorageInfo = RegistryStorageInfoVector>
	using CaseInsensitiveIdentifierRegistry = IdentifierRegistry<Value, StorageInfo, StringMapCaseInsensitive>;

	template<HasGetIdentifier Value, template<typename> typename StorageInfo = RegistryStorageInfoVector>
	using CaseInsensitiveIdentifierPointerRegistry = IdentifierPointerRegistry<Value, StorageInfo, StringMapCaseInsensitive>;

	template<HasGetIdentifier Value, template<typename> typename StorageInfo = RegistryStorageInfoVector>
	using CaseInsensitiveIdentifierInstanceRegistry = IdentifierInstanceRegistry<Value, StorageInfo, StringMapCaseInsensitive>;

/* Macros to generate declaration and constant accessor methods for a UniqueKeyRegistry member variable. */

#define IDENTIFIER_REGISTRY(name) \
	IDENTIFIER_REGISTRY_CUSTOM_PLURAL(name, name##s)

#define IDENTIFIER_REGISTRY_CUSTOM_PLURAL(singular, plural) \
	IDENTIFIER_REGISTRY_FULL_CUSTOM(singular, plural, plural, plural, 0)

#define IDENTIFIER_REGISTRY_CUSTOM_INDEX_OFFSET(name, index_offset) \
	IDENTIFIER_REGISTRY_FULL_CUSTOM(name, name##s, name##s, name##s, index_offset)

#define IDENTIFIER_REGISTRY_FULL_CUSTOM(singular, plural, registry, debug_name, index_offset) \
	registry { #debug_name }; \
public: \
	constexpr void lock_##plural() { \
		registry.lock(); \
	} \
	constexpr bool plural##_are_locked() const { \
		return registry.is_locked(); \
	} \
	template<typename = void> \
	constexpr void reserve_##plural(size_t size) requires(decltype(registry)::storage_type_reservable) { \
		registry.reserve(size); \
	} \
	template<typename = void> \
	constexpr void reserve_more_##plural(size_t size) requires(decltype(registry)::storage_type_reservable) { \
		registry.reserve_more(size); \
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
	template<NodeTools::FunctorConvertible<fixed_point_t, fixed_point_t> FixedPointFunctor = std::identity> \
	constexpr NodeTools::NodeCallback auto expect_##singular##_decimal_map( \
		NodeTools::Callback<fixed_point_map_t<decltype(registry)::external_value_type const*>&&> auto callback, \
		FixedPointFunctor fixed_point_functor = {} \
	) const { \
		return registry.expect_item_decimal_map(callback, fixed_point_functor); \
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
	constexpr decltype(registry)::external_value_type const_kw* get_##singular##_by_identifier(std::string_view identifier) const_kw { \
		return registry.get_item_by_identifier(identifier); \
	} \
	template<std::derived_from<decltype(registry)::external_value_type> T> \
	constexpr T const_kw* get_cast_##singular##_by_identifier(std::string_view identifier) const_kw { \
		return registry.get_cast_item_by_identifier<T>(identifier); \
	} \
	constexpr decltype(registry)::external_value_type const_kw* get_##singular##_by_index(std::size_t index) const_kw { \
		return index >= index_offset ? registry.get_item_by_index(index - index_offset) : nullptr; \
	} \
	constexpr decltype(registry)::storage_type const_kw& get_##plural() const_kw { \
		return registry.get_items(); \
	} \
	constexpr NodeTools::Callback<std::string_view> auto expect_##singular##_str( \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&> auto callback, bool allow_empty = false, \
		bool warn = false \
	) const_kw { \
		return registry.expect_item_str(callback, allow_empty, warn); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_identifier( \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&> auto callback, bool warn = false \
	) const_kw { \
		return registry.expect_item_identifier(callback, warn); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_string( \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&> auto callback, bool allow_empty = false, \
		bool warn = false \
	) const_kw { \
		return registry.expect_item_string(callback, allow_empty, warn); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_identifier_or_string( \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&> auto callback, bool allow_empty = false, \
		bool warn = false \
	) const_kw { \
		return registry.expect_item_identifier_or_string(callback, allow_empty, warn); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_assign_and_default( \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_assign_and_default(default_callback, callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_assign( \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_assign(callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_dictionary_and_length_and_default( \
		NodeTools::LengthCallback auto length_callback, \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_dictionary_and_length_and_default(length_callback, default_callback, callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_dictionary_and_default( \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_dictionary_and_default(default_callback, callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_dictionary( \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_dictionary(callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_dictionary_reserve_length_and_default( \
		Reservable auto& reservable, \
		NodeTools::KeyValueCallback auto default_callback, \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_dictionary_reserve_length_and_default(reservable, default_callback, callback); \
	} \
	constexpr NodeTools::NodeCallback auto expect_##singular##_dictionary_reserve_length( \
		Reservable auto& reservable, \
		NodeTools::Callback<decltype(registry)::external_value_type const_kw&, ast::NodeCPtr> auto callback \
	) const_kw { \
		return registry.expect_item_dictionary_reserve_length(reservable, callback); \
	}
}

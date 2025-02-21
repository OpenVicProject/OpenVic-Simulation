#pragma once

#include <any>
#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <variant>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/types/FunctionRef.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct InstanceManager;

	struct DiplomaticActionType {
		friend struct DiplomaticActionManager;
		friend struct CancelableDiplomaticActionType;

		struct Argument {
			InstanceManager& instance_manager;
			CountryInstance* sender;
			CountryInstance* reciever;
			std::any context_data;
		};

		using allowed_to_commit_func = FunctionRef<bool(Argument const&)>;
		using get_acceptance_func = FunctionRef<std::int32_t(Argument const&)>;
		using commit_action_func = FunctionRef<void(Argument&)>;

		static bool allowed_to_commit_default(Argument const& argument) {
			return true;
		}

		static std::int32_t get_acceptance_default(Argument const& argument) {
			return 1;
		}

		struct Initializer {
			CountryRelationManager::influence_value_type influence_cost = 0;
			commit_action_func commit;
			allowed_to_commit_func allowed = allowed_to_commit_default;
			get_acceptance_func get_acceptance = get_acceptance_default;
		};

		const CountryRelationManager::influence_value_type influence_cost = 0;
		const commit_action_func commit_action_caller;
		const allowed_to_commit_func allowed_to_commit = allowed_to_commit_default;
		const get_acceptance_func get_acceptance = get_acceptance_default;

		void commit_action(Argument& arg) const {
			commit_action_caller(arg);
		}

	private:
		DiplomaticActionType(Initializer&& initializer);
	};

	struct CancelableDiplomaticActionType : DiplomaticActionType {
		friend struct DiplomaticActionManager;

		using allowed_to_cancel_func = FunctionRef<bool(Argument const&)>;

		static bool allowed_to_cancel_default(Argument const& argument) {
			return true;
		}

		struct Initializer {
			CountryRelationManager::influence_value_type influence_cost = 0;
			commit_action_func commit;
			allowed_to_commit_func allowed = allowed_to_commit_default;
			get_acceptance_func get_acceptance = get_acceptance_default;
			allowed_to_cancel_func allowed_cancel = allowed_to_cancel_default;

			operator DiplomaticActionType::Initializer() {
				return { influence_cost, commit, allowed, get_acceptance };
			}
		};

		const allowed_to_cancel_func allowed_to_cancel = allowed_to_cancel_default;

	private:
		CancelableDiplomaticActionType(Initializer&& initializer);
	};

	struct DiplomaticActionTickCache;

	struct DiplomaticActionTypeStorage : std::variant<DiplomaticActionType, CancelableDiplomaticActionType>, HasIdentifier {
		using base_type = std::variant<DiplomaticActionType, CancelableDiplomaticActionType>;

		template<typename T>
		constexpr DiplomaticActionTypeStorage(std::string_view identifier, T&& t) : HasIdentifier(identifier), base_type(t) {}

		template<class Visitor>
		constexpr decltype(auto) visit(Visitor&& vis) {
			return std::visit(std::forward<Visitor>(vis), *this);
		}

		template<class Visitor>
		constexpr decltype(auto) visit(Visitor&& vis) const {
			return std::visit(std::forward<Visitor>(vis), *this);
		}

		constexpr bool is_cancelable() const {
			return visit([](auto&& arg) -> bool {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::same_as<T, CancelableDiplomaticActionType>) {
					return true;
				} else {
					return false;
				}
			});
		}
	};

	struct DiplomaticActionTickCache {
		DiplomaticActionType::Argument argument;
		DiplomaticActionTypeStorage const* type = nullptr;
		bool allowed_to_commit = false;
		std::int32_t acceptance = -1;
	};

	struct DiplomaticActionManager {
	private:
		IdentifierRegistry<DiplomaticActionTypeStorage> IDENTIFIER_REGISTRY(diplomatic_action_type);

	public:
		DiplomaticActionManager() = default;

		bool add_diplomatic_action(std::string_view identifier, DiplomaticActionType::Initializer&& initializer);
		bool add_cancelable_diplomatic_action(
			std::string_view identifier, CancelableDiplomaticActionType::Initializer&& initializer
		);

		DiplomaticActionTickCache create_diplomatic_action_tick(
			std::string_view identifier, CountryInstance* sender, CountryInstance* reciever, std::any context_data,
			InstanceManager& instance_manager
		);

		bool setup_diplomatic_actions();
	};
}

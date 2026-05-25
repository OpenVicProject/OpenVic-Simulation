#include "DiplomaticAction.hpp"

using namespace OpenVic;

DiplomaticActionType::DiplomaticActionType(DiplomaticActionType::Initializer&& initializer)
	: commit_action_caller { std::move(initializer.commit) }, allowed_to_commit { std::move(initializer.allowed) },
	  get_acceptance { std::move(initializer.get_acceptance) } {}

CancelableDiplomaticActionType::CancelableDiplomaticActionType(CancelableDiplomaticActionType::Initializer&& initializer)
	: allowed_to_cancel { std::move(initializer.allowed_cancel) }, DiplomaticActionType { std::move(initializer) } {}

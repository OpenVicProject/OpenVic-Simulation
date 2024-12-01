//Based on https://en.cppreference.com/w/cpp/feature_test#Compiler_Features_Dump
#pragma once

#include <execution> //used in the macro

namespace OpenVic {
	constexpr int EXECUTION_POLICY_MINIMUM_VERSION = 201603L;
	#if __cpp_lib_execution != '_' && __cpp_lib_execution >= EXECUTION_POLICY_MINIMUM_VERSION
		#define PARALLELISE_IF_SUPPORTED(items_rh, captured_variables, item_variable_name, method_body) \
		auto& items = items_rh; \
		std::for_each( \
			std::execution::par, \
			items.begin(), \
			items.end(), \
			[captured_variables](auto& item_variable_name) -> void { \
				method_body \
			} \
		);
	#else
		#define PARALLELISE_IF_SUPPORTED(items_rh, captured_variables, item_variable_name, method_body) \
		for (auto& item_variable_name : items_rh) { \
			method_body \
		}
	#endif
}
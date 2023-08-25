#pragma once

#include <map>

#include "openvic/types/Colour.hpp"
#include "openvic/types/Date.hpp"
#include "openvic/types/Return.hpp"
#include "openvic/types/fixed_point/FP.hpp"

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

namespace OpenVic {
	namespace ast = ovdl::v2script::ast;

	namespace NodeTools {

		template<typename T>
		return_t expect_type(ast::NodeCPtr node, std::function<return_t(T const&)> callback);

		return_t expect_identifier(ast::NodeCPtr node, std::function<return_t(std::string_view)> callback);
		return_t expect_string(ast::NodeCPtr node, std::function<return_t(std::string_view)> callback);
		return_t expect_identifier_or_string(ast::NodeCPtr node, std::function<return_t(std::string_view)> callback);
		return_t expect_bool(ast::NodeCPtr node, std::function<return_t(bool)> callback);
		return_t expect_int(ast::NodeCPtr node, std::function<return_t(int64_t)> callback);
		return_t expect_uint(ast::NodeCPtr node, std::function<return_t(uint64_t)> callback);
		return_t expect_fixed_point(ast::NodeCPtr node, std::function<return_t(FP)> callback);
		return_t expect_colour(ast::NodeCPtr node, std::function<return_t(colour_t)> callback);
		return_t expect_date(ast::NodeCPtr node, std::function<return_t(Date)> callback);
		return_t expect_assign(ast::NodeCPtr node, std::function<return_t(std::string_view, ast::NodeCPtr)> callback);

		static const std::function<size_t(size_t)> default_length_callback = [](size_t size) -> size_t { return size; };

		template<typename T> requires requires(T& t) {
			{ t.size() } -> std::same_as<size_t>;
			t.reserve( size_t {} );
		}
		std::function<size_t(size_t)> reserve_length_callback(T& t) {
			return [&t](size_t size) -> size_t {
				t.reserve(t.size() + size);
				return size;
			};
		}

		return_t expect_list_and_length(ast::NodeCPtr node, std::function<size_t(size_t)> length_callback, std::function<return_t(ast::NodeCPtr)> callback, bool file_node = false);
		return_t expect_list_of_length(ast::NodeCPtr node, std::function<return_t(ast::NodeCPtr)> callback, size_t length, bool file_node = false);
		return_t expect_list(ast::NodeCPtr node, std::function<return_t(ast::NodeCPtr)> callback, bool file_node = false);
		return_t expect_dictionary_and_length(ast::NodeCPtr node, std::function<size_t(size_t)> length_callback, std::function<return_t(std::string_view, ast::NodeCPtr)> callback, bool file_node = false);
		return_t expect_dictionary(ast::NodeCPtr node, std::function<return_t(std::string_view, ast::NodeCPtr)> callback, bool file_node = false);

		static const std::function<return_t(ast::NodeCPtr)> success_callback = [](ast::NodeCPtr) -> return_t { return SUCCESS; };

		struct dictionary_entry_t {
			bool must_appear, can_repeat;
			std::function<return_t(ast::NodeCPtr)> callback;
		};
		using dictionary_key_map_t = std::map<std::string, dictionary_entry_t, std::less<void>>;
		return_t expect_dictionary_keys(ast::NodeCPtr node, dictionary_key_map_t const& keys, bool allow_other_keys = false, bool file_node = false);
	}
}

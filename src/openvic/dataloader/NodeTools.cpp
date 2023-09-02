#include "NodeTools.hpp"

#include <charconv>

using namespace OpenVic;

template<typename T>
return_t NodeTools::expect_type(ast::NodeCPtr node, std::function<return_t(T const&)> callback) {
	if (node != nullptr) {
		if (node->is_type<T>()) {
			return callback(ast::cast_node_cptr<T>(node));
		}
		Logger::error("Invalid node type ", node->get_type(), " when expecting ", T::get_type_static());
	} else {
		Logger::error("Null node when expecting ", T::get_type_static());
	}
	return FAILURE;
}

static return_t identifier_callback_wrapper(std::function<return_t(std::string_view)> callback, std::string_view identifier) {
	if (!identifier.empty()) {
		return callback(identifier);
	}
	Logger::error("Empty identifier node string");
	return FAILURE;
}

return_t NodeTools::expect_identifier(ast::NodeCPtr node, std::function<return_t(std::string_view)> callback) {
	return expect_type<ast::IdentifierNode>(node, [callback](ast::IdentifierNode const& identifier_node) -> return_t {
		return identifier_callback_wrapper(callback, identifier_node._name);
	});
}

return_t NodeTools::expect_string(ast::NodeCPtr node, std::function<return_t(std::string_view)> callback) {
	return expect_type<ast::StringNode>(node, [callback](ast::StringNode const& string_node) -> return_t {
		return callback(string_node._name);
	});
}

return_t NodeTools::expect_identifier_or_string(ast::NodeCPtr node, std::function<return_t(std::string_view)> callback) {
	if (node != nullptr) {
		if (node->is_type<ast::IdentifierNode>()) {
			return identifier_callback_wrapper(callback, ast::cast_node_cptr<ast::IdentifierNode>(node)._name);
		} else if (node->is_type<ast::StringNode>()) {
			return callback(ast::cast_node_cptr<ast::StringNode>(node)._name);
		} else {
			Logger::error("Invalid node type ", node->get_type(), " when expecting ", ast::IdentifierNode::get_type_static(), " or ", ast::StringNode::get_type_static());
		}
	} else {
		Logger::error("Null node when expecting ", ast::IdentifierNode::get_type_static(), " or ", ast::StringNode::get_type_static());
	}
	return FAILURE;
}

return_t NodeTools::expect_bool(ast::NodeCPtr node, std::function<return_t(bool)> callback) {
	return expect_identifier(node, [callback](std::string_view identifier) -> return_t {
		if (identifier == "yes") {
			return callback(true);
		} else if (identifier == "no") {
			return callback(false);
		}
		Logger::error("Invalid bool identifier text: ", identifier);
		return FAILURE;
	});
}

return_t NodeTools::expect_int(ast::NodeCPtr node, std::function<return_t(int64_t)> callback) {
	return expect_identifier(node, [callback](std::string_view identifier) -> return_t {
		bool successful = false;
		const int64_t val = StringUtils::string_to_int64(identifier, &successful, 10);
		if (successful) {
			return callback(val);
		}
		Logger::error("Invalid int identifier text: ", identifier);
		return FAILURE;
	});
}

return_t NodeTools::expect_uint(ast::NodeCPtr node, std::function<return_t(uint64_t)> callback) {
	return expect_identifier(node, [callback](std::string_view identifier) -> return_t {
		bool successful = false;
		const uint64_t val = StringUtils::string_to_uint64(identifier, &successful, 10);
		if (successful) {
			return callback(val);
		}
		Logger::error("Invalid uint identifier text: ", identifier);
		return FAILURE;
	});
}

return_t NodeTools::expect_fixed_point(ast::NodeCPtr node, std::function<return_t(FP)> callback) {
	return expect_identifier(node, [callback](std::string_view identifier) -> return_t {
		bool successful = false;
		const FP val = FP::parse(identifier.data(), identifier.length(), &successful);
		if (successful) {
			return callback(val);
		}
		Logger::error("Invalid fixed point identifier text: ", identifier);
		return FAILURE;
	});
}

return_t NodeTools::expect_colour(ast::NodeCPtr node, std::function<return_t(colour_t)> callback) {
	colour_t col = NULL_COLOUR;
	uint32_t components = 0;
	return_t ret = expect_list_of_length(node, std::bind(expect_fixed_point, std::placeholders::_1,
		[&col, &components](FP val) -> return_t {
			return_t ret = SUCCESS;
			if (val < 0 || val > 255) {
				Logger::error("Invalid colour component: ", val);
				val = FP::_0();
				ret = FAILURE;
			}
			if (val <= 1) val *= 255;
			col = (col << 8) | val.to_int32_t();
			components++;
			return ret;
		}), 3);
	if (components < 3) col <<= 8 * (3 - components);
	if (callback(col) != SUCCESS) ret = FAILURE;
	return ret;
}

return_t NodeTools::expect_date(ast::NodeCPtr node, std::function<return_t(Date)> callback) {
	return expect_identifier(node, [callback](std::string_view identifier) -> return_t {
		bool successful = false;
		const Date date = Date::from_string(identifier, &successful);
		if (successful) {
			return callback(date);
		}
		Logger::error("Invalid date identifier text: ", identifier);
		return FAILURE;
	});
}

return_t NodeTools::expect_assign(ast::NodeCPtr node, std::function<return_t(std::string_view, ast::NodeCPtr)> callback) {
	return expect_type<ast::AssignNode>(node, [callback](ast::AssignNode const& assign_node) -> return_t {
		return callback(assign_node._name, assign_node._initializer.get());
	});
}

return_t NodeTools::expect_list_and_length(ast::NodeCPtr node, std::function<size_t(size_t)> length_callback, std::function<return_t(ast::NodeCPtr)> callback, bool file_node) {
	const std::function<return_t(std::vector<ast::NodeUPtr> const&)> list_func = [length_callback, callback](std::vector<ast::NodeUPtr> const& list) -> return_t {
		return_t ret = SUCCESS;
		size_t size = length_callback(list.size());
		if (size > list.size()) {
			Logger::error("Trying to read more values than the list contains: ", size, " > ", list.size());
			size = list.size();
			ret = FAILURE;
		}
		std::for_each(list.begin(), list.begin() + size, [callback, &ret](ast::NodeUPtr const& sub_node) -> void {
			if (callback(sub_node.get()) != SUCCESS) ret = FAILURE;
		});
		return ret;
	};
	if (!file_node) {
		return expect_type<ast::ListNode>(node, [list_func](ast::ListNode const& list_node) -> return_t {
			return list_func(list_node._statements);
		});
	} else {
		return expect_type<ast::FileNode>(node, [list_func](ast::FileNode const& file_node) -> return_t {
			return list_func(file_node._statements);
		});
	}
}

return_t NodeTools::expect_list_of_length(ast::NodeCPtr node, std::function<return_t(ast::NodeCPtr)> callback, size_t length, bool file_node) {
	return_t ret = SUCCESS;
	if (expect_list_and_length(node, [length, &ret](size_t size) -> size_t {
		if (size != length) {
			Logger::error("List length ", size, " does not match expected length ", length);
			ret = FAILURE;
			if (length < size) return length;
		}
		return size;
	}, callback, file_node) != SUCCESS) ret = FAILURE;
	return ret;
}

return_t NodeTools::expect_list(ast::NodeCPtr node, std::function<return_t(ast::NodeCPtr)> callback, bool file_node) {
	return expect_list_and_length(node, default_length_callback, callback, file_node);
}

return_t NodeTools::expect_dictionary_and_length(ast::NodeCPtr node, std::function<size_t(size_t)> length_callback, std::function<return_t(std::string_view, ast::NodeCPtr)> callback, bool file_node) {
	return expect_list_and_length(node, length_callback, std::bind(expect_assign, std::placeholders::_1, callback), file_node);
}

return_t NodeTools::expect_dictionary(ast::NodeCPtr node, std::function<return_t(std::string_view, ast::NodeCPtr)> callback, bool file_node) {
	return expect_dictionary_and_length(node, default_length_callback, callback, file_node);
}

return_t NodeTools::expect_dictionary_keys(ast::NodeCPtr node, dictionary_key_map_t const& keys, bool allow_other_keys, bool file_node) {
	std::map<std::string, size_t, std::less<void>> key_count;
	return_t ret = expect_dictionary(node, [keys, allow_other_keys, &key_count](std::string_view key, ast::NodeCPtr value) -> return_t {
		const dictionary_key_map_t::const_iterator it = keys.find(key);
		if (it == keys.end()) {
			if (allow_other_keys) return SUCCESS;
			Logger::error("Invalid dictionary key: ", key);
			return FAILURE;
		}
		const size_t count = ++key_count[it->first];
		dictionary_entry_t const& entry = it->second;
		if (!entry.can_repeat && count > 1) {
			Logger::error("Invalid repeat of dictionary key: ", key);
			return FAILURE;
		}
		return entry.callback(value);
	}, file_node);
	for (dictionary_key_map_t::value_type const& entry : keys) {
		if (entry.second.must_appear && key_count.find(entry.first) == key_count.end()) {
			Logger::error("Mandatory dictionary key not present: ", entry.first);
			ret = FAILURE;
		}
	}
	return ret;
}

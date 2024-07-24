#include "RuntimeFiber.hpp"

#include <cassert>
#include <optional>

#include "RuntimeProcess.hpp" // IWYU pragma: keep
#include "Stacktrace.hpp"
#include <lauf/runtime/process.h>
#include <lauf/runtime/stacktrace.h>

using namespace OpenVic::Vm;

const lauf_runtime_value* RuntimeFiberRef::get_vstack_base() const {
	assert(is_valid());
	return lauf_runtime_get_vstack_base(_handle);
}

RuntimeFiber::~RuntimeFiber() {
	destroy();
}

std::optional<Stacktrace> RuntimeFiber::get_stacktrace() const {
	assert(is_valid());
	auto result = lauf_runtime_get_stacktrace(*_process, _handle);
	if (result == nullptr) {
		return std::nullopt;
	}
	return std::make_optional<Stacktrace>(result);
}

std::optional<RuntimeFiberRef> RuntimeFiber::get_parent() {
	assert(is_valid());
	auto result = lauf_runtime_get_fiber_parent(*_process, _handle);
	if (result == nullptr) {
		return std::nullopt;
	}
	return result;
}

const lauf_runtime_value* RuntimeFiber::get_vstack_ptr() const {
	assert(is_valid());
	return lauf_runtime_get_vstack_ptr(*_process, _handle);
}

bool RuntimeFiber::resume(
	const lauf_runtime_value* input, size_t input_count, lauf_runtime_value* output, size_t output_count
) {
	assert(is_valid());
	return lauf_runtime_resume(*_process, _handle, input, input_count, output, output_count);
}

bool RuntimeFiber::resume_until_completion(
	const lauf_runtime_value* input, size_t input_count, lauf_runtime_value* output, size_t output_count
) {
	assert(is_valid());
	return lauf_runtime_resume_until_completion(*_process, _handle, input, input_count, output, output_count);
}

void RuntimeFiber::destroy() {
	if (!is_valid()) {
		return;
	}
	lauf_runtime_destroy_fiber(*_process, _handle);
	_handle = nullptr;
}

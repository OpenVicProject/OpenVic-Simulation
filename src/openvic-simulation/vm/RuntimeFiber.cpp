#include "RuntimeFiber.hpp"

#include "openvic-simulation/vm/RuntimeProcess.hpp" // IWYU pragma: keep

#include <lauf/runtime/process.h>
#include <lauf/runtime/stacktrace.h>
#include <lauf/runtime/value.h>

using namespace OpenVic::Vm;

const lauf_runtime_value* RuntimeFiberRef::get_vstack_base() const {
	return lauf_runtime_get_vstack_base(*this);
}

RuntimeFiber::~RuntimeFiber() {
	if (_handle == nullptr) {
		return;
	}
	lauf_runtime_destroy_fiber(*_process, *this);
	_handle = nullptr;
}

std::optional<Stacktrace> RuntimeFiber::get_stacktrace() const {
	return _process->get_stacktrace_for(*this);
}

std::optional<RuntimeFiberRef> RuntimeFiber::get_parent() {
	return _process->get_parent_for(*this);
}

const lauf_runtime_value* RuntimeFiber::get_vstack_ptr() const {
	return _process->get_vstack_ptr_for(*this);
}

bool RuntimeFiber::resume(lauf_runtime_value input, lauf_runtime_value& output) {
	return resume(std::span<lauf_runtime_value> { &input, 1 }, output);
}
bool RuntimeFiber::resume(lauf_runtime_value input, std::span<lauf_runtime_value> output) {
	return resume(std::span<lauf_runtime_value> { &input, 1 }, output);
}
bool RuntimeFiber::resume(const std::span<lauf_runtime_value> input, lauf_runtime_value& output) {
	return resume(input, std::span<lauf_runtime_value> { &output, 1 });
}
bool RuntimeFiber::resume(const std::span<lauf_runtime_value> input, std::span<lauf_runtime_value> output) {
	return lauf_runtime_resume(*_process, *this, input.data(), input.size(), output.data(), output.size());
}

bool RuntimeFiber::resume_until_completion(lauf_runtime_value input, lauf_runtime_value& output) {
	return resume_until_completion(std::span<lauf_runtime_value> { &input, 1 }, output);
}
bool RuntimeFiber::resume_until_completion(lauf_runtime_value input, std::span<lauf_runtime_value> output) {
	return resume_until_completion(std::span<lauf_runtime_value> { &input, 1 }, output);
}
bool RuntimeFiber::resume_until_completion(const std::span<lauf_runtime_value> input, lauf_runtime_value& output) {
	return resume_until_completion(input, std::span<lauf_runtime_value> { &output, 1 });
}
bool RuntimeFiber::resume_until_completion(const std::span<lauf_runtime_value> input, std::span<lauf_runtime_value> output) {
	return lauf_runtime_resume_until_completion(*_process, *this, input.data(), input.size(), output.data(), output.size());
}

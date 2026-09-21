#pragma once

#include <concepts>
#include <mutex>
#include <type_traits>

#include "openvic-simulation/core/memory/SmartPtr.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticSink.hpp"

namespace OpenVic::dataloader {
	class DiagnosticItem;

	template<typename MutexT, std::derived_from<DiagnosticSink> DiagnosticSinkT = DiagnosticSink>
	class MutexSink final : public DiagnosticSink {
	public:
		using sink_type = DiagnosticSinkT;

		using sink_pointer_type =
		    std::conditional_t<std::is_final_v<sink_type>, memory::unique_ptr<sink_type>, memory::unique_base_ptr<sink_type>>;

		explicit MutexSink(sink_pointer_type sink) : _sink(std::move(sink)) {}

		template<typename... Args>
		requires std::constructible_from<DiagnosticSinkT, Args...>
		explicit MutexSink(Args&&... args) : _sink(memory::make_unique(std::forward<Args>(args)...)) {}

		~MutexSink() override = default;

		void begin() override {
			std::lock_guard<MutexT> lock(_mutex);
			_sink->begin();
		}

		void end() override {
			std::lock_guard<MutexT> lock(_mutex);
			_sink->end();
		}

		void consume(DiagnosticItem const& diag) override {
			std::lock_guard<MutexT> lock(_mutex);
			_sink->consume(diag);
		}

		void flush() override {
			std::lock_guard<MutexT> lock(_mutex);
			_sink->flush();
		}

	protected:
		mutable MutexT _mutex;
		sink_pointer_type _sink;
	};

	template<typename MutexT, std::derived_from<DiagnosticSink> DiagnosticSinkT = DiagnosticSink, typename... Args>
	requires std::constructible_from<DiagnosticSinkT, Args...>
	auto make_mutex_sink(Args&&... args) {
		return memory::make_unique<DiagnosticSinkT>(std::forward<Args>(args)...);
	}
}

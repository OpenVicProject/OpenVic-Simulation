#pragma once

#include <concepts>

#include <range/v3/range/concepts.hpp>
#include <range/v3/range/traits.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/core/memory/SmartPtr.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticSink.hpp"

namespace OpenVic::dataloader {
	class DiagnosticItem;
	class DiagnosticBag;

	class DiagnosticRenderer {
	public:
		using sink_pointer_type = memory::unique_base_ptr<DiagnosticSink>;
		using sink_vector = memory::vector<sink_pointer_type>;

		void render(DiagnosticItem const& item) const;
		void render(DiagnosticBag const& bag) const;

		template<ranges::range Container>
		requires std::convertible_to<ranges::range_reference_t<Container>, DiagnosticItem const&>
		void render(Container const& items) const {
			for (sink_pointer_type const& sink : _sinks) {
				sink->begin();
			}

			for (DiagnosticItem const& item : items) {
				render(item);
			}

			for (sink_pointer_type const& sink : _sinks) {
				sink->end();
				sink->flush();
			}
		}

		void flush() const;

		DiagnosticSink* append_sink(sink_pointer_type sink) OV_LIFETIME_BOUND;

		template<std::derived_from<DiagnosticSink> T, typename... Args>
		T* emplace_sink(Args&&... args) OV_LIFETIME_BOUND {
			return static_cast<T*>(append_sink(memory::make_unique<T>(std::forward<Args>(args)...)));
		}

		sink_vector::const_iterator erase_sink(sink_vector::const_iterator pos) OV_LIFETIME_BOUND;

		void clear_sinks();

		sink_vector const& get_sinks() const& OV_LIFETIME_BOUND;
		sink_vector get_sinks() &&;

		template<std::derived_from<DiagnosticSink> T, typename... Args>
		static DiagnosticRenderer make(Args&&... args) {
			DiagnosticRenderer renderer;
			renderer.emplace_sink<T>(std::forward<Args>(args)...);
			return renderer;
		}

	private:
		sink_vector _sinks;
	};
}

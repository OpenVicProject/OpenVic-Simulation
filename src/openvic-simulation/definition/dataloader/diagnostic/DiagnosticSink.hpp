#pragma once

namespace OpenVic::dataloader {
	class DiagnosticItem;

	class DiagnosticSink {
	public:
		virtual ~DiagnosticSink() = default;

		// Called for every diagnostic
		virtual void consume(DiagnosticItem const& diag) = 0;

		virtual void begin() {} // start of a batch / report
		virtual void end() {} // end of a batch / report
		virtual void flush() {} // force output of any buffered data
	};
}

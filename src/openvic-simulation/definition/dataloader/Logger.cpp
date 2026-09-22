#include "Logger.hpp"

#include <memory>
#include <mutex>

#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <spdlog/sinks/wincolor_sink.h>
#else
#include <spdlog/sinks/ansicolor_sink.h>
#endif

using namespace OpenVic::dataloader;

std::shared_ptr<spdlog::logger> logger_registry::default_logger() {
	std::lock_guard<std::mutex> lock(logger_map_mutex);
	return logger;
}

spdlog::logger* logger_registry::get_default_raw() {
	return logger.get();
}

logger_registry& logger_registry::instance() {
	static logger_registry registry;
	return registry;
}

logger_registry::logger_registry() {
#ifdef _WIN32
	auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
	auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif

	auto logger = std::make_shared<spdlog::logger>(std::string { logger_name }, std::move(color_sink));
	spdlog::register_logger(logger);
}

logger_registry::~logger_registry() = default;

std::shared_ptr<spdlog::logger> OpenVic::dataloader::default_logger() {
	return logger_registry::instance().default_logger();
}

spdlog::logger* OpenVic::dataloader::default_logger_raw() {
	return logger_registry::instance().get_default_raw();
}

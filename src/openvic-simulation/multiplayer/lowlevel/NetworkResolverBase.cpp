#include "NetworkResolverBase.hpp"

#include <atomic>
#include <charconv>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/reverse.hpp>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkResolver.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

using namespace OpenVic;

static memory::string _get_cache_key(std::string_view p_hostname, NetworkResolverBase::Type p_type) {
	memory::string key { p_hostname };
	key.insert(key.begin(), '0');
	key.reserve(p_hostname.size() + 1);

	std::to_chars_result result = StringUtils::to_chars(key.data(), key.data() + 1, static_cast<uint8_t>(p_type));
	if (result.ec != std::errc {}) {
		key = memory::string { p_hostname };
	}

	return key;
}

NetworkResolverBase::ResolveHandler::Item::Item() : type { Type::NONE } {
	clear();
}

void NetworkResolverBase::ResolveHandler::Item::clear() {
	status.store(static_cast<int32_t>(ResolverStatus::NONE));
	response.clear();
	type = Type::NONE;
	hostname.clear();
}

int32_t NetworkResolverBase::ResolveHandler::find_empty_id() const {
	for (int i = 0; i < MAX_QUERIES; i++) {
		if (queue[i].status.load() == static_cast<int32_t>(ResolverStatus::NONE)) {
			return i;
		}
	}
	return INVALID_ID;
}

void NetworkResolverBase::ResolveHandler::resolve_queues() {
	for (int i = 0; i < MAX_QUERIES; i++) {
		if (queue[i].status.load() != static_cast<int32_t>(ResolverStatus::WAITING)) {
			continue;
		}

		std::unique_lock lock { mutex };
		memory::string hostname = queue[i].hostname;
		Type type = queue[i].type;

		lock.unlock();

		// We should not lock while resolving the hostname,
		// only when modifying the queue.
		memory::vector<IpAddress> response = NetworkResolver::singleton()._resolve_hostname(hostname, type);

		lock.lock();
		// Could have been completed by another function, or deleted.
		if (queue[i].status.load() != static_cast<int32_t>(ResolverStatus::WAITING)) {
			continue;
		}
		// We might be overriding another result, but we don't care as long as the result is valid.
		if (response.size()) {
			memory::string key = _get_cache_key(hostname, type);
			cache[key] = response;
		}
		queue[i].response = response;
		queue[i].status.store(static_cast<int32_t>(response.empty() ? ResolverStatus::ERROR : ResolverStatus::DONE));
	}
}

void NetworkResolverBase::ResolveHandler::_thread_runner(ResolveHandler& handler) {
	while (!handler.should_abort.load(std::memory_order_acquire)) {
		handler.semaphore.acquire();
		handler.resolve_queues();
	}
}

NetworkResolverBase::ResolveHandler::ResolveHandler() : thread { _thread_runner, std::ref(*this) }, semaphore { 0 } {
	should_abort.store(false, std::memory_order_release);
}

NetworkResolverBase::ResolveHandler::~ResolveHandler() {
	should_abort.store(true, std::memory_order_release);
	semaphore.release();
	thread.join();
}

IpAddress NetworkResolverBase::resolve_hostname(std::string_view p_hostname, Type p_type) {
	const memory::vector<IpAddress> addresses = resolve_hostname_addresses(p_hostname, p_type);
	return addresses.empty() ? IpAddress {} : addresses.front();
}

memory::vector<IpAddress> NetworkResolverBase::resolve_hostname_addresses(std::string_view p_hostname, Type p_type) {
	memory::string key = _get_cache_key(p_hostname, p_type);
	{
		std::unique_lock lock(_resolve_handler.mutex);
		if (_resolve_handler.cache.contains(key)) {
			return _resolve_handler.cache[key];
		} else {
			// This should be run unlocked so the resolver thread can keep resolving
			// other requests.
			lock.unlock();
			memory::vector<IpAddress> result = _resolve_hostname(p_hostname, p_type);
			lock.lock();
			// We might be overriding another result, but we don't care as long as the result is valid.
			if (result.size()) {
				_resolve_handler.cache[key] = result;
			}
			return result;
		}
	}
}

int32_t NetworkResolverBase::resolve_hostname_queue_item(std::string_view p_hostname, Type p_type) {
	std::unique_lock lock(_resolve_handler.mutex);

	int32_t id = _resolve_handler.find_empty_id();

	if (id == INVALID_ID) {
		Logger::warning("Out of resolver queries");
		return id;
	}

	memory::string key = _get_cache_key(p_hostname, p_type);
	_resolve_handler.queue[id].hostname = p_hostname;
	_resolve_handler.queue[id].type = p_type;
	if (_resolve_handler.cache.contains(key)) {
		_resolve_handler.queue[id].response = _resolve_handler.cache[key];
		_resolve_handler.queue[id].status.store(static_cast<int32_t>(ResolverStatus::DONE));
	} else {
		_resolve_handler.queue[id].response = memory::vector<IpAddress>();
		_resolve_handler.queue[id].status.store(static_cast<int32_t>(ResolverStatus::WAITING));
		if (_resolve_handler.thread.joinable()) {
			_resolve_handler.semaphore.release();
		} else {
			_resolve_handler.resolve_queues();
		}
	}

	return id;
}

NetworkResolverBase::ResolverStatus NetworkResolverBase::get_resolve_item_status(int32_t p_id) const {
	OV_ERR_FAIL_INDEX_V_MSG(
		p_id, MAX_QUERIES, ResolverStatus::NONE,
		fmt::format(
			"Too many concurrent DNS resolver queries ({}, but should be {} at most). Try performing less network requests at "
			"once.",
			p_id, MAX_QUERIES
		)
	);

	ResolverStatus result = static_cast<ResolverStatus>(_resolve_handler.queue[p_id].status.load());
	if (result == ResolverStatus::NONE) {
		Logger::error("Condition status == " _OV_STR(ResolverStatus::NONE));
		return ResolverStatus::NONE;
	}
	return result;
}

memory::vector<IpAddress> NetworkResolverBase::get_resolve_item_addresses(int32_t p_id) const {
	OV_ERR_FAIL_INDEX_V_MSG(
		p_id, MAX_QUERIES, memory::vector<IpAddress> {},
		fmt::format(
			"Too many concurrent DNS resolver queries ({}, but should be {} at most). Try performing less network requests at "
			"once.",
			p_id, MAX_QUERIES
		)
	);

	std::unique_lock lock(const_cast<std::mutex&>(_resolve_handler.mutex));

	if (_resolve_handler.queue[p_id].status.load() != static_cast<int32_t>(ResolverStatus::DONE)) {
		Logger::error(fmt::format("Resolve of '{}' didn't complete yet.", _resolve_handler.queue[p_id].hostname));
		return {};
	}

	return _resolve_handler.queue[p_id].response | ranges::views::filter([](IpAddress const& addr) -> bool {
			   return addr.is_valid();
		   }) |
		ranges::to<memory::vector<IpAddress>>();
}

IpAddress NetworkResolverBase::get_resolve_item_address(int32_t p_id) const {
	OV_ERR_FAIL_INDEX_V_MSG(
		p_id, MAX_QUERIES, IpAddress {},
		fmt::format(
			"Too many concurrent DNS resolver queries ({}, but should be {} at most). Try performing less network requests at "
			"once.",
			p_id, MAX_QUERIES
		)
	);

	std::unique_lock lock(const_cast<std::mutex&>(_resolve_handler.mutex));

	if (_resolve_handler.queue[p_id].status.load() != static_cast<int32_t>(ResolverStatus::DONE)) {
		Logger::error(fmt::format("Resolve of '{}' didn't complete yet.", _resolve_handler.queue[p_id].hostname));
		return {};
	}

	for (IpAddress const& address : _resolve_handler.queue[p_id].response) {
		if (address.is_valid()) {
			return address;
		}
	}
	return IpAddress {};
}

void NetworkResolverBase::erase_resolve_item(int32_t p_id) {
	OV_ERR_FAIL_INDEX_MSG(
		p_id, MAX_QUERIES,
		fmt::format(
			"Too many concurrent DNS resolver queries ({}, but should be {} at most). Try performing less network requests at "
			"once.",
			p_id, MAX_QUERIES
		)
	);

	_resolve_handler.queue[p_id].status.store(static_cast<int32_t>(ResolverStatus::NONE));
}
void NetworkResolverBase::clear_cache(std::string_view p_hostname) {
	std::unique_lock lock(_resolve_handler.mutex);

	if (p_hostname.empty()) {
		_resolve_handler.cache.clear();
	} else {
		_resolve_handler.cache.erase(_get_cache_key(p_hostname, Type::NONE));
		_resolve_handler.cache.erase(_get_cache_key(p_hostname, Type::IPV4));
		_resolve_handler.cache.erase(_get_cache_key(p_hostname, Type::IPV6));
		_resolve_handler.cache.erase(_get_cache_key(p_hostname, Type::ANY));
	}
}

memory::vector<IpAddress> NetworkResolverBase::get_local_addresses() const {
	OpenVic::string_map_t<InterfaceInfo> interfaces = get_local_interfaces();
	if (interfaces.empty()) {
		return {};
	}

	memory::vector<IpAddress> result;
	size_t reserve_size = 0;

	for (std::pair<memory::string, InterfaceInfo> const& pair : interfaces | ranges::views::reverse) {
		reserve_size += pair.second.ip_addresses.size();
	}
	if (reserve_size == 0) {
		return {};
	}

	result.reserve(reserve_size);
	for (std::pair<memory::string, InterfaceInfo> const& pair : interfaces | ranges::views::reverse) {
		for (IpAddress const& address : pair.second.ip_addresses | ranges::views::reverse) {
			result.push_back(address);
		}
	}
	return result;
}

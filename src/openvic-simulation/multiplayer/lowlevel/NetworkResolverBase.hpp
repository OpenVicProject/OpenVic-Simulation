#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <semaphore>
#include <string_view>
#include <thread>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct NetworkResolverBase {
		static constexpr size_t MAX_QUERIES = 256;
		static constexpr int32_t INVALID_ID = -1;

		enum class Provider : uint8_t {
			UNIX,
			WINDOWS,
		};

		enum class ResolverStatus : uint8_t {
			NONE,
			WAITING,
			DONE,
			ERROR,
		};

		enum class Type : uint8_t {
			NONE,
			IPV4,
			IPV6,
			ANY,
		};

		struct InterfaceInfo {
			memory::string name;
			memory::string name_friendly;
			memory::string index;
			memory::vector<IpAddress> ip_addresses;
		};

		IpAddress resolve_hostname(std::string_view p_hostname, Type p_type = Type::ANY);
		memory::vector<IpAddress> resolve_hostname_addresses(std::string_view p_hostname, Type p_type = Type::ANY);
		// async resolver hostname
		int32_t resolve_hostname_queue_item(std::string_view p_hostname, Type p_type = Type::ANY);
		ResolverStatus get_resolve_item_status(int32_t p_id) const;
		memory::vector<IpAddress> get_resolve_item_addresses(int32_t p_id) const;
		IpAddress get_resolve_item_address(int32_t p_id) const;

		void erase_resolve_item(int32_t p_id);

		void clear_cache(std::string_view p_hostname = "");

		memory::vector<IpAddress> get_local_addresses() const;
		virtual OpenVic::string_map_t<InterfaceInfo> get_local_interfaces() const = 0;

		virtual Provider provider() const = 0;

	protected:
		virtual memory::vector<IpAddress> _resolve_hostname(std::string_view p_hostname, Type p_type = Type::ANY) const = 0;

		struct ResolveHandler {
			int32_t find_empty_id() const;
			void resolve_queues();

			struct Item {
				Item();
				void clear();

				memory::vector<IpAddress> response;
				memory::string hostname;
				std::atomic_int32_t status;
				Type type;
			};

			std::array<Item, MAX_QUERIES> queue;
			string_map_t<memory::vector<IpAddress>> cache;
			std::mutex mutex;
			std::thread thread;
			std::counting_semaphore<> semaphore;
			std::atomic_bool should_abort;

			static void _thread_runner(ResolveHandler& handler);

			ResolveHandler();
			~ResolveHandler();
		} _resolve_handler;
	};
}

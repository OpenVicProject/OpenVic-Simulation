#pragma once

#include <memory>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <type_traits>
#include <utility>

#include <plf_colony.h>

#include <fmt/format.h>

#include "openvic-simulation/utility/DequeMemory.hpp"
#include "openvic-simulation/utility/MemoryTracker.hpp"

#include <foonathan/memory/allocator_storage.hpp>
#include <foonathan/memory/container.hpp>
#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/heap_allocator.hpp>
#include <foonathan/memory/new_allocator.hpp>
#include <foonathan/memory/smart_ptr.hpp>
#include <foonathan/memory/tracking.hpp>

namespace OpenVic::memory {
	template<class RawAllocator>
	tracker<RawAllocator> make_tracked_allocator( //
		RawAllocator&& alloc
	) {
#ifdef DEBUG_ENABLED
		return foonathan::memory::make_tracked_allocator(
			typename tracker<RawAllocator>::tracker {}, std::forward<RawAllocator>(alloc)
		);
#else
		return std::forward<RawAllocator>(alloc);
#endif
	}

	template<class RawAllocator>
	using allocator_reference = foonathan::memory::allocator_reference<RawAllocator>;

	template<class RawAllocator>
	allocator_reference<RawAllocator> make_allocator_reference(RawAllocator&& alloc) {
		return foonathan::memory::make_allocator_reference(alloc);
	}

	template<typename T, class RawAllocator>
	using allocator_deleter = foonathan::memory::allocator_deleter<T, RawAllocator>;

	template<class RawAllocator>
	struct will_optimize_deleter_for : std::false_type {};

	template<>
	struct will_optimize_deleter_for<foonathan::memory::heap_allocator> : std::true_type {};

	template<>
	struct will_optimize_deleter_for<foonathan::memory::malloc_allocator> : std::true_type {};

	template<>
	struct will_optimize_deleter_for<foonathan::memory::new_allocator> : std::true_type {};

#ifdef DEBUG_ENABLED
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using unique_ptr = foonathan::memory::unique_ptr<T, tracker<RawAllocator>>;

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using unique_base_ptr = foonathan::memory::unique_base_ptr<T, tracker<RawAllocator>>;

	template<typename T, class RawAllocator, typename... Args>
	requires(!std::is_array_v<T>)
	unique_ptr<T, std::decay_t<RawAllocator>> allocate_unique(RawAllocator&& alloc, Args&&... args) {
		return foonathan::memory::allocate_unique<T>(
			make_tracked_allocator(std::forward<RawAllocator>(alloc)), std::forward<Args>(args)...
		);
	}

	template<typename T, class RawAllocator>
	requires(std::is_array_v<T>)
	unique_ptr<T, std::decay_t<RawAllocator>> allocate_unique(RawAllocator&& alloc, std::size_t size) {
		return foonathan::memory::allocate_unique<T, tracker<RawAllocator>>(
			make_tracked_allocator(std::forward<RawAllocator>(alloc)), size
		);
	}
#else
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using unique_ptr = std::conditional_t<
		will_optimize_deleter_for<RawAllocator>::value, std::unique_ptr<T>,
		foonathan::memory::unique_ptr<T, tracker<RawAllocator>>>;

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using unique_base_ptr = std::conditional_t<
		will_optimize_deleter_for<RawAllocator>::value, std::unique_ptr<T>,
		foonathan::memory::unique_base_ptr<T, tracker<RawAllocator>>>;

	template<typename T, class RawAllocator, typename... Args>
	requires(!std::is_array_v<T>)
	unique_ptr<T, std::decay_t<RawAllocator>> allocate_unique(RawAllocator&& alloc, Args&&... args) {
		if constexpr (will_optimize_deleter_for<RawAllocator>::value) {
			return std::make_unique<T>(std::forward<Args>(args)...);
		} else {
			return foonathan::memory::allocate_unique<T>(
				make_tracked_allocator(std::forward<RawAllocator>(alloc)), std::forward<Args>(args)...
			);
		}
	}

	template<typename T, class RawAllocator>
	requires(std::is_array_v<T>)
	unique_ptr<T, std::decay_t<RawAllocator>> allocate_unique(RawAllocator&& alloc, std::size_t size) {
		if constexpr (will_optimize_deleter_for<RawAllocator>::value) {
			return std::make_unique<T>(size);
		} else {
			return foonathan::memory::allocate_unique<T, tracker<RawAllocator>>(
				make_tracked_allocator(std::forward<RawAllocator>(alloc)), size
			);
		}
	}
#endif

	template<typename T, class RawAllocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline T* allocate_new(RawAllocator&& alloc, Args&&... args) {
		allocator_reference<RawAllocator> alloc_ref = ::OpenVic::memory::make_allocator_reference(alloc);
		T* ptr = static_cast<T*>(alloc_ref.allocate_node(sizeof(T), alignof(T)));
		::new (ptr) T(std::forward<Args>(args)...);
		return ptr;
	}

	template<typename T, class RawAllocator, typename... Args>
	requires(std::is_unbounded_array_v<T>)
	static inline T* allocate_new(RawAllocator&& alloc, size_t n) {
		allocator_reference<RawAllocator> alloc_ref = ::OpenVic::memory::make_allocator_reference(alloc);
		return static_cast<T*>(alloc_ref.allocate_node(sizeof(T) * n, alignof(T)));
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline T* make_new(Args&&... args) {
		return allocate_new<T>(make_tracked_allocator(RawAllocator {}), std::forward<Args>(args)...);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(std::is_unbounded_array_v<T>)
	static inline T* make_new(size_t n) {
		return allocate_new<T>(make_tracked_allocator(RawAllocator {}), n);
	}

	template<typename T, class RawAllocator>
	requires(!std::is_array_v<T>)
	static inline void dealloc_delete(RawAllocator&& alloc, T* ptr) {
		ptr->~T();
		allocator_reference<RawAllocator> alloc_ref = ::OpenVic::memory::make_allocator_reference(alloc);
		alloc_ref.deallocate(ptr, sizeof(T), alignof(T));
	}

	template<typename T, class RawAllocator>
	requires(std::is_unbounded_array_v<T>)
	static inline void dealloc_delete(RawAllocator&& alloc, T* ptr, size_t n) {
		std::destroy_n(ptr, n);
		allocator_reference<RawAllocator> alloc_ref = ::OpenVic::memory::make_allocator_reference(alloc);
		alloc_ref.deallocate(ptr, sizeof(T) * n, alignof(T));
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	requires(!std::is_array_v<T>)
	static inline void delete_at(T* ptr) {
		dealloc_delete(make_tracked_allocator(RawAllocator {}), ptr);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	requires(std::is_unbounded_array_v<T>)
	static inline void delete_at(T* ptr, size_t n) {
		dealloc_delete(make_tracked_allocator(RawAllocator {}), ptr, n);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline unique_ptr<T, RawAllocator> make_unique(Args&&... args) {
		return allocate_unique<T>(RawAllocator {}, std::forward<Args>(args)...);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(std::is_unbounded_array_v<T>)
	static inline unique_ptr<T, RawAllocator> make_unique(size_t n) {
		return allocate_unique<T>(make_tracked_allocator(RawAllocator {}), n);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline unique_ptr<T, RawAllocator> make_unique_for_overwrite() {
		using raw_ptr = std::unique_ptr<T, foonathan::memory::allocator_deallocator<T, tracker<RawAllocator>>>;

		tracker<RawAllocator> alloc = make_tracked_allocator(RawAllocator {});
		auto memory = alloc.allocate_node(sizeof(T), alignof(T));
		// raw_ptr deallocates memory in case of constructor exception
		raw_ptr result(static_cast<T*>(memory), { alloc });
		// pass ownership to return value using a deleter that calls destructor
		return { result.release(), { alloc } };
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(std::is_unbounded_array_v<T>)
	static inline unique_ptr<T, RawAllocator> make_unique_for_overwrite(size_t size) {
		using raw_ptr = std::unique_ptr<T, foonathan::memory::allocator_deallocator<T, tracker<RawAllocator>>>;

		tracker<RawAllocator> alloc = make_tracked_allocator(RawAllocator {});
		auto memory = alloc.allocate_array(size, sizeof(T), alignof(T));
		// raw_ptr deallocates memory in case of constructor exception
		raw_ptr result(static_cast<T*>(memory), { alloc, size });
		// pass ownership to return value using a deleter that calls destructor
		return { result.release(), { alloc, size } };
	}

	template<typename T, class RawAllocator, typename... Args>
	std::shared_ptr<T> allocate_shared(RawAllocator&& alloc, Args&&... args) {
		return foonathan::memory::allocate_shared<T>(
			make_tracked_allocator(std::forward<RawAllocator>(alloc)), std::forward<Args>(args)...
		);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	static inline std::shared_ptr<T> make_shared(Args&&... args) {
		return allocate_shared<T>(RawAllocator {}, std::forward<Args>(args)...);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using vector = foonathan::memory::vector<T, tracker<RawAllocator>>;

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using deque = OpenVic::utility::memory::deque<T, tracker<RawAllocator>>;

	template<typename CharT, class RawAllocator = foonathan::memory::default_allocator>
	using basic_string =
		std::basic_string<CharT, std::char_traits<CharT>, foonathan::memory::std_allocator<CharT, tracker<RawAllocator>>>;

	template<class RawAllocator = foonathan::memory::default_allocator>
	using string_alloc = basic_string<char, RawAllocator>;

	using string = string_alloc<>;

	template<class RawAllocator = foonathan::memory::default_allocator>
	using wstring_alloc = basic_string<wchar_t, RawAllocator>;

	using wstring = wstring_alloc<>;

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using colony = plf::colony<T, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;

	template<typename T, typename Container = memory::deque<T>>
	using stack = std::stack<T, Container>;

	template<typename T, typename Container = memory::deque<T>>
	using queue = std::queue<T, Container>;

	template<typename T, typename CharTraits = std::char_traits<T>, class RawAllocator = foonathan::memory::default_allocator>
	using basic_stringstream =
		std::basic_stringstream<T, CharTraits, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;

	template<class RawAllocator = foonathan::memory::default_allocator>
	using stringstream_alloc = basic_stringstream<char, std::char_traits<char>, RawAllocator>;
	template<class RawAllocator = foonathan::memory::default_allocator>
	using wstringstream_alloc = basic_stringstream<wchar_t, std::char_traits<wchar_t>, RawAllocator>;

	using stringstream = stringstream_alloc<>;
	using wstringstream = wstringstream_alloc<>;

	namespace fmt {
		template<typename T, class RawAllocator = foonathan::memory::default_allocator>
		using basic_memory_buffer = ::fmt::basic_memory_buffer<
			T, ::fmt::inline_buffer_size, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;

		inline static memory::string vformat(::fmt::string_view fmt, ::fmt::format_args args) {
			memory::fmt::basic_memory_buffer<char> buf {};
			::fmt::vformat_to(std::back_inserter(buf), fmt, args);
			return memory::string(buf.data(), buf.size());
		}

		template<typename... Args>
		memory::string format(::fmt::string_view fmt, const Args&... args) {
			return memory::fmt::vformat(fmt, ::fmt::make_format_args(args...));
		}
	}
}

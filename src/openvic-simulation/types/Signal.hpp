#pragma once

#include <array>
#include <atomic>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>

#include "openvic-simulation/types/CowPtr.hpp"
#include "openvic-simulation/types/CowVector.hpp"
#include "openvic-simulation/types/NullMutex.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Utility.hpp"

// Based heavily on https://github.com/palacaze/sigslot and https://github.com/mousebyte/sigslot20

// MIT License

// Copyright (c) 2017 Pierre-Antoine Lacaze

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

namespace OpenVic {
	namespace _detail::signal {
		template<typename Lock, typename... Args>
		class basic_signal;

		template<template<typename...> typename Signal, typename Owner, typename... Args>
		class basic_signal_property;

		struct connection_blocker;
		struct connection;
		struct scoped_connection;

		template<typename Lockable>
		struct basic_observer;

		template<typename...>
		struct is_signal : std::false_type {};

		template<typename Lock, typename... Args>
		struct is_signal<basic_signal<Lock, Args...>> : std::true_type {};

		template<typename S>
		concept Signal = is_signal<S>::value;
	}

	template<typename Lock, typename... Args>
	using basic_signal = _detail::signal::basic_signal<Lock, Args...>;

	template<template<typename...> typename Signal, typename Owner, typename... Args>
	using basic_signal_property = _detail::signal::basic_signal_property<Signal, Owner, Args...>;

	template<typename... T>
	using signal = basic_signal<std::mutex, T...>;

	template<typename Owner, typename... T>
	using signal_property = basic_signal_property<signal, Owner, T...>;

	namespace nothread {
		template<typename... T>
		using signal = basic_signal<null_mutex, T...>;

		template<typename Owner, typename... T>
		using signal_property = basic_signal_property<signal, Owner, T...>;
	}

	using connection_blocker = _detail::signal::connection_blocker;
	using connection = _detail::signal::connection;
	using scoped_connection = _detail::signal::scoped_connection;

	template<typename Lockable>
	using basic_observer = _detail::signal::basic_observer<Lockable>;

	using observer = basic_observer<std::mutex>;

	namespace nothread {
		using observer = basic_observer<null_mutex>;
	}

	template<typename Lockable, typename Arg, typename... T, typename... Args>
	requires(!_detail::signal::Signal<std::decay_t<Arg>>)
	connection connect(basic_signal<Lockable, T...>& sig, Arg&& arg, Args&&... args);

	template<typename Lockable1, typename Lockable2, typename... T1, typename... T2, typename... Args>
	connection connect(basic_signal<Lockable1, T1...>& sig1, basic_signal<Lockable2, T2...>& sig2, Args&&... args);
}


namespace OpenVic::_detail::signal {
	template<typename Func, typename... Args>
	concept FreeCallable = requires(Func func, Args... args) { func(args...); };

	template<typename Func, typename Self, typename... Args>
	concept MemberCallable = requires(Func func, Self self, Args... args) { (self->*func)(args...); };

	template<typename T>
	concept WeakPtr = requires(T p) {
		{ p.expired() } -> std::same_as<bool>;
		p.lock();
		p.reset();
	};

	template<typename T>
	std::weak_ptr<T> to_weak(std::weak_ptr<T> w) {
		return w;
	}

	template<typename T>
	std::weak_ptr<T> to_weak(std::shared_ptr<T> s) {
		return s;
	}

	template<typename T>
	concept WeakPtrCompatible = requires(std::decay_t<T> t) {
		{ to_weak(t) } -> WeakPtr;
	};

	struct observer_type {};

	template<typename T>
	concept Observer = std::is_base_of_v<observer_type, std::remove_pointer_t<std::remove_reference_t<T>>>;

	using group_id = uint32_t;

	namespace mock {
		struct a {
			virtual ~a() = default;
			void f();
			virtual void g();
			static void h();
		};
		struct b {
			virtual ~b() = default;
			void f();
			virtual void g();
		};
		struct c : a, b {
			void f();
			void g() override;
		};
		struct d : virtual a {
			void g() override;
		};

		union function_union {
			decltype(&d::g) dm;
			decltype(&c::g) mm;
			decltype(&c::g) mvm;
			decltype(&a::f) m;
			decltype(&a::g) vm;
			decltype(&a::h) s;
			void (*f)();
			void* o;
		};
	}

	union function_pointer {
		void* value() {
			return data;
		}
		void const* value() const {
			return data;
		}
		template<typename T>
		T& value() {
			return *static_cast<T*>(value());
		}
		template<typename T>
		T const& value() const {
			return *static_cast<const T*>(value());
		}

		OV_ALWAYS_INLINE explicit operator bool() const {
			return value() != nullptr;
		}

		OV_ALWAYS_INLINE bool operator==(const function_pointer& o) const {
			return std::equal(std::begin(data), std::end(data), std::begin(o.data));
		}

		mock::function_union _;
		alignas(sizeof(mock::function_union)) char data[sizeof(mock::function_union)];
	};

	template<typename T>
	struct function_traits {
		static void ptr(T const&, function_pointer& ptr) {
			ptr.value<std::nullptr_t>() = nullptr;
		}

		static constexpr bool is_disconnectable = false;
		static constexpr bool must_check_object = true;
	};

	template<typename T>
	concept Function = std::is_function_v<T>;

	template<Function T>
	struct function_traits<T> {
		static void ptr(T& t, function_pointer& ptr) {
			ptr.value<T*>() = &t;
		}

		static constexpr bool is_disconnectable = true;
		static constexpr bool must_check_object = false;
	};

	template<Function T>
	struct function_traits<T*> {
		static void ptr(T* t, function_pointer& ptr) {
			ptr.value<T*>() = t;
		}

		static constexpr bool is_disconnectable = true;
		static constexpr bool must_check_object = false;
	};

	template<typename T>
	concept MemberFunctionPointer = std::is_member_function_pointer_v<T>;

	template<MemberFunctionPointer T>
	struct function_traits<T> {
		static void ptr(T t, function_pointer& ptr) {
			ptr.value<T>() = t;
		}

		static constexpr bool is_disconnectable = false;
		static constexpr bool must_check_object = true;
	};

	template<typename T>
	concept HasCallOperator = requires { &std::remove_reference_t<T>::operator(); };

	template<HasCallOperator T>
	struct function_traits<T> {
		using call_type = decltype(&std::remove_reference_t<T>::operator());

		static void ptr(T const&, function_pointer& ptr) {
			function_traits<call_type>::ptr(&T::operator(), ptr);
		}

		static constexpr bool is_disconnectable = function_traits<call_type>::is_disconnectable;
		static constexpr bool must_check_object = function_traits<call_type>::must_check_object;
	};

	template<typename T>
	function_pointer get_function_ptr(T const& t) {
		function_pointer ptr {};
		function_traits<std::decay_t<T>>::ptr(t, ptr);
		return ptr;
	}

	using obj_ptr = void const*;

	template<typename T>
	obj_ptr get_object_ptr(T const& t);

	template<typename T>
	struct object_pointer {
		static obj_ptr get(T const&) {
			return nullptr;
		}
	};

	template<typename T>
	concept Pointer = std::is_pointer_v<T>;

	template<typename T>
	requires Pointer<T*>
	struct object_pointer<T*> {
		static obj_ptr get(T const* t) {
			return reinterpret_cast<obj_ptr>(t);
		}
	};

	template<WeakPtr T>
	struct object_pointer<T> {
		static obj_ptr get(T const& t) {
			auto p = t.lock();
			return get_object_ptr(p);
		}
	};

	template<WeakPtrCompatible T>
	requires(!Pointer<T> && !WeakPtr<T>)
	struct object_pointer<T> {
		static obj_ptr get(T const& t) {
			return t ? reinterpret_cast<obj_ptr>(t.get()) : nullptr;
		}
	};

	template<typename T>
	obj_ptr get_object_ptr(T const& t) {
		return object_pointer<T>::get(t);
	}

	// Reduces compile-time at a slight cost of runtime performance
#ifdef DEBUG_ENABLED
	template<typename B, typename D, typename... Args>
	inline std::shared_ptr<B> make_shared(Args&&... args) {
		return std::shared_ptr<B>(static_cast<B*>(memory::make_new<D>(std::forward<Args>(args)...)));
	}
#else
	template<typename B, typename D, typename... Args>
	inline std::shared_ptr<B> make_shared(Args&&... args) {
		return std::static_pointer_cast<B>(memory::make_shared<D>(std::forward<Args>(args)...));
	}
#endif

	template<typename Signal>
	struct signal_wrapper {
		template<typename... U>
		void operator()(U&&... u) {
			(*signal)(std::forward<U>(u)...);
		}

		Signal* signal {};
	};

	struct slot_state {
		constexpr slot_state(group_id gid) : _group(gid) {}

		virtual ~slot_state() = default;

		virtual bool connected() const {
			return is_connected;
		}

		bool disconnect() {
			bool ret = is_connected.exchange(false);
			if (ret) {
				do_disconnect();
			}
			return ret;
		}

		bool blocked() const {
			return is_blocked.load();
		}
		void block() {
			is_blocked.store(true);
		}
		void unblock() {
			is_blocked.store(false);
		}

	protected:
		virtual void do_disconnect() {}

		size_t index() const {
			return _index;
		}

		size_t& index() {
			return _index;
		}

		group_id group() const {
			return _group;
		}

	private:
		template<typename, typename...>
		friend class basic_signal;

		std::size_t _index = 0; // index into the array of slot pointers inside the signal
		const group_id _group; // slot group this slot belongs to
		std::atomic_bool is_connected = true;
		std::atomic_bool is_blocked = false;
	};

	struct connection_blocker {
		connection_blocker() = default;
		~connection_blocker() {
			release();
		}

		connection_blocker(connection_blocker const&) = delete;
		connection_blocker& operator=(connection_blocker const&) = delete;

		connection_blocker(connection_blocker&& other) : state { std::move(other.state) } {}

		connection_blocker& operator=(connection_blocker&& other) {
			release();
			state.swap(other.state);
			return *this;
		}

	private:
		friend class connection;
		explicit connection_blocker(std::weak_ptr<slot_state> s) : state { std::move(s) } {
			if (std::shared_ptr<slot_state> ptr = state.lock()) {
				ptr->block();
			}
		}

		void release() {
			if (std::shared_ptr<slot_state> ptr = state.lock()) {
				ptr->unblock();
			}
		}

		std::weak_ptr<slot_state> state;
	};

	struct connection {
		connection() = default;
		virtual ~connection() = default;

		connection(connection const&) = default;
		connection& operator=(connection const&) = default;
		connection(connection&&) = default;
		connection& operator=(connection&&) = default;

		bool valid() const {
			return !state.expired();
		}

		bool connected() const {
			const std::shared_ptr<slot_state> d = state.lock();
			return d && d->connected();
		}

		bool disconnect() {
			std::shared_ptr<slot_state> d = state.lock();
			return d && d->disconnect();
		}

		bool blocked() const {
			const std::shared_ptr<slot_state> d = state.lock();
			return d && d->blocked();
		}

		void block() {
			if (std::shared_ptr<slot_state> d = state.lock()) {
				d->block();
			}
		}

		void unblock() {
			if (std::shared_ptr<slot_state> d = state.lock()) {
				d->unblock();
			}
		}

		connection_blocker blocker() const {
			return connection_blocker { state };
		}

	protected:
		template<typename, typename...>
		friend class basic_signal;
		explicit connection(std::weak_ptr<slot_state> s) : state { std::move(s) } {}

		std::weak_ptr<slot_state> state;
	};

	struct scoped_connection final : public connection {
		scoped_connection() = default;
		~scoped_connection() override {
			disconnect();
		}

		scoped_connection(connection const& c) : connection(c) {}
		scoped_connection(connection&& c) : connection(std::move(c)) {}

		scoped_connection(scoped_connection const&) = delete;
		scoped_connection& operator=(scoped_connection const&) = delete;

		scoped_connection(scoped_connection&& other) : connection { std::move(other.state) } {}

		scoped_connection& operator=(scoped_connection&& other) {
			disconnect();
			state.swap(other.state);
			return *this;
		}

	private:
		template<typename, typename...>
		friend class basic_signal;
		explicit scoped_connection(std::weak_ptr<slot_state> s) : connection { std::move(s) } {}
	};

	template<typename Lockable>
	struct basic_observer : private observer_type {
		virtual ~basic_observer() = default;

	protected:
		/**
		 * Disconnect all signals connected to this object.
		 *
		 * To avoid invocation of slots on a semi-destructed instance, which may happen
		 * in multi-threaded contexts, derived classes should call this method in their
		 * destructor. This will ensure proper disconnection prior to the destruction.
		 */
		void disconnect_all() {
			std::unique_lock<Lockable> _ { mutex };
			connections.clear();
		}

	private:
		template<typename, typename...>
		friend class basic_signal;

		void add_connection(connection conn) {
			std::unique_lock<Lockable> _ { mutex };
			connections.emplace_back(std::move(conn));
		}

		Lockable mutex;
		memory::vector<scoped_connection> connections;
	};

	using observer_st = basic_observer<null_mutex>;
	using observer = basic_observer<std::mutex>;

	struct cleanable {
		virtual ~cleanable() = default;
		virtual void clean(slot_state*) = 0;
	};

	template<typename...>
	struct basic_slot;

	template<typename... T>
	using slot_ptr = std::shared_ptr<basic_slot<T...>>;

	template<typename... Args>
	struct basic_slot : public slot_state {
		using base_types = std::tuple<Args...>;

		explicit basic_slot(cleanable& c, group_id gid) : slot_state(gid), cleaner(c) {}
		~basic_slot() override = default;

		// method effectively responsible for calling the "slot" function with
		// supplied arguments whenever emission happens.
		virtual void call_slot(Args...) = 0;

		template<typename... U>
		void operator()(U&&... u) {
			if (slot_state::connected() && !slot_state::blocked()) {
				call_slot(std::forward<U>(u)...);
			}
		}

		// check if we are storing callable c
		template<typename C>
		bool has_callable(C const& c) const {
			function_pointer cp = get_function_ptr(c);
			function_pointer p = get_callable();
			return cp && p && cp == p;
		}

		template<typename C>
		bool has_full_callable(C const& c) const {
			if constexpr (function_traits<C>::must_check_object) {
				return has_callable(c) && check_class_type<std::decay_t<C>>();
			} else {
				return has_callable(c);
			}
		}

		// check if we are storing object o
		template<typename O>
		bool has_object(O const& o) const {
			return get_object() == get_object_ptr(o);
		}

	protected:
		void do_disconnect() final {
			cleaner.clean(this);
		}

		// retrieve a pointer to the object embedded in the slot
		virtual obj_ptr get_object() const {
			return nullptr;
		}

		// retrieve a pointer to the callable embedded in the slot
		virtual function_pointer get_callable() const {
			return get_function_ptr(nullptr);
		}

		template<typename U>
		bool check_class_type() const {
			return false;
		}

	private:
		cleanable& cleaner;
	};

	template<typename Func, typename... Args>
	struct slot final : public basic_slot<Args...> {
		template<typename F, typename Gid>
		constexpr slot(cleanable& c, F&& f, Gid gid) : basic_slot<Args...>(c, gid), func { std::forward<F>(f) } {}

	protected:
		void call_slot(Args... args) override {
			func(std::forward<Args>(args)...);
		}

		function_pointer get_callable() const override {
			return get_function_ptr(func);
		}

	private:
		std::decay_t<Func> func;
	};

	template<typename Func, typename... Args>
	struct slot_extended final : public basic_slot<Args...> {
		template<typename F>
		constexpr slot_extended(cleanable& c, F&& f, group_id gid) : basic_slot<Args...>(c, gid), func { std::forward<F>(f) } {}

		connection conn;

	protected:
		void call_slot(Args... args) override {
			func(conn, std::forward<Args>(args)...);
		}

		function_pointer get_callable() const override {
			return get_function_ptr(func);
		}

	private:
		std::decay_t<Func> func;
	};

	template<typename Pmf, typename Ptr, typename... Args>
	struct slot_pmf final : public basic_slot<Args...> {
		template<typename F, typename P>
		constexpr slot_pmf(cleanable& c, F&& f, P&& p, group_id gid)
			: basic_slot<Args...>(c, gid), pmf { std::forward<F>(f) }, ptr { std::forward<P>(p) } {}

	protected:
		void call_slot(Args... args) override {
			((*ptr).*pmf)(std::forward<Args>(args)...);
		}

		function_pointer get_callable() const override {
			return get_function_ptr(pmf);
		}

		obj_ptr get_object() const override {
			return get_object_ptr(ptr);
		}

	private:
		std::decay_t<Pmf> pmf;
		std::decay_t<Ptr> ptr;
	};

	template<typename Pmf, typename Ptr, typename... Args>
	struct slot_pmf_extended final : public basic_slot<Args...> {
		template<typename F, typename P>
		constexpr slot_pmf_extended(cleanable& c, F&& f, P&& p, group_id gid)
			: basic_slot<Args...>(c, gid), pmf { std::forward<F>(f) }, ptr { std::forward<P>(p) } {}

		connection conn;

	protected:
		void call_slot(Args... args) override {
			((*ptr).*pmf)(conn, std::forward<Args>(args)...);
		}

		function_pointer get_callable() const override {
			return get_function_ptr(pmf);
		}
		obj_ptr get_object() const override {
			return get_object_ptr(ptr);
		}

	private:
		std::decay_t<Pmf> pmf;
		std::decay_t<Ptr> ptr;
	};

	template<typename Func, typename WeakPtr, typename... Args>
	struct slot_tracked final : public basic_slot<Args...> {
		template<typename F, typename P>
		constexpr slot_tracked(cleanable& c, F&& f, P&& p, group_id gid)
			: basic_slot<Args...>(c, gid), func { std::forward<F>(f) }, ptr { std::forward<P>(p) } {}

		bool connected() const override {
			return !ptr.expired() && slot_state::connected();
		}

	protected:
		void call_slot(Args... args) override {
			auto sp = ptr.lock();
			if (!sp) {
				slot_state::disconnect();
				return;
			}
			if (slot_state::connected()) {
				func(std::forward<Args>(args)...);
			}
		}

		function_pointer get_callable() const override {
			return get_function_ptr(func);
		}

		obj_ptr get_object() const override {
			return get_object_ptr(ptr);
		}

	private:
		std::decay_t<Func> func;
		std::decay_t<WeakPtr> ptr;
	};

	template<typename Pmf, typename WeakPtr, typename... Args>
	struct slot_pmf_tracked final : public basic_slot<Args...> {
		template<typename F, typename P>
		constexpr slot_pmf_tracked(cleanable& c, F&& f, P&& p, group_id gid)
			: basic_slot<Args...>(c, gid), pmf { std::forward<F>(f) }, ptr { std::forward<P>(p) } {}

		bool connected() const override {
			return !ptr.expired() && slot_state::connected();
		}

	protected:
		void call_slot(Args... args) override {
			auto sp = ptr.lock();
			if (!sp) {
				slot_state::disconnect();
				return;
			}
			if (slot_state::connected()) {
				((*sp).*pmf)(std::forward<Args>(args)...);
			}
		}

		function_pointer get_callable() const override {
			return get_function_ptr(pmf);
		}

		obj_ptr get_object() const override {
			return get_object_ptr(ptr);
		}

	private:
		std::decay_t<Pmf> pmf;
		std::decay_t<WeakPtr> ptr;
	};

	template<typename Func, typename... Args>
	struct is_member_callable : std::false_type {};

	template<typename Func, typename Self, typename... Args>
	struct is_member_callable<Func, Self, Args...> {
		static constexpr bool value = MemberCallable<Func, Self, Args...>;
	};

	template<typename Func, typename... T>
	concept Callable = FreeCallable<Func, T...> || is_member_callable<Func, T...>::value;

	template<typename Lockable, typename... Args>
	class basic_signal final : public cleanable {
		template<typename L>
		using is_thread_safe = std::bool_constant<!std::same_as<L, null_mutex>>;

		using lock_type = std::unique_lock<Lockable>;
		using slot_base = basic_slot<Args...>;
		using slot_ptr = _detail::signal::slot_ptr<Args...>;
		using slots_type = memory::vector<slot_ptr>;
		struct group_type {
			slots_type slts;
			group_id gid;
		};
		using list_type = memory::vector<group_type>; // kept ordered by ascending gid

		template<typename L>
		using cow_type = std::conditional_t<
			is_thread_safe<L>::value, cow_vector<typename list_type::value_type, typename list_type::allocator_type>,
			list_type>;

		template<typename L>
		using cow_copy_type = std::conditional_t<
			is_thread_safe<L>::value, cow_vector<typename list_type::value_type, typename list_type::allocator_type>,
			list_type const&>;

	public:
		using arg_list = std::tuple<Args...>;
		using ext_arg_list = std::tuple<connection&, Args...>;

		basic_signal() : is_blocked(false) {}
		~basic_signal() override {
			disconnect_all();
		}

		basic_signal(basic_signal const&) = delete;
		basic_signal& operator=(basic_signal const&) = delete;

		basic_signal(basic_signal&& other) : is_blocked { other.is_blocked.load() } {
			lock_type lock(other.mutex);
			using std::swap;
			swap(slots, other.slots);
		}

		basic_signal& operator=(basic_signal&& other) {
			lock_type lock1(mutex, std::defer_lock);
			lock_type lock2(other.mutex, std::defer_lock);
			std::lock(lock1, lock2);

			using std::swap;
			swap(slots, other.slots);
			is_blocked.store(other.is_blocked.exchange(is_blocked.load()));
			return *this;
		}

		/**
		 * Emit a signal
		 *
		 * Effect: All non blocked and connected slot functions will be called
		 *         with supplied arguments.
		 * Safety: With proper locking (see pal::signal), emission can happen from
		 *         multiple threads simultaneously. The guarantees only apply to the
		 *         signal object, it does not cover thread safety of potentially
		 *         shared state used in slot functions.
		 *
		 * @param a... arguments to emit
		 */
		template<typename... U>
		void operator()(U&&... a) const {
			if (is_blocked) {
				return;
			}

			// Reference to the slots to execute them out of the lock
			// a copy may occur if another thread writes to it.
			cow_copy_type<Lockable> ref = slots_reference();

			size_t index = 0;
			for (group_type const& g : cow::read(ref)) {
				index += g.slts.size();
			}

			for (group_type const& group : cow::read(ref)) {
				for (slot_ptr const& s : group.slts) {
					index--;
					if (OV_unlikely(index == 0)) {
						s->operator()(std::forward<U>(a)...);
						break;
					}

					s->operator()(a...);
				}
			}
		}

		/**
		 * Connect a callable of compatible arguments
		 *
		 * Effect: Creates and stores a new slot responsible for executing the
		 *         supplied callable for every subsequent signal emission.
		 * Safety: Thread-safety depends on locking policy.
		 *
		 * @param c a callable
		 * @param gid an identifier that can be used to order slot execution
		 * @return a connection object that can be used to interact with the slot
		 */
		template<Callable<Args...> Callable>
		connection connect(Callable&& c, group_id gid = 0) {
			using slot_t = slot<Callable, Args...>;
			std::shared_ptr<slot_base> s = make_slot<slot_t>(std::forward<Callable>(c), gid);
			connection conn(s);
			add_slot(std::move(s));
			return conn;
		}

		/**
		 * Connect a callable with an additional connection argument
		 *
		 * The callable's first argument must be of type connection. This overload
		 * the callable to manage it's own connection through this argument.
		 *
		 * @param c a callable
		 * @param gid an identifier that can be used to order slot execution
		 * @return a connection object that can be used to interact with the slot
		 */
		template<Callable<connection&, Args...> Callable>
		connection connect_extended(Callable&& c, group_id gid = 0) {
			using slot_t = slot_extended<Callable, Args...>;
			std::shared_ptr<slot_base> s = make_slot<slot_t>(std::forward<Callable>(c), gid);
			connection conn(s);
			std::static_pointer_cast<slot_t>(s)->conn = conn;
			add_slot(std::move(s));
			return conn;
		}

		/**
		 * Overload of connect for pointers over member functions derived from
		 * observer
		 *
		 * @param pmf a pointer over member function
		 * @param ptr an object pointer derived from observer
		 * @param gid an identifier that can be used to order slot execution
		 * @return a connection object that can be used to interact with the slot
		 */
		template<typename Pmf, Observer Ptr>
		requires Callable<Pmf, Ptr, Args...>
		connection connect(Pmf&& pmf, Ptr&& ptr, group_id gid = 0) {
			using slot_t = slot_pmf<Pmf, Ptr, Args...>;
			std::shared_ptr<slot_base> s = make_slot<slot_t>(std::forward<Pmf>(pmf), std::forward<Ptr>(ptr), gid);
			connection conn(s);
			add_slot(std::move(s));
			ptr->add_connection(conn);
			return conn;
		}

		/**
		 * Overload of connect for pointers over member functions
		 *
		 * @param pmf a pointer over member function
		 * @param ptr an object pointer
		 * @param gid an identifier that can be used to order slot execution
		 * @return a connection object that can be used to interact with the slot
		 */
		template<typename Pmf, typename Ptr>
		requires Callable<Pmf, Ptr, Args...> && (!Observer<Ptr> && !WeakPtrCompatible<Ptr>)
		connection connect(Pmf&& pmf, Ptr&& ptr, group_id gid = 0) {
			using slot_t = slot_pmf<Pmf, Ptr, Args...>;
			std::shared_ptr<slot_base> s = make_slot<slot_t>(std::forward<Pmf>(pmf), std::forward<Ptr>(ptr), gid);
			connection conn(s);
			add_slot(std::move(s));
			return conn;
		}

		/**
		 * Overload  of connect for pointer over member functions and
		 *
		 * @param pmf a pointer over member function
		 * @param ptr an object pointer
		 * @param gid an identifier that can be used to order slot execution
		 * @return a connection object that can be used to interact with the slot
		 */
		template<typename Pmf, typename Ptr>
		requires Callable<Pmf, Ptr, connection&, Args...> && (!WeakPtrCompatible<Ptr>)
		connection connect_extended(Pmf&& pmf, Ptr&& ptr, group_id gid = 0) {
			using slot_t = slot_pmf_extended<Pmf, Ptr, Args...>;
			std::shared_ptr<slot_base> s = make_slot<slot_t>(std::forward<Pmf>(pmf), std::forward<Ptr>(ptr), gid);
			connection conn(s);
			std::static_pointer_cast<slot_t>(s)->conn = conn;
			add_slot(std::move(s));
			return conn;
		}

		/**
		 * Overload of connect for lifetime object tracking and automatic disconnection
		 *
		 * Ptr must be convertible to an object following a loose form of weak pointer
		 * concept, by implementing the ADL-detected conversion function to_weak().
		 *
		 * This overload covers the case of a pointer over member function and a
		 * trackable pointer of that class.
		 *
		 * Note: only weak references are stored, a slot does not extend the lifetime
		 * of a supplied object.
		 *
		 * @param pmf a pointer over member function
		 * @param ptr a trackable object pointer
		 * @param gid an identifier that can be used to order slot execution
		 * @return a connection object that can be used to interact with the slot
		 */
		template<typename Pmf, typename Ptr>
		requires(!Callable<Pmf, Args...>) && WeakPtrCompatible<Ptr>
		connection connect(Pmf&& pmf, Ptr&& ptr, group_id gid = 0) {
			std::weak_ptr w = to_weak(std::forward<Ptr>(ptr));
			using slot_t = slot_pmf_tracked<Pmf, decltype(w), Args...>;
			std::shared_ptr<slot_base> s = make_slot<slot_t>(std::forward<Pmf>(pmf), w, gid);
			connection conn(s);
			add_slot(std::move(s));
			return conn;
		}

		/**
		 * Overload of connect for lifetime object tracking and automatic disconnection
		 *
		 * Trackable must be convertible to an object following a loose form of weak
		 * pointer concept, by implementing the ADL-detected conversion function to_weak().
		 *
		 * This overload covers the case of a standalone callable and unrelated trackable
		 * object.
		 *
		 * Note: only weak references are stored, a slot does not extend the lifetime
		 * of a supplied object.
		 *
		 * @param c a callable
		 * @param ptr a trackable object pointer
		 * @param gid an identifier that can be used to order slot execution
		 * @return a connection object that can be used to interact with the slot
		 */
		template<Callable<Args...> Callable, WeakPtrCompatible Trackable>
		connection connect(Callable&& c, Trackable&& ptr, group_id gid = 0) {
			std::weak_ptr w = to_weak(std::forward<Trackable>(ptr));
			using slot_t = slot_tracked<Callable, decltype(w), Args...>;
			std::shared_ptr<slot_base> s = make_slot<slot_t>(std::forward<Callable>(c), w, gid);
			connection conn(s);
			add_slot(std::move(s));
			return conn;
		}

		/**
		 * Creates a connection whose duration is tied to the return object
		 * Use the same semantics as connect
		 */
		template<typename... CallArgs>
		scoped_connection connect_scoped(CallArgs&&... args) {
			return connect(std::forward<CallArgs>(args)...);
		}

		/**
		 * Disconnect slots bound to a callable
		 *
		 * Effect: Disconnects all the slots bound to the callable in argument.
		 * Safety: Thread-safety depends on locking policy.
		 *
		 * If the callable is a free or static member function, this overload is always
		 * available. However, RTTI is needed for it to work for pointer to member
		 * functions, function objects or and (references to) lambdas, because the
		 * C++ spec does not mandate the pointers to member functions to be unique.
		 *
		 * @param c a callable
		 * @return the number of disconnected slots
		 */
		template<typename C>
		requires(
			(Callable<C, Args...> || Callable<C, connection&, Args...> || MemberFunctionPointer<C>) &&
			function_traits<C>::is_disconnectable
		)
		size_t disconnect(C const& c) {
			return disconnect_if([&](slot_ptr const& s) {
				return s->has_full_callable(c);
			});
		}

		/**
		 * Disconnect slots bound to this object
		 *
		 * Effect: Disconnects all the slots bound to the object or tracked object
		 *         in argument.
		 * Safety: Thread-safety depends on locking policy.
		 *
		 * The object may be a pointer or trackable object.
		 *
		 * @param obj an object
		 * @return the number of disconnected slots
		 */
		template<typename Obj>
		requires(!Callable<Obj, Args...> && !Callable<Obj, connection&, Args...> && !MemberFunctionPointer<Obj>)
		size_t disconnect(Obj const& obj) {
			return disconnect_if([&](slot_ptr const& s) {
				return s->has_object(obj);
			});
		}

		/**
		 * Disconnect slots bound both to a callable and object
		 *
		 * Effect: Disconnects all the slots bound to the callable and object in argument.
		 * Safety: Thread-safety depends on locking policy.
		 *
		 * For naked pointers, the Callable is expected to be a pointer over member
		 * function. If obj is trackable, any kind of Callable can be used.
		 *
		 * @param c a callable
		 * @param obj an object
		 * @return the number of disconnected slots
		 */
		template<typename Callable, typename Obj>
		size_t disconnect(Callable const& c, Obj const& obj) {
			return disconnect_if([&](slot_ptr const& s) {
				return s->has_object(obj) && s->has_callable(c);
			});
		}

		/**
		 * Disconnect slots in a particular group
		 *
		 * Effect: Disconnects all the slots in the group id in argument.
		 * Safety: Thread-safety depends on locking policy.
		 *
		 * @param gid a group id
		 * @return the number of disconnected slots
		 */
		size_t disconnect(group_id gid) {
			lock_type lock(mutex);
			for (group_type& group : cow::write(slots)) {
				if (group.gid == gid) {
					size_t count = group.slts.size();
					group.slts.clear();
					return count;
				}
			}
			return 0;
		}

		/**
		 * Disconnects all the slots
		 * Safety: Thread safety depends on locking policy
		 */
		void disconnect_all() {
			lock_type lock(mutex);
			clear();
		}

		/**
		 * Disconnects all the slots with nullptr objects
		 * Safety: Thread safety depends on locking policy
		 */
		void disconnect_all_dead() {
			disconnect_if([&](slot_ptr const& s) {
				return s->has_object(nullptr);
			});
		}

		/**
		 * Blocks signal emission
		 * Safety: thread safe
		 */
		void block() {
			is_blocked.store(true);
		}

		/**
		 * Blocks all slots in a given group
		 * Safety: thread safe
		 */
		void block(group_id const& gid) {
			lock_type lock(mutex);
			for (group_type& group : cow::write(slots)) {
				if (group.gid == gid) {
					for (slot_ptr& slt : group.slts) {
						slt->block();
					}
				}
			}
		}

		/**
		 * Unblocks signal emission
		 * Safety: thread safe
		 */
		void unblock() {
			is_blocked.store(false);
		}

		/**
		 * Unblocks all slots in a given group
		 * Safety: thread safe
		 */
		void unblock(group_id const& gid) {
			lock_type lock(mutex);
			for (group_type& group : cow::write(slots)) {
				if (group.gid == gid) {
					for (slot_ptr& slt : group.slts) {
						slt->unblock();
					}
				}
			}
		}

		/**
		 * Tests blocking state of signal emission
		 */
		bool blocked() const {
			return is_blocked.load();
		}

		/**
		 * Get number of connected slots
		 * Safety: thread safe
		 */
		size_t slot_count() {
			cow_copy_type<Lockable> ref = slots_reference();
			size_t count = 0;
			for (group_type const& g : cow::read(ref)) {
				count += g.slts.size();
			}
			return count;
		}

	protected:
		/**
		 * remove disconnected slots
		 */
		void clean(slot_state* state) override {
			lock_type lock(mutex);
			const size_t& idx = state->index();
			const group_id gid = state->group();

			// find the group
			for (group_type& group : cow::write(slots)) {
				if (group.gid == gid) {
					slots_type& slts = group.slts;

					// ensure we have the right slot, in case of concurrent cleaning
					if (idx < slts.size() && slts[idx] && slts[idx].get() == state) {
						std::swap(slts[idx], slts.back());
						slts[idx]->index() = idx;
						slts.pop_back();
					}

					return;
				}
			}
		}

	private:
		// used to get a reference to the slots for reading
		inline cow_copy_type<Lockable> slots_reference() const {
			lock_type lock(mutex);
			return slots;
		}

		// create a new slot
		template<typename Slot, typename... A>
		inline std::shared_ptr<slot_base> make_slot(A&&... a) {
			return make_shared<slot_base, Slot>(*this, std::forward<A>(a)...);
		}

		// add the slot to the list of slots of the right group
		void add_slot(slot_ptr&& s) {
			const group_id gid = s->group();

			lock_type lock(mutex);
			auto& groups = cow::write(slots);

			// find the group
			typename std::remove_reference_t<decltype(groups)>::iterator it = groups.begin();
			while (it != groups.end() && it->gid < gid) {
				it++;
			}

			// create a new group if necessary
			if (it == groups.end() || it->gid != gid) {
				it = groups.insert(it, { {}, gid });
			}

			// add the slot
			s->index() = it->slts.size();
			it->slts.push_back(std::move(s));
		}

		// disconnect a slot if a condition occurs
		template<typename Cond>
		size_t disconnect_if(Cond&& cond) {
			lock_type lock(mutex);
			auto& groups = cow::write(slots);

			size_t count = 0;

			for (group_type& group : groups) {
				slots_type& slts = group.slts;
				size_t i = 0;
				while (i < slts.size()) {
					if (cond(slts[i])) {
						std::swap(slts[i], slts.back());
						slts[i]->index() = i;
						slts.pop_back();
						++count;
					} else {
						++i;
					}
				}
			}

			return count;
		}

		// to be called under lock: remove all the slots
		void clear() {
			cow::write(slots).clear();
		}

	private:
		mutable Lockable mutex;
		cow_type<Lockable> slots;
		std::atomic_bool is_blocked;
	};

	template<typename Sig, typename... Args>
	concept HasConnect = requires(Sig sig, Args&&... args) {
		{ sig.connect(std::forward<Args>(args)...) } -> std::same_as<connection>;
	};
	template<typename Sig, typename... Args>
	concept HasConnectExtended = requires(Sig sig, Args&&... args) {
		{ sig.connect_extended(std::forward<Args>(args)...) } -> std::same_as<connection>;
	};

	template<typename Sig, typename... Args>
	concept HasDisconnect = requires(Sig sig, Args&&... args) {
		{ sig.disconnect(std::forward<Args>(args)...) } -> std::same_as<size_t>;
	};

	template<template<typename...> typename Signal, typename Owner, typename... Args>
	class basic_signal_property final {
		using signal_type = Signal<Args...>;
		std::optional<signal_type> storage;
		signal_type* signal;
		friend Owner;

		template<typename... U>
		void operator()(U&&... args) {
			(*signal)(std::forward<U>(args)...);
		}

		size_t slot_count() {
			return signal->slot_count();
		}

		void block() {
			signal->block();
		}

		void block(group_id const& gid) {
			signal->block(gid);
		}

		void unblock() {
			signal->unblock();
		}

		void unblock(group_id const& gid) {
			signal->unblock(gid);
		}

		bool blocked() const {
			return signal->blocked();
		}

		basic_signal_property(basic_signal_property&& other) {
			if (other.storage.has_value()) {
				storage = std::move(other.storage);
				signal = std::addressof(*storage);
				other.signal = nullptr;
			} else {
				std::swap(signal, other.signal);
			}
		}

		basic_signal_property& operator=(basic_signal_property&& other) {
			if (signal != other.signal) {
				if (other.storage.has_value()) {
					storage = std::move(other.storage);
					signal = std::addressof(*storage);
					other.signal = nullptr;
				} else {
					std::swap(signal, other.signal);
				}
			}
			return *this;
		}

		~basic_signal_property() = default;

	public:
		basic_signal_property() : storage(std::in_place), signal(std::addressof(*storage)) {}

		explicit basic_signal_property(signal_type* sig) : storage(std::nullopt), signal(sig) {}

		basic_signal_property(basic_signal_property const&) = delete;
		basic_signal_property& operator=(basic_signal_property const&) = delete;

		template<typename... Ts>
		requires HasConnect<signal_type, Ts...>
		connection connect(Ts&&... args) {
			return signal->connect(std::forward<Ts>(args)...);
		}

		template<typename... Ts>
		requires HasConnectExtended<signal_type, Ts...>
		connection connect_extended(Ts&&... args) {
			return signal->connect_extended(std::forward<Ts>(args)...);
		}

		template<typename... Ts>
		requires HasConnect<signal_type, Ts...>
		scoped_connection connect_scoped(Ts&&... args) {
			return signal->connect(std::forward<Ts>(args)...);
		}

		template<typename... Ts>
		requires HasDisconnect<signal_type, Ts...>
		size_t disconnect(Ts&&... args) {
			return signal->disconnect(std::forward<Ts>(args)...);
		}

		void disconnect_all() {
			signal->disconnect_all();
		}
	};
}

namespace OpenVic {
	template<typename Lockable, typename Arg, typename... T, typename... Args>
	requires(!_detail::signal::Signal<std::decay_t<Arg>>)
	connection connect(basic_signal<Lockable, T...>& sig, Arg&& arg, Args&&... args) {
		return sig.connect(std::forward<Arg>(arg), std::forward<Args>(args)...);
	}

	template<typename Lockable1, typename Lockable2, typename... T1, typename... T2, typename... Args>
	connection connect(basic_signal<Lockable1, T1...>& sig1, basic_signal<Lockable2, T2...>& sig2, Args&&... args) {
		return sig1.connect(
			_detail::signal::signal_wrapper<basic_signal<Lockable2, T2...>> { std::addressof(sig2) },
			std::forward<Args>(args)...
		);
	}
}

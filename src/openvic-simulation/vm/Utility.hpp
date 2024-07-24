#pragma once

namespace OpenVic::Vm::utility {
	template<typename T>
	struct HandleBase {
		HandleBase(T* handle) : _handle(handle) {}

		T* handle() {
			return this->_handle;
		}

		const T* handle() const {
			return this->_handle;
		}

		operator T*() {
			return this->_handle;
		}

		operator const T*() const {
			return this->_handle;
		}

	protected:
		T* _handle;
	};

	template<typename Self, typename T>
	struct MoveOnlyHandleBase : HandleBase<T> {
		MoveOnlyHandleBase(Self&& other) : HandleBase<T>(other._handle) {
			other._handle = nullptr;
		}

		Self& operator=(Self&& other) {
			this->_handle = other._handle;
			other._handle = nullptr;
			return *this;
		}

		MoveOnlyHandleBase(Self const&) = delete;
		Self& operator=(Self const&) = delete;

	protected:
		using HandleBase<T>::HandleBase;
	};

	template<typename Self, typename Derived>
	struct MoveOnlyHandleDerived : Derived {
		using Derived::Derived;

		MoveOnlyHandleDerived(Self&& other) : Derived(other._handle) {
			other._handle = nullptr;
		}

		Self& operator=(Self&& other) {
			this->_handle = other._handle;
			other._handle = nullptr;
			return *this;
		}

		MoveOnlyHandleDerived(Self const&) = delete;
		Self& operator=(Self const&) = delete;

		Derived& as_ref() {
			return *this;
		}

		Derived const& as_ref() const {
			return *this;
		}
	};
}

#pragma once

namespace OpenVic::Vm {
	template<typename T>
	struct HandlerRef {
		HandlerRef(T* handle) : _handle(handle) {}

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
	struct UniqueHandler : HandlerRef<T> {
		UniqueHandler(Self&& other) : HandlerRef<T>(other._handle) {
			other._handle = nullptr;
		}

		Self& operator=(Self&& other) {
			this->_handle = other._handle;
			other._handle = nullptr;
			return *this;
		}

		UniqueHandler(Self const&) = delete;
		Self& operator=(Self const&) = delete;

	protected:
		using HandlerRef<T>::HandlerRef;
	};

	template<typename Self, typename RefType>
	struct UniqueHandlerDerived : RefType {
		using RefType::RefType;

		UniqueHandlerDerived(Self&& other) : RefType(other._handle) {
			other._handle = nullptr;
		}

		Self& operator=(Self&& other) {
			this->_handle = other._handle;
			other._handle = nullptr;
			return *this;
		}

		UniqueHandlerDerived(Self const&) = delete;
		Self& operator=(Self const&) = delete;

		RefType& as_ref() {
			return *this;
		}

		RefType const& as_ref() const {
			return *this;
		}

		operator RefType&() {
			return as_ref();
		}

		operator RefType const&() const {
			return as_ref();
		}
	};
}

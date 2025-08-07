# Container types
This file describes the various C++ types used to store items.
Use this to pick between container types.

## Sequence containers
Sequence containers are array/list-like. They consist of a sequence of items.
- array<T,SIZE> / C-array
- vector<T>
- cow_vector<T>
- FixedVector<T>
- colony<T>
- deque<T>
- stack<T>
- queue<T>

### array<T,SIZE> / C-array
[Official documentation](https://en.cppreference.com/w/cpp/container/array.html)
Usage: `std::array<T, SIZE> values;`
Requires:
	- T must be MoveConstructible and MoveAssignable.
	- SIZE must be compile-time constant.
Capacity: fixed
Size: fixed
This is stored as a range of addresses on the stack.

Note `std::array<int, SIZE> values;` does not initialise values, use `std::array<int, SIZE> values {};` instead to set values to 0;

### vector<T>
[Official documentation](https://en.cppreference.com/w/cpp/container/vector.html)
Usage: `memory::vector<T> values;`
Requires:
	- T must be moveable (for reallocations).
Capacity: expandable and shrinkable
Size: expandable and shrinkable
Note `vector<bool>` acts as a bit set, see [official documentation](https://en.cppreference.com/w/cpp/container/vector_bool.html).

### cow_vector<T>
Source: src\openvic-simulation\types\CowVector.hpp
Usage: `memory::cow_vector<T> values;`
Requires:
	- T must be moveable (for reallocations).
Capacity: expandable and shrinkable
Size: expandable and shrinkable
Used in src\openvic-simulation\types\Signal.hpp.
Note 'cow' stands for copy-on-write.

### FixedVector<T>
Source: src\openvic-simulation\types\FixedVector.hpp
Usage: `memory::FixedVector<T> values(capacity);`
Requires:
	- Runtime constant capacity in the constructor.
Capacity: fixed
Size: expandable (up to capacity) and shrinkable
Used in src\openvic-simulation\types\IndexedFlatMap.hpp for non-movable types.

## Associative container
Associative containers are map/dictionary-like. They consist of unique keys with their associated values.
- IndexedFlatMap<Key,Value>
- ordered_map<Key,Value>
- unordered_map<Key,Value>
- IdentifierRegistry<Value>
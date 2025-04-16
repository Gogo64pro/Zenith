//
// Created by gogop on 4/3/2025.
//

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>

template <typename T, size_t N>
class small_vector {
	static_assert(N > 0, "small_vector requires a positive inline capacity");

	// Buffer for small-size optimization
	alignas(T) char inline_buffer_[N * sizeof(T)];
	T* data_ = reinterpret_cast<T*>(inline_buffer_);
	size_t size_ = 0;
	size_t capacity_ = N;

	// Switch to heap allocation when needed
	void grow(size_t new_capacity);

public:
	using value_type = T;
	using size_type = size_t;
	using reference = T&;
	using const_reference = const T&;
	using iterator = T*;
	using const_iterator = const T*;

	small_vector() = default;
	~small_vector();

	// Copy constructor
	small_vector(const small_vector& other);

	// Move constructor
	small_vector(small_vector&& other) noexcept;

	// Copy assignment
	small_vector& operator=(const small_vector& other);

	// Move assignment
	small_vector& operator=(small_vector&& other) noexcept;

	void swap(small_vector& other) noexcept;

	// Element access
	reference operator[](size_t pos);
	const_reference operator[](size_t pos) const;

	reference front();
	const_reference front() const;

	reference back();
	const_reference back() const;

	T* data() noexcept;
	const T* data() const noexcept;

	// Iterators
	iterator begin() noexcept;
	const_iterator begin() const noexcept;
	const_iterator cbegin() const noexcept;

	iterator end() noexcept;
	const_iterator end() const noexcept;
	const_iterator cend() const noexcept;

	// Capacity
	bool empty() const noexcept;
	size_t size() const noexcept;
	size_t capacity() const noexcept;

	void reserve(size_t new_capacity);
	void shrink_to_fit();

	// Modifiers
	void clear() noexcept;
	void push_back(const T& value);
	void push_back(T&& value);

	template <typename... Args>
	reference emplace_back(Args&&... args);

	void pop_back();
	void resize(size_t new_size);
	void resize(size_t new_size, const T& value);

	template <typename Allocator>
	small_vector(std::vector<T, Allocator>&& other);

	template <typename Allocator>
	small_vector& operator=(std::vector<T, Allocator>&& other);

	template <typename InputIt>
	void append(InputIt first, InputIt last);

	template <typename... Args>
	void emplace_back_bulk(Args&&... args);

	template <typename InputIt>
	small_vector(InputIt first, InputIt last);

	void swap_range(size_t pos, small_vector& other, size_t other_pos, size_t count);

	iterator erase_range(const_iterator first, const_iterator last);
};

//C++ templates...

template <typename T, size_t N>
void small_vector<T, N>::grow(size_t new_capacity) {
	assert(new_capacity > capacity_);

	T* new_data = static_cast<T*>(::operator new(new_capacity * sizeof(T)));
	size_t i = 0;

	try {
		for (; i < size_; ++i) {
			new (new_data + i) T(std::move(data_[i]));
		}
	} catch (...) {
		for (size_t j = 0; j < i; ++j) {
			new_data[j].~T();
		}
		::operator delete(new_data);
		throw;
	}

	// Destroy old elements
	for (size_t j = 0; j < size_; ++j) {
		data_[j].~T();
	}

	// If we were using heap memory before, free it
	if (data_ != reinterpret_cast<T*>(inline_buffer_)) {
		::operator delete(data_);
	}

	data_ = new_data;
	capacity_ = new_capacity;
}

template <typename T, size_t N>
small_vector<T, N>::~small_vector() {
	for (size_t i = 0; i < size_; ++i) {
		data_[i].~T();
	}
	if (data_ != reinterpret_cast<T*>(inline_buffer_)) {
		::operator delete(data_);
	}
}

template <typename T, size_t N>
small_vector<T, N>::small_vector(const small_vector& other) {
	reserve(other.size_);
	size_ = other.size_;
	for (size_t i = 0; i < size_; ++i) {
		new (data_ + i) T(other.data_[i]);
	}
}

template <typename T, size_t N>
small_vector<T, N>::small_vector(small_vector&& other) noexcept {
	if (other.data_ == reinterpret_cast<T*>(other.inline_buffer_)) {
		// Other is using inline storage - move elements one by one
		reserve(other.size_);
		size_ = other.size_;
		for (size_t i = 0; i < size_; ++i) {
			new (data_ + i) T(std::move(other.data_[i]));
		}
	} else {
		// Other is using heap storage - take ownership
		data_ = other.data_;
		size_ = other.size_;
		capacity_ = other.capacity_;

		// Leave other in valid state with inline storage
		other.data_ = reinterpret_cast<T*>(other.inline_buffer_);
		other.size_ = 0;
		other.capacity_ = N;
	}
}

template <typename T, size_t N>
small_vector<T, N>& small_vector<T, N>::operator=(const small_vector& other) {
	if (this != &other) {
		small_vector tmp(other);
		swap(tmp);
	}
	return *this;
}

template <typename T, size_t N>
small_vector<T, N>& small_vector<T, N>::operator=(small_vector&& other) noexcept {
	if (this != &other) {
		small_vector tmp(std::move(other));
		swap(tmp);
	}
	return *this;
}

template <typename T, size_t N>
void small_vector<T, N>::swap(small_vector& other) noexcept {
	using std::swap;

	if (this == &other) return;

	// Case 1: Both use inline storage
	if (data_ == reinterpret_cast<T*>(inline_buffer_) &&
	    other.data_ == reinterpret_cast<T*>(other.inline_buffer_)) {

		// Swap the active elements
		for (size_t i = 0; i < std::max(size_, other.size_); ++i) {
			if (i < size_ && i < other.size_) {
				swap(data_[i], other.data_[i]);
			} else if (i < size_) {
				// This has more elements - move to other
				new (other.data_ + i) T(std::move(data_[i]));
				data_[i].~T();
			} else {
				// Other has more elements - move to this
				new (data_ + i) T(std::move(other.data_[i]));
				other.data_[i].~T();
			}
		}
		swap(size_, other.size_);
		// Capacity stays N for both
	}
		// Case 2: This uses heap, other uses inline
	else if (other.data_ == reinterpret_cast<T*>(other.inline_buffer_)) {
		// Move other's elements to this's heap
		// Then take ownership of this's heap
		small_vector tmp;
		tmp.reserve(other.size_);
		for (size_t i = 0; i < other.size_; ++i) {
			new (tmp.data_ + i) T(std::move(other.data_[i]));
			other.data_[i].~T();
		}
		tmp.size_ = other.size_;

		// Now put this's heap into other
		other.data_ = data_;
		other.size_ = size_;
		other.capacity_ = capacity_;

		// And take tmp's inline buffer
		data_ = reinterpret_cast<T*>(inline_buffer_);
		size_ = tmp.size_;
		capacity_ = N;
		for (size_t i = 0; i < size_; ++i) {
			new (data_ + i) T(std::move(tmp.data_[i]));
			tmp.data_[i].~T();
		}
	}
		// Case 3: Other uses heap, this uses inline
	else if (data_ == reinterpret_cast<T*>(inline_buffer_)) {
		other.swap(*this); // Use the previous case
	}
		// Case 4: Both use heap
	else {
		swap(data_, other.data_);
		swap(size_, other.size_);
		swap(capacity_, other.capacity_);
	}
}

template <typename T, size_t N>
typename small_vector<T, N>::reference small_vector<T, N>::operator[](size_t pos) {
	assert(pos < size_);
	return data_[pos];
}

template <typename T, size_t N>
typename small_vector<T, N>::const_reference small_vector<T, N>::operator[](size_t pos) const {
	assert(pos < size_);
	return data_[pos];
}

template <typename T, size_t N>
typename small_vector<T, N>::reference small_vector<T, N>::front() {
	assert(size_ > 0);
	return data_[0];
}

template <typename T, size_t N>
typename small_vector<T, N>::const_reference small_vector<T, N>::front() const {
	assert(size_ > 0);
	return data_[0];
}

template <typename T, size_t N>
typename small_vector<T, N>::reference small_vector<T, N>::back() {
	assert(size_ > 0);
	return data_[size_ - 1];
}

template <typename T, size_t N>
typename small_vector<T, N>::const_reference small_vector<T, N>::back() const {
	assert(size_ > 0);
	return data_[size_ - 1];
}

template <typename T, size_t N>
T* small_vector<T, N>::data() noexcept {
	return data_;
}

template <typename T, size_t N>
const T* small_vector<T, N>::data() const noexcept {
	return data_;
}

template <typename T, size_t N>
typename small_vector<T, N>::iterator small_vector<T, N>::begin() noexcept {
	return data_;
}

template <typename T, size_t N>
typename small_vector<T, N>::const_iterator small_vector<T, N>::begin() const noexcept {
	return data_;
}

template <typename T, size_t N>
typename small_vector<T, N>::const_iterator small_vector<T, N>::cbegin() const noexcept {
	return data_;
}

template <typename T, size_t N>
typename small_vector<T, N>::iterator small_vector<T, N>::end() noexcept {
	return data_ + size_;
}

template <typename T, size_t N>
typename small_vector<T, N>::const_iterator small_vector<T, N>::end() const noexcept {
	return data_ + size_;
}

template <typename T, size_t N>
typename small_vector<T, N>::const_iterator small_vector<T, N>::cend() const noexcept {
	return data_ + size_;
}

template <typename T, size_t N>
bool small_vector<T, N>::empty() const noexcept {
	return size_ == 0;
}

template <typename T, size_t N>
size_t small_vector<T, N>::size() const noexcept {
	return size_;
}

template <typename T, size_t N>
size_t small_vector<T, N>::capacity() const noexcept {
	return capacity_;
}

template <typename T, size_t N>
void small_vector<T, N>::reserve(size_t new_capacity) {
	if (new_capacity > capacity_) {
		grow(new_capacity);
	}
}

template <typename T, size_t N>
void small_vector<T, N>::shrink_to_fit() {
	if (size_ == capacity_) return;

	if (size_ <= N) {
		// Switch back to inline storage if possible
		if (data_ != reinterpret_cast<T*>(inline_buffer_)) {
			small_vector tmp;
			for (size_t i = 0; i < size_; ++i) {
				tmp.push_back(std::move(data_[i]));
			}
			this->swap(tmp);
		}
	} else {
		// Reallocate to exactly fit the size
		grow(size_);
	}
}

template <typename T, size_t N>
void small_vector<T, N>::clear() noexcept {
	for (size_t i = 0; i < size_; ++i) {
		data_[i].~T();
	}
	size_ = 0;
}

template <typename T, size_t N>
void small_vector<T, N>::push_back(const T& value) {
	if (size_ == capacity_) {
		grow(capacity_ * 2);
	}
	new (data_ + size_) T(value);
	++size_;
}

template <typename T, size_t N>
void small_vector<T, N>::push_back(T&& value) {
	if (size_ == capacity_) {
		grow(capacity_ * 2);
	}
	new (data_ + size_) T(std::move(value));
	++size_;
}

template <typename T, size_t N>
void small_vector<T, N>::pop_back() {
	assert(size_ > 0);
	data_[size_ - 1].~T();
	--size_;
}

template <typename T, size_t N>
void small_vector<T, N>::resize(size_t new_size) {
	if (new_size < size_) {
		for (size_t i = new_size; i < size_; ++i) {
			data_[i].~T();
		}
	} else if (new_size > size_) {
		reserve(new_size);
		for (size_t i = size_; i < new_size; ++i) {
			new (data_ + i) T();
		}
	}
	size_ = new_size;
}

template <typename T, size_t N>
void small_vector<T, N>::resize(size_t new_size, const T& value) {
	if (new_size < size_) {
		for (size_t i = new_size; i < size_; ++i) {
			data_[i].~T();
		}
	} else if (new_size > size_) {
		reserve(new_size);
		for (size_t i = size_; i < new_size; ++i) {
			new (data_ + i) T(value);
		}
	}
	size_ = new_size;
}

template <typename T, size_t N>
template <typename Allocator>
small_vector<T, N>::small_vector(std::vector<T, Allocator>&& other) {
	if (other.size() <= N) {
		// Move elements one by one into inline storage
		size_ = other.size();
		capacity_ = N;
		for (size_t i = 0; i < size_; ++i) {
			new (data_ + i) T(std::move(other[i]));
		}
	} else {
		// Take ownership of the vector's heap allocation
		data_ = other.data();
		size_ = other.size();
		capacity_ = other.capacity();

		// Leave the source vector in valid but empty state
		other = std::vector<T, Allocator>(); // Reset to empty vector
	}
}

template <typename T, size_t N>
template <typename Allocator>
small_vector<T, N>& small_vector<T, N>::operator=(std::vector<T, Allocator>&& other) {
	small_vector tmp(std::move(other));
	swap(tmp);
	return *this;
}

template <typename T, size_t N>
template <typename InputIt>
void small_vector<T, N>::append(InputIt first, InputIt last) {
	const size_t count = std::distance(first, last);
	reserve(size_ + count);
	for (; first != last; ++first) {
		new (data_ + size_) T(*first);
		++size_;
	}
}

template <typename T, size_t N>
template <typename InputIt>
small_vector<T, N>::small_vector(InputIt first, InputIt last) {
	const size_t count = std::distance(first, last);
	if (count > N) {
		data_ = static_cast<T*>(::operator new(count * sizeof(T)));
		capacity_ = count;
	}
	size_ = count;
	std::uninitialized_copy(first, last, data_);
}

template <typename T, size_t N>
void small_vector<T, N>::swap_range(size_t pos, small_vector& other, size_t other_pos, size_t count) {
	assert(pos + count <= size_);
	assert(other_pos + count <= other.size_);

	for (size_t i = 0; i < count; ++i) {
		using std::swap;
		swap(data_[pos + i], other.data_[other_pos + i]);
	}
}

template <typename T, size_t N>
typename small_vector<T, N>::iterator small_vector<T, N>::erase_range(const_iterator first, const_iterator last) {
	const size_t start_pos = first - begin();
	const size_t end_pos = last - begin();
	const size_t count = end_pos - start_pos;

	// Move elements down
	for (size_t i = end_pos; i < size_; ++i) {
		data_[i - count] = std::move(data_[i]);
	}

	// Destroy leftover elements
	for (size_t i = size_ - count; i < size_; ++i) {
		data_[i].~T();
	}
	size_ -= count;
	return begin() + start_pos;
}

template <typename T, size_t N>
template <typename... Args>
typename small_vector<T, N>::reference small_vector<T, N>::emplace_back(Args&&... args) {
	if (size_ == capacity_) {
		grow(capacity_ * 2);
	}
	new (data_ + size_) T(std::forward<Args>(args)...);
	++size_;
	return data_[size_ - 1];
}

template <typename T, size_t N>
template <typename... Args>
void small_vector<T, N>::emplace_back_bulk(Args&&... args) {
	(emplace_back(std::forward<Args>(args)), ...);
}
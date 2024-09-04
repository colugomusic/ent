#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <list>
#include <numeric>
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ent {

template <typename... Ts>
struct table {
	auto resize(size_t size) -> void {
		if (size <= this->size()) {
			return;
		}
		(std::get<std::vector<Ts>>(data_).resize(size), ...);
	}
	auto push_back() -> size_t {
		const auto index = size();
		(std::get<std::vector<Ts>>(data_).emplace_back(), ...);
		return index;
	}
	auto is_valid(size_t index) const -> bool {
		return index < size();
	}
	auto size() const -> size_t { return std::get<0>(data_).size(); }
	template <typename T> [[nodiscard]]
	auto find(const T& value) const -> std::optional<size_t> {
		size_t index = 0;
		for (auto value_ : get<T>()) {
			if (value_ == value) {
				return index;
			};
			index++;
		}
		return std::nullopt;
	}
	template <typename T, typename Pred> [[nodiscard]]
	auto find(Pred&& pred) const -> std::optional<size_t> {
		size_t index = 0;
		for (auto value : get<T>()) {
			if (pred(value)) {
				return index;
			}
			index++;
		}
		return std::nullopt;
	}
	template <typename T>
	auto set(size_t index, T&& value) -> void {
		get<std::decay_t<T>>(index) = std::forward<T>(value);
	}
	template <typename T> [[nodiscard]] auto get() -> std::vector<T>& { return std::get<std::vector<T>>(data_); }
	template <typename T> [[nodiscard]] auto get() const -> const std::vector<T>& { return std::get<std::vector<T>>(data_); }
	template <typename T> [[nodiscard]] auto get(size_t index) -> T& { return std::get<std::vector<T>>(data_)[index]; }
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T& { return std::get<std::vector<T>>(data_)[index]; }
private:
	using Tuple = std::tuple<std::vector<Ts>...>;
	Tuple data_;
};

template <size_t BlockSize, typename... Ts>
struct sparse_block {
	[[nodiscard]]
	auto get_next() const {
		return next_;
	}
	auto set_next(sparse_block<BlockSize, Ts...>* next) -> void {
		next_ = next;
	}
	auto reset(size_t elem_index) -> void {
		(reset<Ts>(elem_index), ...);
	}
	auto clear() -> void {
		for (size_t i = 0; i < BlockSize; ++i) {
			reset(i);
		}
	}
	template <typename T>
	auto set(size_t elem_index, T&& value) -> T& {
		return get<std::decay_t<T>>(elem_index) = std::forward<T>(value);
	}
	template <typename T> [[nodiscard]] auto get(size_t elem_index) -> T&             { return std::get<std::array<T, BlockSize>>(data_)[elem_index % BlockSize]; }
	template <typename T> [[nodiscard]] auto get(size_t elem_index) const -> const T& { return std::get<std::array<T, BlockSize>>(data_)[elem_index % BlockSize]; }
private:
	template <typename T>
	auto reset(size_t elem_index) -> void {
		get<T>(elem_index % BlockSize) = T{};
	}
	using Tuple = std::tuple<std::array<Ts, BlockSize>...>;
	Tuple data_;
	sparse_block<BlockSize, Ts...>* next_ = nullptr;
};

template <size_t BlockSize, typename... Ts>
struct sparse_table {
	using block_t = sparse_block<BlockSize, Ts...>;
	sparse_table() = default;
	sparse_table(const sparse_table&) = delete;
	sparse_table& operator=(const sparse_table&) = delete;
	sparse_table(sparse_table&& other) noexcept
		: first_{other.first_}
		, last_{other.last_}
		, block_count_{other.block_count_}
		, free_indices_{std::move(other.free_indices_)}
	{
		other.first_ = nullptr;
		other.last_  = nullptr;
		other.block_count_ = 0;
	}
	sparse_table& operator=(sparse_table&& other) noexcept {
		if (this != &other) {
			erase_blocks();
			first_        = other.first_;
			last_         = other.last_;
			block_count_  = other.block_count_;
			free_indices_ = std::move(other.free_indices_);
			other.first_  = nullptr;
			other.last_   = nullptr;
			other.block_count_ = 0;
		}
		return *this;
	}
	~sparse_table() { erase_blocks(); }
	[[nodiscard]]
	auto add() -> size_t {
		const auto lock = std::unique_lock(mutex_);
		if (free_indices_.empty()) {
			free_indices_.resize(BlockSize);
			std::iota(free_indices_.rbegin(), free_indices_.rend(), BlockSize * block_count_);
			add_block();
		}
		return pop_free_index();
	}
	auto erase(size_t elem_index) -> void {
		const auto lock = std::unique_lock(mutex_);
		get_block(elem_index).reset(elem_index);
		free_indices_.push_back(elem_index);
	}
	auto erase_no_reset(size_t elem_index) -> void {
		const auto lock = std::unique_lock(mutex_);
		free_indices_.push_back(elem_index);
	}
	auto clear() -> void {
		const auto lock = std::unique_lock(mutex_);
		with_each_block([](block_t* block) { block->clear(); });
		free_indices_.resize(BlockSize * block_count_);
		std::iota(free_indices_.rbegin(), free_indices_.rend(), 0);
	}
	auto capacity() const -> size_t {
		return (block_count_ * BlockSize);
	}
	auto count() const -> size_t {
		return (block_count_ * BlockSize) - free_indices_.size();
	}
	template <typename T, typename PredFn> [[nodiscard]]
	auto locked_find(PredFn&& pred) -> std::optional<size_t> {
		const auto lock = std::unique_lock(mutex_);
		for (size_t i = 0; i < capacity(); ++i) {
			if (pred(get<T>(i))) {
				return i;
			}
		}
		return std::nullopt;
	}
	template <typename Fn>
	auto locked_visit(Fn&& fn) -> void {
		const auto lock     = std::unique_lock(mutex_);
		const auto capacity = block_count_ * BlockSize;
		for (size_t i = 0; i < capacity; ++i) {
			fn(i);
		}
	}
	template <typename T, typename Fn>
	auto locked_visit(Fn&& fn) -> void {
		const auto lock     = std::unique_lock(mutex_);
		const auto capacity = block_count_ * BlockSize;
		for (size_t i = 0; i < capacity; ++i) {
			fn(i, get<T>(i));
		}
	}
	auto lock() -> std::unique_lock<std::mutex> {
		return std::unique_lock(mutex_);
	}
	template <typename T> auto set(size_t index, T&& value) -> T&                { return get_block(index).set(index, std::forward<T>(value)); }
	template <typename T> [[nodiscard]] auto get(size_t index) -> T&             { return get_block(index).template get<T>(index); }
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T& { return get_block(index).template get<T>(index); }
private:
	auto add_block() -> void {
		const auto new_block = new block_t;
		if (!first_) {
			first_ = new_block;
		}
		if (last_) {
			last_->set_next(new_block);
		}
		last_ = new_block;
		block_count_++;
	}
	auto erase_blocks() -> void {
		with_each_block([](block_t* block) { delete block; });
	}
	template <typename Fn>
	auto with_each_block(Fn&& fn) -> void {
		auto block = first_;
		while (block) {
			const auto next = block->get_next();
			fn(block);
			block = next;
		}
	}
	auto get_block(size_t elem_index) -> sparse_block<BlockSize, Ts...>& {
		const auto block_index = elem_index / BlockSize;
		auto block = first_;
		for (size_t i = 0; i < block_index; ++i) { block = block->get_next(); }
		return *block;
	}
	auto get_block(size_t elem_index) const -> const sparse_block<BlockSize, Ts...>& {
		const auto block_index = elem_index / BlockSize;
		auto block = first_;
		for (size_t i = 0; i < block_index; ++i) { block = block->get_next(); }
		return *block;
	}
	auto pop_free_index() -> size_t {
		const auto index = free_indices_.back();
		free_indices_.pop_back();
		return index;
	}
	block_t* first_ = nullptr;
	block_t* last_  = nullptr;
	std::mutex          mutex_;
	std::atomic<size_t> block_count_ = 0;
	std::vector<size_t> free_indices_;
};

} // ent
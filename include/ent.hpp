#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <format>
#include <list>
#include <mutex>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ent {

struct lock_t{};
static constexpr auto lock = lock_t{};

template <size_t N>
struct string_literal {
	constexpr string_literal(const char (&str)[N]) { std::copy_n(str, N, value); }
	char value[N];
};

template <string_literal Name, size_t BlockSize, typename... Ts>
struct table {
	using const_row_t = std::tuple<const Ts&...>;
	using row_t       = std::tuple<Ts&...>;
private:
	struct block_index { size_t value; };
	struct sub_index   { size_t value; };
	struct lookup_t {
		block_index block;
		sub_index   sub;
	};
	template <typename T> using column_t = std::array<T, BlockSize>;
	struct block_t {
		using data_t = std::tuple<column_t<Ts>...>;
		data_t data;
		block_t* next = nullptr;
	};
	[[nodiscard]] static
	auto get(block_t* block, sub_index idx) -> row_t {
		return std::apply([idx](auto&... args) {
			return std::make_tuple(args[idx]...);
		}, block->data);
	}
	[[nodiscard]] static
	auto get(const block_t& block, sub_index idx) -> const_row_t {
		return std::apply([idx](auto&... args) {
			return std::make_tuple(args[idx]...);
		}, block.data);
	}
	static auto reset(block_t* block, sub_index idx) -> void                                             { (reset<Ts>(block, idx), ...); }
	static auto clear_block(block_t* block) -> void                                                      { for (size_t i = 0; i < BlockSize; ++i) { reset(block, {i}); } }
	template <typename T> [[nodiscard]] static auto get(block_t* block, sub_index idx) -> T&             { return std::get<column_t<T>>(block->data)[idx.value]; }
	template <typename T> [[nodiscard]] static auto get(const block_t& block, sub_index idx) -> const T& { return std::get<column_t<T>>(block.data)[idx.value]; }
	template <typename T> auto set(block_t* block, sub_index idx, T&& value) -> T&                       { return get<std::decay_t<T>>(block, idx) = std::forward<T>(value); }
	template <typename T> static auto reset(block_t* block, sub_index idx) -> void                       { get<T>(block, idx) = T{}; }
public:
	table() = default;
	table(const table&) = delete;
	table& operator=(const table&) = delete;
	table(table&& other) noexcept
		: first_{other.first_}
		, last_{other.last_}
		, block_count_{other.block_count_.load()}
		, free_indices_{std::move(other.free_indices_)}
	{
		other.first_ = nullptr;
		other.last_  = nullptr;
		other.block_count_ = 0;
	}
	table& operator=(table&& other) noexcept {
		if (this != &other) {
			erase_blocks();
			first_        = other.first_;
			last_         = other.last_;
			free_indices_ = std::move(other.free_indices_);
			block_count_.store(other.block_count_.load());
			other.first_  = nullptr;
			other.last_   = nullptr;
			other.block_count_ = 0;
		}
		return *this;
	}
	~table() { erase_blocks(); }
	[[nodiscard]]
	auto acquire(ent::lock_t) -> size_t {
		const auto lock = std::lock_guard{mutex_};
		if (free_indices_.empty()) {
			free_indices_.resize(BlockSize);
			std::iota(free_indices_.rbegin(), free_indices_.rend(), BlockSize * block_count_);
			add_block();
		}
		return pop_free_index();
	}
	auto release(ent::lock_t, size_t elem_index) -> void {
		const auto lock   = std::lock_guard{mutex_};
		const auto lookup = make_lookup(elem_index);
		auto& block       = get_block(lookup.block);
		reset(&block, lookup.sub);
		free_indices_.push_back(elem_index);
	}
	auto release_no_reset(ent::lock_t, size_t elem_index) -> void {
		const auto lock = std::lock_guard{mutex_};
		free_indices_.push_back(elem_index);
	}
	auto clear(ent::lock_t) -> void {
		const auto lock = std::lock_guard{mutex_};
		with_each_block([](block_t* block) { clear_block(block); });
		free_indices_.resize(BlockSize * block_count_);
		std::iota(free_indices_.rbegin(), free_indices_.rend(), 0);
	}
	[[nodiscard]]
	auto get_capacity() const -> size_t {
		return (block_count_ * BlockSize);
	}
	[[nodiscard]]
	auto get_active_row_count(ent::lock_t) const -> size_t {
		const auto lock = std::lock_guard{mutex_};
		return (block_count_ * BlockSize) - free_indices_.size();
	}
	template <typename T, typename PredFn> [[nodiscard]]
	auto find(ent::lock_t, PredFn&& pred) -> std::optional<size_t> {
		const auto lock = std::lock_guard{mutex_};
		for (size_t i = 0; i < get_capacity(); ++i) {
			if (pred(get<T>(i))) {
				return i;
			}
		}
		return std::nullopt;
	}
	template <typename Fn>
	auto visit(ent::lock_t, Fn&& fn) -> void {
		const auto lock     = std::lock_guard{mutex_};
		const auto capacity = block_count_ * BlockSize;
		for (size_t i = 0; i < capacity; ++i) {
			fn(i);
		}
	}
	template <typename T, typename Fn>
	auto visit(ent::lock_t, Fn&& fn) -> void {
		const auto lock     = std::lock_guard{mutex_};
		const auto capacity = block_count_ * BlockSize;
		for (size_t i = 0; i < capacity; ++i) {
			fn(i, get<T>(i));
		}
	}
	template <typename T, typename Fn>
	auto visit(ent::lock_t, Fn&& fn) const -> void {
		const auto lock     = std::lock_guard{mutex_};
		const auto capacity = block_count_ * BlockSize;
		for (size_t i = 0; i < capacity; ++i) {
			fn(i, get<T>(i));
		}
	}
	[[nodiscard]]
	auto get(size_t idx) -> row_t {
		auto lookup = make_lookup(idx);
		auto& block = get_block(lookup.block);
		return get(&block, lookup.sub);
	}
	[[nodiscard]]
	auto get(size_t idx) const -> const_row_t {
		auto lookup = make_lookup(idx);
		auto& block = get_block(lookup.block);
		return get(block, lookup.sub);
	}
	template <typename T>
	auto set(size_t idx, T&& value) -> T& {
		auto lookup = make_lookup(idx);
		auto& block = get_block(lookup.block);
		return set(&block, lookup.sub, std::forward<T>(value));
	}
	template <typename T> [[nodiscard]]
	auto get(size_t idx) -> T& {
		auto lookup = make_lookup(idx);
		auto& block = get_block(lookup.block);
		return get<T>(&block, lookup.sub);
	}
	template <typename T> [[nodiscard]]
	auto get(size_t idx) const -> const T& {
		auto lookup = make_lookup(idx);
		auto& block = get_block(lookup.block);
		return get<T>(block, lookup.sub);
	}
	[[nodiscard]] static constexpr
	auto get_name() -> std::string_view {
		return Name.value;
	}
private:
	auto add_block() -> void {
		const auto new_block = new block_t;
		if (!first_) { first_ = new_block; }
		if (last_) { last_->next = new_block; }
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
			const auto next = block->next;
			fn(block);
			block = next;
		}
	}
	auto get_block(block_index idx) -> block_t& {
		// NOTE: This looks like a data race. But it is not!
		// As long as the index being passed in was valid when this function was called,
		// the loop will exit before reading the 'next' field of any contended blocks.
		// If you are having a hard time grasping that then consider what happens if
		// idx == 0 (the body of the loop is not run.)
		auto block = first_;
		for (size_t i = 0; i < idx.value; ++i) { block = block->next; }
		return *block;
	}
	auto get_block(block_index idx) const -> const block_t& {
		auto block = first_;
		for (size_t i = 0; i < idx.value; ++i) { block = block->next; }
		return *block;
	}
	auto pop_free_index() -> size_t {
		const auto index = free_indices_.back();
		free_indices_.pop_back();
		return index;
	}
	auto make_lookup(size_t elem_index) const -> lookup_t {
		if (elem_index >= get_capacity()) {
			throw std::out_of_range(
				std::format(
					"Tried to look up an element from table '{0}' at index {1}, but {1} is not a valid index! The table capacity is {2}.",
					get_name(), elem_index, get_capacity()));
		}
		return {{elem_index / BlockSize}, {elem_index % BlockSize}};
	}
	block_t*            first_       = nullptr;
	block_t*            last_        = nullptr;
	std::atomic<size_t> block_count_ = 0;
	std::vector<size_t> free_indices_;
	std::mutex          mutex_;
	std::string         name_;
};

// A single-threaded table where rows can only be acquired and never released.
template <string_literal Name, typename... Ts>
struct simple_table {
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
	template <typename T> [[nodiscard]] auto get(size_t index) -> T& {
		check_index(index);
		return std::get<std::vector<T>>(data_)[index];
	}
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T& {
		check_index(index);
		return std::get<std::vector<T>>(data_)[index];
	}
	[[nodiscard]] static constexpr
	auto get_name() -> std::string_view {
		return Name.value;
	}
private:
	auto check_index(size_t index) const -> void {
		if (index >= size()) {
			throw std::out_of_range(
				std::format(
					"Tried to look up an element from table '{0}' at index {1}, but {1} is not a valid index! The table size is {2}.",
					get_name(), index, size()));
		}
	}
	using Tuple = std::tuple<std::vector<Ts>...>;
	Tuple data_;
};

} // ent
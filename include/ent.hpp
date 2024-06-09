#pragma once

#include <array>
#include <list>
#include <numeric>
#include <optional>
#include <tuple>
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
	template <typename T> auto set(size_t index, T value) -> void { get<T>(index) = std::move(value); }
	template <typename T> [[nodiscard]] auto get() -> std::vector<T>& { return std::get<std::vector<T>>(data_); }
	template <typename T> [[nodiscard]] auto get() const -> const std::vector<T>& { return std::get<std::vector<T>>(data_); }
	template <typename T> [[nodiscard]] auto get(size_t index) -> T& { return std::get<std::vector<T>>(data_)[index]; }
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T& { return std::get<std::vector<T>>(data_)[index]; }
private:
	using Tuple = std::tuple<std::vector<Ts>...>;
	Tuple data_;
};

template <typename T>
struct flex_vec {
	auto swap(size_t a, size_t b) -> void {
		std::swap(data_[a], data_[b]);
	}
	auto push_back() -> size_t {
		const auto index = size();
		data_.emplace_back();
		return index;
	}
	auto reset(size_t index) -> void {
		data_[index] = T{};
	}
	auto get() -> std::vector<T>& { return data_; }
	auto get() const -> const std::vector<T>& { return data_; }
	auto size() const -> size_t { return data_.size(); }
private:
	std::vector<T> data_;
};

template <typename... Ts>
struct flex_table {
	auto add() -> size_t {
		if (free_indices_.empty()) {
			const auto index = size();
			(std::get<flex_vec<Ts>>(data_).push_back(), ...);
			index_map_.push_back(index);
			size_++;
			return index;
		}
		const auto index = free_indices_.back();
		(std::get<flex_vec<Ts>>(data_).reset(index_map_[index]), ...);
		free_indices_.pop_back();
		size_++;
		return index;
	}
	auto clear() -> void {
		free_indices_.resize(size_);
		std::iota(free_indices_.begin(), free_indices_.end(), 0);
		std::iota(index_map_.begin(), index_map_.end(), 0);
		size_ = 0;
	}
	auto erase(size_t index) -> void {
		(std::get<flex_vec<Ts>>(data_).swap(index_map_[index], index_map_.back()), ...);
		std::swap(index_map_[index], index_map_.back());
		free_indices_.push_back(index);
		size_--;
	}
	auto is_valid(size_t index) const -> bool {
		if (index >= index_map_.size()) {
			return false;
		}
		if (std::find(free_indices_.begin(), free_indices_.end(), index) != free_indices_.end()) {
			return false;
		}
		return true;
	}
	auto size() const -> size_t { return size_; }
	template <typename T> [[nodiscard]]
	auto find(const T& value) const -> std::optional<size_t> {
		size_t index = 0;
		for (auto value_ : get<T>()) {
			if (value_ == value) {
				const auto pos = size_t(std::distance(index_map_.begin(), std::find(index_map_.begin(), index_map_.end(), index)));
				if (is_valid(pos)) {
					return pos;
				}
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
				const auto pos = size_t(std::distance(index_map_.begin(), std::find(index_map_.begin(), index_map_.end(), index)));
				if (is_valid(pos)) {
					return pos;
				}
			}
			index++;
		}
		return std::nullopt;
	}
	template <typename T> auto set(size_t index, T value) -> void { get<T>(index) = std::move<T>(value); }
	template <typename T> [[nodiscard]] auto get() -> std::vector<T>&             { return std::get<flex_vec<T>>(data_).get(); }
	template <typename T> [[nodiscard]] auto get() const -> const std::vector<T>& { return std::get<flex_vec<T>>(data_).get(); }
	template <typename T> [[nodiscard]] auto get(size_t index) -> T&              { return get<T>()[index_map_[index]]; }
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T&  { return get<T>()[index_map_[index]]; }
private:
	using Tuple = std::tuple<flex_vec<Ts>...>;
	Tuple data_;
	size_t size_ = 0;
	std::vector<size_t> index_map_;
	std::vector<size_t> free_indices_;
};

template <size_t BlockSize, typename... Ts>
struct sparse_block {
	template <typename T> auto reset(size_t elem_index) -> void                       { get<T>(elem_index) = T{}; }
	template <typename T> auto set(size_t elem_index, T value) -> void                { get<T>(index) = std::move<T>(value); }
	template <typename T> [[nodiscard]] auto get(size_t elem_index) -> T&             { return std::get<std::array<T, BlockSize>>(data_)[elem_index % BlockSize]; }
	template <typename T> [[nodiscard]] auto get(size_t elem_index) const -> const T& { return std::get<std::array<T, BlockSize>>(data_)[elem_index % BlockSize]; }
private:
	using Tuple = std::tuple<std::array<Ts, BlockSize>...>;
	Tuple data_;
};

template <size_t BlockSize, typename... Ts>
struct sparse_table {
	auto add() -> size_t {
		if (free_indices_.empty()) {
			free_indices_.resize(BlockSize);
			std::iota(free_indices_.rbegin(), free_indices_.rend(), BlockSize * blocks_.size());
			blocks_.emplace_back();
		}
		const auto idx = free_indices_.back();
		free_indices_.pop_back();
		(get_block(idx).reset<Ts>(idx), ...);
		return idx;
	}
	auto erase(size_t index) -> void {
		free_indices_.push_back(index);
	}
	auto clear() -> void {
		free_indices_.resize(BlockSize * blocks_.size());
		std::iota(free_indices_.rbegin(), free_indices_.rend(), 0);
	}
	auto size() const -> size_t {
		return (blocks_.size() * BlockSize) - free_indices_.size();
	}
	template <typename T> auto set(size_t index, T value) -> void                { get_block(index).set(index, std::move<T>(value)); }
	template <typename T> [[nodiscard]] auto get(size_t index) -> T&             { return get_block(index).get<T>(index); }
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T& { return get_block(index).get<T>(index); }
private:
	auto get_block(size_t index) -> sparse_block<BlockSize, Ts...>&             {
		auto pos = blocks_.begin();
		std::advance(pos, index / BlockSize);
		return *pos;
	}
	auto get_block(size_t index) const -> const sparse_block<BlockSize, Ts...>& {
		auto pos = blocks_.begin();
		std::advance(pos, index / BlockSize);
		return *pos;
	}
	std::list<sparse_block<BlockSize, Ts...>> blocks_;
	std::vector<size_t> free_indices_;
};

template <size_t BlockSize, typename T>
struct block_array {
	auto swap(size_t a, size_t b) -> void {
		std::swap(data_[a], data_[b]);
	}
	template <typename VisitFn>
	auto visit(const VisitFn& visit_fn, size_t n) -> void {
		for (size_t i = 0; i < n; ++i) {
			visit_fn(data_[i]);
		}
	}
private:
	std::array<T, BlockSize> data_;
};

template <size_t BlockSize, typename... Ts>
struct block {
	auto swap(size_t a, size_t b) -> void {
		(std::get<block_array<BlockSize, Ts>>(data_).swap(a, b), ...);
	}
	template <typename T, typename VisitFn> auto visit(const VisitFn& visit_fn, size_t n) -> void {
		std::get<block_array<BlockSize, T>>(data_).visit(visit_fn, n);
	}
	template <typename T> auto reset(size_t index) -> void                       { get<T>(index) = T{}; }
	template <typename T> auto set(size_t index, T value) -> void                { get<T>(index) = std::move<T>(value); }
	template <typename T> [[nodiscard]] auto get(size_t index) -> T&             { return std::get<block_array<T, BlockSize>>(data_)[index]; }
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T& { return std::get<block_array<T, BlockSize>>(data_)[index]; }
private:
	using Tuple = std::tuple<block_array<BlockSize, Ts>...>;
	Tuple data_;
};

template <size_t BlockSize, typename... Ts>
struct block_table {
	auto add() -> size_t {
		for (size_t i = 0; i < blocks_.size(); i++) {
			if (block_sizes_[i] < BlockSize) {
				const auto idx = (i * BlockSize) + block_sizes_[i]++;
				(get_block(idx).reset<Ts>(idx), ...);
				return idx;
			}
		}
		const auto idx = blocks_.size() * BlockSize;
		blocks_.emplace_back();
		block_sizes_.push_back(1);
		(get_block(idx).reset<Ts>(idx), ...);
		return idx;
	}
	auto erase(size_t elem_index) -> void {
		const auto block_index = elem_index / BlockSize;
		const auto block_size  = get_block_size(elem_index);
		const auto array_index = elem_index % BlockSize;
		(get_block(elem_index).swap(array_index, block_size - 1), ...);
		block_sizes_[block_index]--;
	}
	auto clear() -> void {
		std::fill(block_sizes_.begin(), block_sizes_.end(), 0);
	}
	auto size() const -> size_t {
		return std::accumulate(block_sizes_.begin(), block_sizes_.end(), 0);
	}
	template <typename T, typename VisitFn>
	auto visit(const VisitFn& visit_fn) -> void {
		size_t block_index = 0;
		for (auto& block : blocks_) {
			block.visit<T>(visit_fn, block_sizes_[block_index++]);
		}
	}
	template <typename T> auto set(size_t elem_index, T value) -> void                { get_block(elem_index).set(elem_index, std::move<T>(value)); }
	template <typename T> [[nodiscard]] auto get(size_t elem_index) -> T&             { return get_block(elem_index).get<T>(elem_index % BlockSize); }
	template <typename T> [[nodiscard]] auto get(size_t elem_index) const -> const T& { return get_block(elem_index).get<T>(elem_index % BlockSize); }
private:
	auto get_block(size_t elem_index) -> sparse_block<BlockSize, Ts...>&             {
		auto pos = blocks_.begin();
		std::advance(pos, elem_index / BlockSize);
		return *pos;
	}
	auto get_block(size_t elem_index) const -> const block<BlockSize, Ts...>& {
		auto pos = blocks_.begin();
		std::advance(pos, elem_index / BlockSize);
		return *pos;
	}
	auto get_block_size(size_t elem_index) const -> size_t {
		return block_sizes_[elem_index / BlockSize];
	}
	std::list<block<BlockSize, Ts...>> blocks_;
	std::vector<size_t> block_sizes_;
};

} // ent
#pragma once

#include <algorithm>
#include <array>
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
	template <typename T>
	auto set(size_t index, T&& value) -> void {
		get<std::decay_t<T>>(index) = std::forward<T>(value);
	}
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
	[[nodiscard]]
	auto get_next() const {
		return next_;
	}
	[[nodiscard]]
	auto is_alive(size_t elem_index) const -> bool {
		return alive_flags_[elem_index % BlockSize];
	}
	auto get_living_elements(std::vector<size_t>* out) const -> void {
		for (size_t i = 0; i < BlockSize; ++i) {
			if (alive_flags_[i]) {
				out->push_back(i);
			}
		}
	}
	auto set_next(sparse_block<BlockSize, Ts...>* next) -> void {
		next_ = next;
	}
	auto kill(size_t elem_index) -> void {
		alive_flags_.reset(elem_index % BlockSize);
	}
	auto kill_all() -> void {
		alive_flags_.reset();
	}
	auto revive(size_t elem_index) -> void {
		(reset<Ts>(elem_index), ...);
		alive_flags_.set(elem_index % BlockSize);
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
		if constexpr (std::is_copy_assignable_v<T>) {
			get<T>(elem_index % BlockSize) = T{};
		}
	}
	using Tuple = std::tuple<std::array<Ts, BlockSize>...>;
	Tuple data_;
	sparse_block<BlockSize, Ts...>* next_ = nullptr;
	std::bitset<BlockSize> alive_flags_;
};

template <size_t BlockSize, typename... Ts>
struct sparse_table {
	using block_t = sparse_block<BlockSize, Ts...>;
	sparse_table()
		: first_{new block_t}, last_{first_}
	{
		free_indices_.resize(BlockSize);
		std::iota(free_indices_.rbegin(), free_indices_.rend(), 0);
	}
	~sparse_table() { erase_blocks(); }
	[[nodiscard]]
	auto add() -> size_t {
		if (free_indices_.empty()) {
			free_indices_.resize(BlockSize);
			std::iota(free_indices_.rbegin(), free_indices_.rend(), BlockSize * block_count_);
			add_block();
		}
		const auto idx = free_indices_.back();
		free_indices_.pop_back();
		get_block(idx).revive(idx);
		return idx;
	}
	[[nodiscard]]
	auto is_alive(size_t elem_index) const -> bool {
		return get_block(elem_index).is_alive(elem_index);
	}
	auto get_living_elements(std::vector<size_t>* out) const -> void {
		out->clear();
		auto block = first_;
		while (block) {
			block->get_living_elements(out);
			block = block->get_next();
		}
	}
	auto erase(size_t elem_index) -> void {
		get_block(elem_index).kill(elem_index);
		free_indices_.push_back(elem_index);
	}
	auto clear() -> void {
		auto block = first_;
		while (block) {
			block->kill_all();
			block = block->get_next();
		}
		free_indices_.resize(BlockSize * block_count_);
		std::iota(free_indices_.rbegin(), free_indices_.rend(), 0);
	}
	auto size() const -> size_t {
		return (block_count_ * BlockSize) - free_indices_.size();
	}
	template <typename T> auto set(size_t index, T&& value) -> T&                { return get_block(index).set(index, std::forward<T>(value)); }
	template <typename T> [[nodiscard]] auto get(size_t index) -> T&             { return get_block(index).template get<T>(index); }
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T& { return get_block(index).template get<T>(index); }
private:
	auto add_block() -> void {
		const auto new_block = new block_t;
		last_->set_next(new_block);
		last_ = new_block;
		block_count_++;
	}
	auto erase_blocks() -> void {
		auto block = first_;
		while (block) {
			const auto next = block->get_next();
			delete block;
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
	block_t* first_;
	block_t* last_;
	size_t block_count_ = 1;
	std::vector<size_t> free_indices_;
};

template <typename KeyType>
struct sparse_map_handle {
	KeyType key;
	size_t index;
};

template <size_t BlockSize, template <typename Key, typename Value> typename MapImpl, typename KeyType, typename... Ts>
struct sparse_map {
	using key_type    = KeyType;
	using handle_type = sparse_map_handle<key_type>;
	[[nodiscard]]
	auto add(key_type key) -> handle_type {
		const auto index    = table.add();
		key_to_index[key]   = index;
		index_to_key[index] = key;
		return {key, index};
	}
	[[nodiscard]]
	auto exists(key_type key) const -> bool {
		return key_to_index.find(key) != key_to_index.end();
	}
	[[nodiscard]]
	auto is_alive(size_t index) const -> bool {
		return index_to_key.find(index) != index_to_key.end();
	}
	auto get_living_elements(std::vector<handle_type>* out) const -> void {
		out->clear();
		for (const auto [key, index] : key_to_index) {
			out->push_back({key, index});
		}
	}
	auto get_living_elements(std::vector<size_t>* out) const -> void {
		table.get_living_elements(out);
	}
	auto erase(handle_type handle) -> void {
		table.erase(handle.index);
		key_to_index.erase(handle.key);
		index_to_key.erase(handle.index);
	}
	auto erase(key_type key) -> void {
		erase({key, key_to_index[key]});
	}
	auto erase(size_t index) -> void {
		erase({index_to_key[index], index});
	}
	auto clear() -> void {
		table.clear();
		key_to_index.clear();
		index_to_key.clear();
	}
	auto size() const -> size_t {
		return key_to_index.size();
	}
	template <typename T> auto set(size_t index, T&& value) -> T&                      { return table.set(index, std::forward<T>(value)); }
	template <typename T> auto set(handle_type handle, T&& value) -> T&                { return table.set(handle.index, std::forward<T>(value)); }
	template <typename T> [[nodiscard]] auto get(size_t index) -> T&                   { return table.get(index); }
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T&       { return table.get(index); }
	template <typename T> [[nodiscard]] auto get(handle_type handle) -> T&             { return table.get(handle.index); }
	template <typename T> [[nodiscard]] auto get(handle_type handle) const -> const T& { return table.get(handle.index); }
private:
	MapImpl<key_type, size_t> key_to_index;
	MapImpl<size_t, key_type> index_to_key;
	sparse_table<BlockSize, Ts...> table;
};

namespace experimental {

template <size_t BlockSize, typename... Ts>
struct zi_sparse_block {
	[[nodiscard]]
	auto get_next() const {
		return next_;
	}
	auto set_next(zi_sparse_block<BlockSize, Ts...>* next) -> void {
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
	zi_sparse_block<BlockSize, Ts...>* next_ = nullptr;
};

template <size_t BlockSize, typename... Ts>
struct zi_sparse_table {
	using block_t = zi_sparse_block<BlockSize, Ts...>;
	zi_sparse_table() = default;
	zi_sparse_table(const zi_sparse_table&) = delete;
	zi_sparse_table& operator=(const zi_sparse_table&) = delete;
	zi_sparse_table(zi_sparse_table&& other) noexcept
		: first_{other.first_}
		, last_{other.last_}
		, block_count_{other.block_count_}
		, free_indices_{std::move(other.free_indices_)}
	{
		other.first_ = nullptr;
		other.last_  = nullptr;
		other.block_count_ = 0;
	}
	zi_sparse_table& operator=(zi_sparse_table&& other) noexcept {
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
	~zi_sparse_table() { erase_blocks(); }
	[[nodiscard]]
	auto add() -> size_t {
		if (free_indices_.empty()) {
			free_indices_.resize(BlockSize);
			std::iota(free_indices_.rbegin(), free_indices_.rend(), BlockSize * block_count_);
			add_block();
		}
		return pop_free_index();
	}
	auto erase(size_t elem_index) -> void {
		get_block(elem_index).reset(elem_index);
		free_indices_.push_back(elem_index);
	}
	auto clear() -> void {
		auto block = first_;
		while (block) {
			block->clear();
			block = block->get_next();
		}
		free_indices_.resize(BlockSize * block_count_);
		std::iota(free_indices_.rbegin(), free_indices_.rend(), 0);
	}
	auto capacity() const -> size_t {
		return (block_count_ * BlockSize);
	}
	auto count() const -> size_t {
		return (block_count_ * BlockSize) - free_indices_.size();
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
		auto block = first_;
		while (block) {
			const auto next = block->get_next();
			delete block;
			block = next;
		}
	}
	auto get_block(size_t elem_index) -> zi_sparse_block<BlockSize, Ts...>& {
		const auto block_index = elem_index / BlockSize;
		auto block = first_;
		for (size_t i = 0; i < block_index; ++i) { block = block->get_next(); }
		return *block;
	}
	auto get_block(size_t elem_index) const -> const zi_sparse_block<BlockSize, Ts...>& {
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
	size_t block_count_ = 0;
	std::vector<size_t> free_indices_;
};
} // experimental

} // ent
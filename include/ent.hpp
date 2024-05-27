#pragma once

#include <numeric>
#include <optional>
#include <tuple>
#include <vector>

namespace ent {

template <typename... Ts>
struct static_store {
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
	template <typename T> [[nodiscard]] auto get() -> std::vector<T>& { return std::get<std::vector<T>>(data_); }
	template <typename T> [[nodiscard]] auto get() const -> const std::vector<T>& { return std::get<std::vector<T>>(data_); }
	template <typename T> [[nodiscard]] auto get(size_t index) -> T& { return std::get<std::vector<T>>(data_)[index]; }
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T& { return std::get<std::vector<T>>(data_)[index]; }
private:
	using Tuple = std::tuple<std::vector<Ts>...>;
	Tuple data_;
};

template <typename T>
struct dynamic_vec {
	auto erase(size_t index) -> void {
		std::swap(data_[index], data_.back());
	}
	auto push_back() -> size_t {
		const auto index = size();
		data_.emplace_back();
		return index;
	}
	auto get() -> std::vector<T>& { return data_; }
	auto get() const -> const std::vector<T>& { return data_; }
	auto size() const -> size_t { return data_.size(); }
private:
	std::vector<T> data_;
};

template <typename... Ts>
struct dynamic_store {
	auto add() -> size_t {
		if (free_indices_.empty()) {
			const auto index = size();
			(std::get<dynamic_vec<Ts>>(data_).push_back(), ...);
			index_map_.push_back(index);
			size_++;
			return index;
		}
		const auto index = free_indices_.back();
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
		(std::get<dynamic_vec<Ts>>(data_).erase(index), ...);
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
	template <typename T> [[nodiscard]] auto get() -> std::vector<T>& { return std::get<dynamic_vec<T>>(data_).get(); }
	template <typename T> [[nodiscard]] auto get() const -> const std::vector<T>& { return std::get<dynamic_vec<T>>(data_).get(); }
	template <typename T> [[nodiscard]] auto get(size_t index) -> T& { return get<T>()[index_map_[index]]; }
	template <typename T> [[nodiscard]] auto get(size_t index) const -> const T& { return get<T>()[index_map_[index]]; }
private:
	using Tuple = std::tuple<dynamic_vec<Ts>...>;
	Tuple data_;
	size_t size_ = 0;
	std::vector<size_t> index_map_;
	std::vector<size_t> free_indices_;
};

} // ent
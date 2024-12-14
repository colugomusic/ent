#pragma once

#include <array>
#include <mutex>
#include <vector>

namespace ent {

template <typename T, size_t BlockSize>
class stable_growing_pool {
	struct block_index { size_t value; };
	struct sub_index   { size_t value; };
	struct lookup_t    { block_index block; sub_index sub; };
	struct block_t     { std::array<T, BlockSize> data; block_t* next = nullptr; };
public:
	stable_growing_pool() = default;
	stable_growing_pool(const stable_growing_pool&) = delete;
	stable_growing_pool& operator=(const stable_growing_pool&) = delete;
	stable_growing_pool(stable_growing_pool&& other) noexcept
		: first_{other.first_}
		, last_{other.last_}
		, free_indices_{std::move(other.free_indices_)}
	{
		other.first_ = nullptr;
		other.last_ = nullptr;
	}
	stable_growing_pool& operator=(stable_growing_pool&& other) noexcept {
		if (this != &other) {
			erase_blocks();
			first_ = other.first_;
			last_ = other.last_;
			free_indices_ = std::move(other.free_indices_);
			other.first_ = nullptr;
			other.last_ = nullptr;
		}
		return *this;
	}
	~stable_growing_pool() { erase_blocks(); }
	[[nodiscard]]
	auto acquire() -> size_t {
		const auto lock = std::lock_guard{mutex_};
		if (free_indices_.empty()) {
			free_indices_.resize(BlockSize);
			std::iota(free_indices_.rbegin(), free_indices_.rend(), BlockSize * count_blocks());
			add_block();
		}
		return pop_free_index();
	}
	auto release(size_t elem_index) -> void {
		const auto lock = std::lock_guard{mutex_};
		free_indices_.push_back(elem_index);
	}
	auto set(size_t idx, T&& value) -> T& {
		auto lookup = make_lookup(idx);
		get_block(lookup.block).data[lookup.sub.value] = std::forward<T>(value);
	}
	[[nodiscard]]
	auto get(size_t idx) -> T& {
		auto lookup = make_lookup(idx);
		return get_block(lookup.block).data[lookup.sub.value];
	}
	[[nodiscard]]
	auto get(size_t idx) const -> const T& {
		auto lookup = make_lookup(idx);
		return get_block(lookup.block).data[lookup.sub.value];
	}
private:
	auto add_block() -> void {
		const auto new_block = new block_t;
		if (!first_) { first_ = new_block; }
		if (last_) { last_->next = new_block; }
		last_ = new_block;
	}
	auto erase_blocks() -> void {
		with_each_block([](block_t* block) { delete block; });
	}
	template <typename Fn>
	auto with_each_block(Fn&& fn) const -> void {
		auto block = first_;
		while (block) {
			const auto next = block->next;
			fn(block);
			block = next;
		}
	}
	auto count_blocks() const -> size_t {
		size_t total = 0;
		with_each_block([&total](block_t*) { total++; });
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
	static
	auto make_lookup(size_t elem_index) -> lookup_t {
		return {{elem_index / BlockSize}, {elem_index % BlockSize}};
	}
	block_t* first_     = nullptr;
	block_t* last_      = nullptr;
	std::mutex          mutex_;
	std::vector<size_t> free_indices_;
};

} // ent

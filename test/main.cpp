#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "ent.hpp"

struct S {
	int value = 0;
};

TEST_CASE("table") {
	ent::simple_table<"test", int, float> store;
	REQUIRE(store.size() == 0);
	REQUIRE(!store.is_valid(0));
	const auto idx0 = store.push_back();
	REQUIRE(store.size() == 1);
	REQUIRE(idx0 == 0);
	const auto idx1 = store.push_back();
	REQUIRE(store.size() == 2);
	REQUIRE(idx1 == 1);
	REQUIRE(store.is_valid(idx0));
	REQUIRE(store.is_valid(idx1));
	REQUIRE(!store.is_valid(2));
	store.get<int>(idx0) = 111;
	store.get<float>(idx0) = 111.1f;
	store.get<int>(idx1) = 222;
	store.get<float>(idx1) = 222.2f;
	auto find = store.find(111);
	REQUIRE(find);
	REQUIRE(*find == idx0);
	find = store.find(222.2f);
	REQUIRE(find);
	REQUIRE(*find == idx1);
	REQUIRE(!store.find(333));
	const auto idx2 = store.push_back();
	REQUIRE(store.size() == 3);
	REQUIRE(idx2 == 2);
	REQUIRE(store.is_valid(idx0));
	REQUIRE(store.is_valid(idx1));
	REQUIRE(store.is_valid(idx2));
	REQUIRE(!store.is_valid(3));
	REQUIRE(store.get<int>(idx0) == 111);
	REQUIRE(store.get<float>(idx0) == 111.1f);
	REQUIRE(store.get<int>(idx1) == 222);
	REQUIRE(store.get<float>(idx1) == 222.2f);
}

TEST_CASE("sparse_table") {
	ent::table<"test", 512, int, float, S> store;
	auto idx0 = store.acquire(ent::lock);
	auto idx1 = store.acquire(ent::lock);
	store.get<int>(idx0) = 111;
	store.get<float>(idx0) = 111.1f;
	store.get<int>(idx1) = 222;
	store.get<float>(idx1) = 222.2f;
	auto idx2 = store.acquire(ent::lock);
	REQUIRE(store.get<int>(idx0) == 111);
	REQUIRE(store.get<float>(idx0) == 111.1f);
	REQUIRE(store.get<int>(idx1) == 222);
	REQUIRE(store.get<float>(idx1) == 222.2f);
	store.release(ent::lock, idx0);
	REQUIRE(store.get<int>(idx1) == 222);
	REQUIRE(store.get<float>(idx1) == 222.2f);
	idx0 = store.acquire(ent::lock);
	store.clear(ent::lock);
	idx0 = store.acquire(ent::lock);
	idx1 = store.acquire(ent::lock);
	idx2 = store.acquire(ent::lock);
	store.get<int>(idx2) = 333;
	store.get<float>(idx2) = 333.3f;
	REQUIRE(store.get<int>(idx2) == 333);
	REQUIRE(store.get<float>(idx2) == 333.3f);
	store.get<S>(idx0) = S{111};
	store.get<S>(idx1) = S{222};
	store.get<S>(idx2) = S{333};
	store.clear(ent::lock);
	idx0 = store.acquire(ent::lock);
	idx1 = store.acquire(ent::lock);
	idx2 = store.acquire(ent::lock);
	REQUIRE(store.get<S>(idx0).value == 0);
	REQUIRE(store.get<S>(idx1).value == 0);
	REQUIRE(store.get<S>(idx2).value == 0);
	store.get<S>(idx1).value = 222;
	REQUIRE(store.get<S>(idx1).value == 222);
	store.release(ent::lock, idx1);
	idx1 = store.acquire(ent::lock);
	REQUIRE(store.get<S>(idx1).value == 0);
	store.clear(ent::lock);
	idx0 = store.acquire(ent::lock);
	idx1 = store.acquire(ent::lock);
	idx2 = store.acquire(ent::lock);
	store.get<int>(idx0) = 111;
	store.get<int>(idx1) = 222;
	store.get<int>(idx2) = 333;
	store.release(ent::lock, idx2);
	store.release(ent::lock, idx0);
	REQUIRE(store.get<int>(idx1) == 222);
}

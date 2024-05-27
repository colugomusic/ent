#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "ent.hpp"

TEST_CASE("static_store") {
	ent::static_store<int, float> store;
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
	store.get<int>(idx0)   = 111;
	store.get<float>(idx0) = 111.1f;
	store.get<int>(idx1)   = 222;
	store.get<float>(idx1) = 222.2f;
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

TEST_CASE("dynamic_store") {
	ent::dynamic_store<int, float> store;
	REQUIRE(store.size() == 0);
	REQUIRE(!store.is_valid(0));
	auto idx0 = store.add();
	REQUIRE(store.size() == 1);
	REQUIRE(idx0 == 0);
	auto idx1 = store.add();
	REQUIRE(store.size() == 2);
	REQUIRE(idx1 == 1);
	REQUIRE(store.is_valid(idx0));
	REQUIRE(store.is_valid(idx1));
	REQUIRE(!store.is_valid(2));
	store.get<int>(idx0)   = 111;
	store.get<float>(idx0) = 111.1f;
	store.get<int>(idx1)   = 222;
	store.get<float>(idx1) = 222.2f;
	auto idx2 = store.add();
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
	store.erase(idx0);
	REQUIRE(store.size() == 2);
	REQUIRE(!store.is_valid(idx0));
	REQUIRE(store.is_valid(idx1));
	REQUIRE(store.is_valid(idx2));
	idx0 = store.add();
	REQUIRE(store.size() == 3);
	REQUIRE(idx0 == 0);
	REQUIRE(store.is_valid(idx0));
	REQUIRE(store.is_valid(idx1));
	REQUIRE(store.is_valid(idx2));

}

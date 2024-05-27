Here is an array of structs:

```c++
struct SomeData {...};
struct OtherData {...};
struct MoreData {...};

struct Item {
  SomeData some_data;
  OtherData other_data;
  MoreData more_data;
  ...
};

std::vector<Item> items;
```

This can be converted to a struct of arrays instead:

```c++
struct Items {
  std::vector<SomeData> some_data;
  std::vector<OtherData> other_data;
  std::vector<MoreData> more_data;
  ...
};
```

Keeping these arrays in sync is error-prone. This library provides the following data structures:

# static_store

Can be used for items that are only added, but never deleted.

```c++
using Items = ent::static_store<
  SomeData,
  OtherData,
  MoreData
>;

Items items;

// Add items
const auto item0 = items.push_back();
const auto item1 = items.push_back();

// Access data
items.get<SomeData>(index0) = {...};
auto& data = items.get<MoreData>(index1);

// Iterate though one of the arrays
for (auto& data : items.get<OtherData>()) {
  ...
}
```

# dynamic_store

items can also be erased. Some additional book-keeping is performed to achieve this and calls to `get()` have one extra level of indirection under the hood. Old indices may be reused.

```c++
using Items = ent::dynamic_store<
  SomeData,
  OtherData,
  MoreData
>;

Items items;

// Add items
const auto item0 = items.push_back();
const auto item1 = items.push_back();

// Erase items
items.erase(item0);
items.erase(item1);
```
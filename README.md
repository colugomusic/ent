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

Items items;
```

Keeping these arrays in sync is error-prone. This library provides the following data structures:

# table

Can be used for items that are only added, but never deleted.

```c++
using Items = ent::table<
  SomeData,
  OtherData,
  MoreData
>;

Items items;

// Add items
const auto item0 = items.push_back();
const auto item1 = items.push_back();

// Access data
items.get<SomeData>(item0) = {...};
auto& data = items.get<MoreData>(item1);

// Iterate though one of the arrays
for (auto& data : items.get<OtherData>()) {
  ...
}
```

# flex_table

Items can also be erased. Some additional book-keeping is performed to achieve this, and calls to `get(index)` have one extra level of array indirection under the hood. Old indices may be reused.

```c++
using Items = ent::flex_table<
  SomeData,
  OtherData,
  MoreData
>;

Items items;

// Add items
const auto item0 = items.add();
const auto item1 = items.add();

// Erase items
items.erase(item0);
items.erase(item1);
```
Here is a diagram showing what is happening under the hood, if there are 4 items in the store and the item at index 1 is erased:

![soaerase](https://github.com/colugomusic/ent/assets/68328892/e4b61736-26fe-4dd2-a4e4-22c82e29ef9e)

The item being erased is always swapped with the last element. The affected indices are updated so that they point to the same data. The public index 1 is also added to a list of free indices so that it can be reused by further calls to `add()`. The new public size of the data store after the operation is 3.

# sparse_table

This one is implemented as a linked list of fixed-size arrays. The advantage of this is that read access is thread-safe.

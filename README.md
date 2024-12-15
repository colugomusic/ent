This is a data-oriented storage table with care taken to make it useable in multi-threaded and realtime contexts (e.g. in an audio application.)

This data structure can be useful in a situation where you want to allocate storage for the "reasonable maximum" number of rows that will exist in your application, but you also want things to continue working if that soft limit is exceeded (Perhaps you are working on software intended for "creative" users who are inclined to push things as far as their hardware can handle, and would become annoyed if the developer imposed an upper limit on the number of entities they can create.)

This is achieved by allocating storage in contiguous blocks of a fixed size (similar to a `std::deque`.) However, the blocks are managed as a linked list rather than with an array of pointers. An upshot of this is that references are 100% stable, elements can be accessed from multiple threads, and interestingly, the access is realtime-safe.

In the ideal case, the table will consist of a single block for the lifetime of your program. If the user pushes things farther than you expect, a second block will be allocated. Accessing an element in that second block will require traversing the linked list, which puts it about in line with `std::deque` in terms of hopping around in memory. If a third block is allocated, it will be worse than `std::deque` (however we still have safe multi-threaded access which `std::deque` does not.)

If you detect that your users are consistently exceeding the initial table capacity then you might choose to increase the block size.

Note that when accessing elements of the table, the only thread-safety provided by this library is the actual accessing of the element, i.e. if you pass in a valid element index, you will safely get a reference to that element. If there is contention on the actual value of an element then you will still need to provide your own synchronization.

Calling `acquire` is like saying, "I want to use a row of this table for something." It will give you back an index to the row. When you are finished using the row, call `release` with the index. Calling `acquire` when every row is in use will cause a new block to be allocated. This is the only situation in which new blocks are allocated.

When a new block is allocated, every column of every row of the block is zero-initialized. The rows are all technically there before you call `acquire`, and you can even access them safely. `get_capacity` will always return `(block size) * (number of allocated blocks)`.

I recommend designing your data so that when a row is zero-initialized, it is recognizable as an "unused" row. When iterating over the table with `find` or `visit`, every row will be visited, regardless of whether or not it is in use, so you might want to skip the invalid rows depending on what you are doing. I am not an expert on writing data-oriented software but I think this kind of behavior is typical (because of the way CPUs work, iterating over a contiguous block of tightly-packed data and simply skipping the rows you're not interested in will often be way faster than doing anything more clever.)

The `ent::lock` annotation is used to inidicate the parts of the API which will take a lock on a mutex to do their work. If a function doesn't have `ent::lock_t` as its first argument, then it is realtime-safe.

# Usage

```c++
static constexpr size_t REASONABLE_MAXIMUM = 1000;
struct Column_A { ... };
struct Column_B { ... };
struct Column_C { ... };
ent::table<REASONABLE_MAXIMUM, Column_A, Column_B, Column_C> table;
// Acquire a row to use.
auto idx1 = table.acquire(ent::lock);
// You can get/set a field like this:
table.get<Column_A>(idx1) = Column_A{};
// Or like this:
table.set(idx1, Column_A{});
// You can also get a tuple for the entire row by omitting the template argument:
auto entire_row = table.get(idx1);
do_something(std::get<Column_A>(entire_row));
// Release the row. This will zero-out all its fields.
table.release(ent::lock, idx1);
// If you want to leave the row intact, you could call this instead:
// table.release_no_reset(ent::lock, idx1);
```

```c++
// Iterate over the table
for (size_t i = 0; i < table.get_capacity(); i++) {
  do_something(table.get<Column_A>(i));
}
// Or visit it like this:
table.visit(ent::lock, [](size_t index){
  do_something(table.get<Column_A>(i));
});
// Or visit a single column like this:
table.visit<Column_A>(ent::lock, [](Column_A& a) {
  do_something(a);
});
```

Good luck and have fun.

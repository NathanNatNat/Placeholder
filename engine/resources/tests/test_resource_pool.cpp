#include "resources/resource_handle.h"
#include "resources/resource_pool.h"

#include <gtest/gtest.h>

#include <string>
#include <unordered_set>

using namespace placeholder::resources;

TEST(ResourceHandleTest, DefaultHandleIsInvalid)
{
    ResourceHandle<int> handle;
    EXPECT_FALSE(handle.isValid());
    EXPECT_FALSE(static_cast<bool>(handle));
    EXPECT_EQ(handle.generation, 0u);
}

TEST(ResourceHandleTest, EqualityComparison)
{
    ResourceHandle<int> a{1, 1};
    ResourceHandle<int> b{1, 1};
    ResourceHandle<int> c{2, 1};
    ResourceHandle<int> d{1, 2};

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);
}

TEST(ResourceHandleTest, HashableForContainers)
{
    std::unordered_set<ResourceHandle<int>> handles;
    handles.insert({0, 1});
    handles.insert({1, 1});
    handles.insert({0, 1});
    EXPECT_EQ(handles.size(), 2u);
}

TEST(ResourcePoolTest, AddAndGet)
{
    ResourcePool<int> pool;
    auto h = pool.add(42);

    EXPECT_TRUE(h.isValid());
    EXPECT_TRUE(pool.isValid(h));
    EXPECT_EQ(pool.count(), 1u);

    int* value = pool.get(h);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 42);
}

TEST(ResourcePoolTest, MultipleAdds)
{
    ResourcePool<std::string> pool;
    auto h1 = pool.add("alpha");
    auto h2 = pool.add("beta");
    auto h3 = pool.add("gamma");

    EXPECT_EQ(pool.count(), 3u);
    EXPECT_NE(h1, h2);
    EXPECT_NE(h2, h3);
    EXPECT_EQ(*pool.get(h1), "alpha");
    EXPECT_EQ(*pool.get(h2), "beta");
    EXPECT_EQ(*pool.get(h3), "gamma");
}

TEST(ResourcePoolTest, RemoveInvalidatesHandle)
{
    ResourcePool<int> pool;
    auto h = pool.add(10);
    pool.remove(h);

    EXPECT_FALSE(pool.isValid(h));
    EXPECT_EQ(pool.get(h), nullptr);
    EXPECT_EQ(pool.count(), 0u);
}

TEST(ResourcePoolTest, StaleHandleDetected)
{
    ResourcePool<int> pool;
    auto h1 = pool.add(10);
    pool.remove(h1);
    auto h2 = pool.add(20);

    EXPECT_FALSE(pool.isValid(h1));
    EXPECT_TRUE(pool.isValid(h2));
    EXPECT_EQ(pool.get(h1), nullptr);
    EXPECT_EQ(*pool.get(h2), 20);
}

TEST(ResourcePoolTest, FreeListRecyclesSlots)
{
    ResourcePool<int> pool;
    auto h1 = pool.add(1);
    auto h2 = pool.add(2);
    pool.remove(h1);
    auto h3 = pool.add(3);

    EXPECT_EQ(h3.index, h1.index);
    EXPECT_NE(h3.generation, h1.generation);
    EXPECT_EQ(pool.count(), 2u);
    EXPECT_EQ(pool.capacity(), 2u);
}

TEST(ResourcePoolTest, ReplaceChangesData)
{
    ResourcePool<int> pool;
    auto h = pool.add(100);

    EXPECT_TRUE(pool.replace(h, 200));
    EXPECT_EQ(*pool.get(h), 200);
    EXPECT_TRUE(pool.isValid(h));
}

TEST(ResourcePoolTest, ReplaceWithStaleHandleFails)
{
    ResourcePool<int> pool;
    auto h = pool.add(100);
    pool.remove(h);

    EXPECT_FALSE(pool.replace(h, 200));
}

TEST(ResourcePoolTest, ForEachVisitsAllLive)
{
    ResourcePool<int> pool;
    pool.add(10);
    auto h2 = pool.add(20);
    pool.add(30);
    pool.remove(h2);

    int sum = 0;
    int count = 0;
    pool.forEach([&](ResourceHandle<int>, int& val)
    {
        sum += val;
        ++count;
    });

    EXPECT_EQ(count, 2);
    EXPECT_EQ(sum, 40);
}

TEST(ResourcePoolTest, ConstForEach)
{
    ResourcePool<int> pool;
    pool.add(5);
    pool.add(15);

    const auto& constPool = pool;
    int sum = 0;
    constPool.forEach([&](ResourceHandle<int>, const int& val)
    {
        sum += val;
    });

    EXPECT_EQ(sum, 20);
}

TEST(ResourcePoolTest, InvalidIndexReturnsNull)
{
    ResourcePool<int> pool;
    ResourceHandle<int> bogus{999, 1};

    EXPECT_FALSE(pool.isValid(bogus));
    EXPECT_EQ(pool.get(bogus), nullptr);
}

TEST(ResourcePoolTest, RemoveIdempotent)
{
    ResourcePool<int> pool;
    auto h = pool.add(42);
    pool.remove(h);
    pool.remove(h);
    EXPECT_EQ(pool.count(), 0u);
}

TEST(ResourcePoolTest, MoveOnlyTypes)
{
    ResourcePool<std::unique_ptr<int>> pool;
    auto h = pool.add(std::make_unique<int>(42));

    auto* ptr = pool.get(h);
    ASSERT_NE(ptr, nullptr);
    ASSERT_NE(*ptr, nullptr);
    EXPECT_EQ(**ptr, 42);
}

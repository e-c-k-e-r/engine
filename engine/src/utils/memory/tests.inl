#include <uf/utils/tests/tests.h>

TEST(MemoryPool_LinearStrategy, {
	pod::MemoryPool pool;
	uf::memoryPool::initialize(pool, 1024, pod::MemoryPool::Strategy::LINEAR, 0);

	auto alloc1 = uf::memoryPool::allocate(pool, 100, 0);
	EXPECT_TRUE(alloc1.pointer != 0);
	EXPECT_EQ(alloc1.size, 100);

	auto alloc2 = uf::memoryPool::allocate(pool, 200, 0);
	EXPECT_TRUE(alloc2.pointer != 0);
	EXPECT_EQ(alloc2.size, 200);
	EXPECT_EQ(alloc2.pointer, alloc1.pointer + 100);

	auto oom_alloc = uf::memoryPool::allocate(pool, 800, 0);

	EXPECT_TRUE(oom_alloc.size == 0);
	EXPECT_TRUE(oom_alloc.pointer != 0);

	uf::memoryPool::destroy(pool);
})

TEST(MemoryPool_PoolStrategy, {
	pod::MemoryPool pool;
	uf::memoryPool::initialize(pool, 1024, pod::MemoryPool::Strategy::POOL, 64);

	auto alloc1 = uf::memoryPool::allocate(pool, 64, 0);
	EXPECT_TRUE(alloc1.pointer != 0);
	EXPECT_EQ(alloc1.size, 64);

	auto alloc2 = uf::memoryPool::allocate(pool, 64, 0);
	EXPECT_TRUE(alloc2.pointer != 0);

	bool freed = uf::memoryPool::free(pool, (void*)alloc1.pointer, 64);
	EXPECT_TRUE(freed);

	auto alloc3 = uf::memoryPool::allocate(pool, 64, 0);
	EXPECT_EQ(alloc3.pointer, alloc1.pointer);

	uf::memoryPool::destroy(pool);
})

TEST(MemoryPool_SegregatedStrategy, {
	pod::MemoryPool pool;
	uf::memoryPool::initialize(pool, 1024, pod::MemoryPool::Strategy::SEGREGATED, 16);

	auto alloc1 = uf::memoryPool::allocate(pool, 20, 0);
	EXPECT_EQ(alloc1.size, 32);

	auto alloc2 = uf::memoryPool::allocate(pool, 60, 0);
	EXPECT_EQ(alloc2.size, 64);

	EXPECT_TRUE(uf::memoryPool::free(pool, (void*)alloc1.pointer, 32));

	auto alloc3 = uf::memoryPool::allocate(pool, 30, 0);
	EXPECT_EQ(alloc3.size, 32);
	EXPECT_EQ(alloc3.pointer, alloc1.pointer);

	uf::memoryPool::destroy(pool);
})

TEST(MemoryPool_BuddyStrategy, {
	pod::MemoryPool pool;
	uf::memoryPool::initialize(pool, 1024, pod::MemoryPool::Strategy::BUDDY, 64);

	auto alloc1 = uf::memoryPool::allocate(pool, 100, 0);
	EXPECT_EQ(alloc1.size, 128);

	auto alloc2 = uf::memoryPool::allocate(pool, 128, 0);
	EXPECT_EQ(alloc2.size, 128);

	EXPECT_TRUE(uf::memoryPool::free(pool, (void*)alloc1.pointer, 128));
	EXPECT_TRUE(uf::memoryPool::free(pool, (void*)alloc2.pointer, 128));


	auto alloc3 = uf::memoryPool::allocate(pool, 256, 0);
	EXPECT_EQ(alloc3.size, 256);
	EXPECT_EQ(alloc3.pointer, alloc1.pointer);

	uf::memoryPool::destroy(pool);
})
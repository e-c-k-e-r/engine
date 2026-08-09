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

TEST(ReaderWriter_BasicTypes, {
	uf::stl::vector<uint8_t> buffer;
	uf::stl::writer writer(buffer, 0, false);

	uint32_t magic = 0xDEADBEEF;
	float pi = 3.14159f;
	uint8_t flag = 42;

	writer.write(magic);
	writer.write(pi);
	writer.write(flag);

	EXPECT_EQ(buffer.size(), sizeof(magic) + sizeof(pi) + sizeof(flag));

	uf::stl::reader reader(buffer, 0, buffer.size(), true, false);

	EXPECT_FALSE(reader.eof());
	EXPECT_EQ(*reader.read<uint32_t>(), magic);
	EXPECT_FLOAT_EQ(*reader.read<float>(), pi);
	EXPECT_EQ(*reader.read<uint8_t>(), flag);
	EXPECT_TRUE(reader.eof());
})

TEST(ReaderWriter_Arrays, {
	uf::stl::vector<uint8_t> buffer;
	uf::stl::writer writer(buffer, 0, false);

	uf::stl::vector<int32_t> outData = { 10, 20, 30, 40, 50 };
	writer.write(outData);

	EXPECT_EQ(buffer.size(), outData.size() * sizeof(int32_t));

	uf::stl::reader reader(buffer, 0, buffer.size(), true, false);
	uf::stl::vector<int32_t> inData;
	bool success = reader.read(5, inData);

	EXPECT_TRUE(success);
	EXPECT_EQ(inData.size(), 5);
	EXPECT_EQ(inData[0], 10);
	EXPECT_EQ(inData[4], 50);
	EXPECT_TRUE(reader.eof());
})

TEST(ReaderWriter_SeekSkipReserve, {
	uf::stl::vector<uint8_t> buffer;
	uf::stl::writer writer(buffer, 0, false);

	uint32_t* reserved = writer.reserve<uint32_t>();
	writer.skip(4);
	writer.write<uint16_t>(0xBEEF);

	EXPECT_EQ(buffer.size(), 10);
	EXPECT_EQ(writer.offset(), 10);

	writer.seek(0);
	writer.write<uint32_t>(0xCAFEBABE);

	uf::stl::reader reader(buffer, 0, buffer.size(), true, false);
	EXPECT_EQ(*reader.read<uint32_t>(), 0xCAFEBABE);

	reader.skip(4);
	EXPECT_EQ(*reader.read<uint16_t>(), 0xBEEF);
})

TEST(Reader_PeekAndZeroCopy, {
	uf::stl::vector<uint8_t> buffer;
	uf::stl::writer writer(buffer, 0, false);
	writer.write<uint64_t>(123456789012345);

	uf::stl::reader readerZC(buffer, 0, buffer.size(), true, false);
	const uint64_t* peekPtr = readerZC.peek<uint64_t>();
	EXPECT_TRUE(peekPtr != nullptr);
	EXPECT_EQ(*peekPtr, 123456789012345);
	EXPECT_EQ(readerZC.offset(), 0);

	const uint64_t* readPtrZC = readerZC.read<uint64_t>();
	EXPECT_EQ((uintptr_t)(peekPtr), (uintptr_t)(readPtrZC)); // fmt does NOT like this line

	uf::stl::reader readerNZC(buffer, 0, buffer.size(), false, false);
	const uint64_t* readPtrNZC = readerNZC.read<uint64_t>();
	EXPECT_TRUE(readPtrNZC != nullptr);
	EXPECT_TRUE(readPtrNZC != (const uint64_t*)buffer.data());
	EXPECT_EQ(*readPtrNZC, 123456789012345);
})

TEST(ReaderWriter_Alignment, {
	uf::stl::vector<uint8_t> buffer;

	uf::stl::writer writer(buffer, 0, true);

	uint8_t smallByte = 0xAA;
	uint32_t largeInt = 0xDEADBEEF;

	writer.write(smallByte);
	writer.write(largeInt);

	EXPECT_EQ(buffer.size(), 8);
	EXPECT_EQ(writer.offset(), 8);

	uf::stl::reader reader(buffer, 0, buffer.size(), true, true);

	EXPECT_EQ(*reader.read<uint8_t>(), 0xAA);
	EXPECT_EQ(*reader.read<uint32_t>(), 0xDEADBEEF);
})

TEST(ReaderWriter_BoundsChecking, {
	uf::stl::vector<uint8_t> buffer;
	uf::stl::writer writer(buffer, 0, false);

	writer.write<uint32_t>(100);
	writer.write<uint32_t>(200);

	EXPECT_EQ(buffer.size(), 8);

	uf::stl::reader reader(buffer, 0, buffer.size(), true, false);

	EXPECT_EQ(*reader.read<uint32_t>(), 100);
	EXPECT_EQ(reader.remaining(), 4);

	const uint64_t* oversizedRead = reader.read<uint64_t>();
	EXPECT_TRUE(oversizedRead == nullptr);

	EXPECT_EQ(reader.remaining(), 4);

	EXPECT_EQ(*reader.read<uint32_t>(), 200);
	EXPECT_TRUE(reader.eof());
})

TEST(Reader_ArrayNonZeroCopy, {
	uf::stl::vector<uint8_t> buffer;
	uf::stl::writer writer(buffer, 0, false);

	uf::stl::vector<float> outData = { 1.1f, 2.2f, 3.3f, 4.4f };
	writer.write(outData);

	uf::stl::reader reader(buffer, 0, buffer.size(), false, false);

	uf::stl::vector<float> inData;
	bool success = reader.read(4, inData);

	EXPECT_TRUE(success);
	EXPECT_EQ(inData.size(), 4);
	EXPECT_FLOAT_EQ(inData[0], 1.1f);
	EXPECT_FLOAT_EQ(inData[3], 4.4f);

	EXPECT_TRUE((const uint8_t*)inData.data() != buffer.data());
})
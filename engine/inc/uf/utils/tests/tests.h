#include <uf/config.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/memory/key_map.h>

#if 1 || UF_DEBUG
	#define EXPECT_TRUE(expr) { \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = ((expr));\
		test.message = ::fmt::format("EXPECT_TRUE({})", #expr);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_FALSE(expr) { \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (!(expr));\
		test.message = ::fmt::format("EXPECT_FALSE({})", #expr);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_EQ(a,b) { \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (((a) == (b)));\
		test.message = ::fmt::format("EXPECT_EQ({}, {}), ({} == {})", #a, #b, a, b);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_NEAR(a,b,eps) { \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (std::fabs((a)-(b)) <= (eps));\
		test.message = ::fmt::format("EXPECT_NEAR({}, {}), ({} == {})", #a, #b, a, b);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_FLOAT_EQ(a,b) { \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (std::fabs((a)-(b)) <= (1.0e-4f));\
		test.message = ::fmt::format("EXPECT_FLOAT_EQ({}, {}), ({} == {})", #a, #b, a, b);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define TEST(name, ...) \
		void TOKEN_PASTE(UNIT_TEST_, name)(){\
			__VA_ARGS__;\
		};\
		static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
			uf::unitTests::tests[#name] = { false, "", TOKEN_PASTE(UNIT_TEST_, name) };\
		});\

#else
	// no-op
	#define TEST(...)
#endif

namespace pod {
	struct UnitTest {
		typedef std::function<void()> function_t;

		bool passes;
		uf::stl::string message;
		pod::UnitTest::function_t function;
	};
}

namespace uf {
	namespace unitTests {
		extern UF_API uf::stl::string current;
		extern UF_API uf::stl::KeyMap<pod::UnitTest> tests;

		void UF_API execute();
	}
}
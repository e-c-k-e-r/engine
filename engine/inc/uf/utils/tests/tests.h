#include <uf/config.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/memory/key_map.h>

#if 1 || UF_DEBUG
	#define EXPECT_TRUE(expr) { \
		bool _val = (expr); \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = _val;\
		test.message = FMT_FORMAT("EXPECT_TRUE({})", #expr);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_FALSE(expr) { \
		bool _val = (expr); \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = !_val;\
		test.message = FMT_FORMAT("EXPECT_FALSE({})", #expr);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_EQ(a,b) { \
		auto _a = (a); \
		auto _b = (b); \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (_a == _b);\
		test.message = FMT_FORMAT("EXPECT_EQ({}, {}), ({} == {})", #a, #b, _a, _b);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_GT(a,b) { \
		auto _a = (a); \
		auto _b = (b); \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (_a > _b);\
		test.message = FMT_FORMAT("EXPECT_GT({}, {}), ({} > {})", #a, #b, _a, _b);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_LT(a,b) { \
		auto _a = (a); \
		auto _b = (b); \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (_a < _b);\
		test.message = FMT_FORMAT("EXPECT_LT({}, {}), ({} < {})", #a, #b, _a, _b);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_GE(a,b) { \
		auto _a = (a); \
		auto _b = (b); \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (_a >= _b);\
		test.message = FMT_FORMAT("EXPECT_GE({}, {}), ({} >= {})", #a, #b, _a, _b);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_LE(a,b) { \
		auto _a = (a); \
		auto _b = (b); \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (_a <= _b);\
		test.message = FMT_FORMAT("EXPECT_LE({}, {}), ({} <= {})", #a, #b, _a, _b);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_NEAR(a,b,eps) { \
		auto _a = (a); \
		auto _b = (b); \
		auto _eps = (eps); \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (std::fabs(_a - _b) <= _eps);\
		test.message = FMT_FORMAT("EXPECT_NEAR({}, {}), ({} == {})", #a, #b, _a, _b);\
		UF_MSG_DEBUG("[{}] {}", test.passes ? "PASS" : "FAIL", test.message);\
	}

	#define EXPECT_FLOAT_EQ(a,b) { \
		auto _a = (a); \
		auto _b = (b); \
		auto& test = uf::unitTests::tests[uf::unitTests::current];\
		test.passes = (std::fabs(_a - _b) <= 1.0e-4f);\
		test.message = FMT_FORMAT("EXPECT_FLOAT_EQ({}, {}), ({} == {})", #a, #b, _a, _b);\
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
#include <uf/utils/tests/tests.h>


uf::stl::string uf::unitTests::current;
uf::stl::KeyMap<pod::UnitTest> uf::unitTests::tests;

void uf::unitTests::execute() {
	auto testCount = uf::unitTests::tests.keys.size();

	if ( !testCount ) return;

	UF_MSG_DEBUG("Running {} tests...", testCount);
	for ( auto& name : uf::unitTests::tests.keys ) {
		uf::unitTests::current = name;
		uf::unitTests::tests[name].function();
	}
	UF_MSG_DEBUG("");
	int pass = 0;
	int fail = 0;
	for ( auto& name : uf::unitTests::tests.keys ) {
		auto& test = uf::unitTests::tests[name];
		auto& value = test.passes ? pass : fail;
		++value;
		UF_MSG_DEBUG("[{}]: [{}]: {}", test.passes ? "PASS" : "FAIL", name, test.message);
	}
	UF_MSG_DEBUG("{} / {} tests passed.", pass, testCount );
	UF_MSG_DEBUG("{} / {} tests failed.", fail, testCount );
}
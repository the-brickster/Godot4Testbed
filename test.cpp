#include "test.h"


namespace goxlap {
	void test::add(int p_value) {
		count += p_value;
	}

	void test::reset() {
		count = 0;
	}

	int test::get_total() const {
		return count;
	}

	void test::_bind_methods() {
		ClassDB::bind_method(D_METHOD("add", "value"), &test::add);
		ClassDB::bind_method(D_METHOD("reset"), &test::reset);
		ClassDB::bind_method(D_METHOD("get_total"), &test::get_total);
	}

	test::test() {
		count = 0;
	}
}

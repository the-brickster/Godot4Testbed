#pragma once

#include "core/object/ref_counted.h"

namespace goxlap {
	class test : public RefCounted {
		GDCLASS(test, RefCounted);

		int count;

	protected:
		static void _bind_methods();

	public:
		void add(int p_value);
		void reset();
		int get_total() const;

		test();

	};

}

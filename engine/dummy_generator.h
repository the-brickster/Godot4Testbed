#ifndef DUMMY_GENERATOR_H
#define DUMMY_GENERATOR_H

namespace goxlap {
	extern bool initialized;
	void noise_initialize();
	float noise_get_2D(float x, float y);
}
#endif // !DUMMY_GENERATOR_H


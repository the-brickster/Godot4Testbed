#include "dummy_generator.h"
#include "../lib/fastnoise/FastNoise.h"


namespace goxlap {
	FastNoise noise;
	bool initialized = false;
	void noise_initialize() {
		noise.SetNoiseType(FastNoise::Perlin);
		initialized = true;
	}

	float noise_get_2D(float x, float y) {
		return noise.GetNoise(x, y);
	}

}

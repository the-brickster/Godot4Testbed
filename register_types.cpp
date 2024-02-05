#include "register_types.h"

#include "core/object/class_db.h"
#include "test.h"
#include "goxlap_godot.h"

void initialize_goxlap_module(ModuleInitializationLevel p_level) {
	using namespace goxlap;
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(test);
	GDREGISTER_CLASS(gd_goxlap::goxlap_engine_gd);
	GDREGISTER_CLASS(gd_goxlap::goxlap_debug_box_wrapper);
}

void uninitialize_goxlap_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

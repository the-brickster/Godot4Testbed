#pragma once
#ifndef GOXLAP_GODOT_H
#include <core/object/ref_counted.h>
#include <scene/3d/camera_3d.h>
#include <vector>
#include <unordered_map>

namespace gd_goxlap {
	enum GodotTypes {
		Mesh,
		Shader,
		Material,
		VisualInstance,
		Scenario
	};

	typedef struct GodotObject {
		RID rid;
		GodotTypes type;
	}GodotObject;

	typedef struct GoxlapGodotMeshWrapper {
		GodotObject shaderRID;
		GodotObject meshRID;
		GodotObject instance;
		Transform3D xform;
		AABB aabb;
		Array meshArr;
		PackedInt32Array meshVerts;
	}GoxlapGodotMeshWrapper;

	class goxlap_debug_box_wrapper : public RefCounted{
		GDCLASS(goxlap_debug_box_wrapper, RefCounted);
	public:
		GodotObject instanceRID;
		GodotObject shaderRID;
		GodotObject meshRID;
		Transform3D xform;
		AABB aabb;
		Array meshArr;
	};

	class goxlap_engine_gd :public RefCounted {
		GDCLASS(goxlap_engine_gd,RefCounted);

	private:
		Camera3D* camera;
		GodotObject rsInstance;
		GodotObject shader;
		GodotObject mesh;
		GodotObject material;
		std::vector<GoxlapGodotMeshWrapper> voxelMeshes;

		
	protected:
		static void _bind_methods();
	public:
		//Debug material
		GodotObject debug_wireframe_material;
		Ref<StandardMaterial3D> debug_standard_material;

		GodotObject scenario;

		RenderingServer* rs;
		Vector<goxlap_debug_box_wrapper*> debug_octree_wireframes;

		void set_camera(Camera3D* p_camera);
		Camera3D* get_camera() const;
		void set_material_rid(RID material);
		void set_scenario_rid(RID scenario);
		void print_something(RID material, RID scenario);
		void create_debug_material();
		void create_voxel_object(Vector3 origin,uint32_t vox_object_size);
	};
	static void create_debug_octree(void* debug_object, int argc, ...);
	static Vector3 transform_coordinates_step(Vector3 position, int chunk_world_size, int half_chunk_world_size);
}
VARIANT_ENUM_CAST(gd_goxlap::GodotTypes);
#endif // !GOXLAP_GODOT_H

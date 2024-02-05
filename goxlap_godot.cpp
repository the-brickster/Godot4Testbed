#include "goxlap_godot.h"
#include "engine/goxlap.h"
#include <bitset>

namespace gd_goxlap {

	static void create_debug_octree(void* debug_object, int argc, ...) {
		goxlap_engine_gd* debug_obj_ptr = (goxlap_engine_gd*)debug_object;

		print_line("A CALL BACK FUNCTION! " + itos(debug_obj_ptr->debug_octree_wireframes.size()));
		va_list varargs;
		va_start(varargs, argc);
		double node_size = va_arg(varargs, double);
		glm::vec3 node_offset = va_arg(varargs, glm::vec3);
		va_end(varargs);
		print_line("NODE SIZE " + itos(node_size));
		Vector3 gd_node_offset = Vector3(node_offset.x, node_offset.y, node_offset.z);
		gd_node_offset = gd_node_offset - Vector3(node_size * 0.5f, node_size * 0.5f, node_size * 0.5f);
		goxlap_debug_box_wrapper* debug_wrapper = memnew(goxlap_debug_box_wrapper);
		debug_wrapper->instanceRID = { debug_obj_ptr->rs->instance_create(),GodotTypes::VisualInstance };
		debug_wrapper->meshRID = { debug_obj_ptr->rs->mesh_create(),GodotTypes::Mesh };
		
		Array arr = Array();
		arr.resize(RenderingServer::ARRAY_MAX);
		

		arr[RenderingServer::ARRAY_VERTEX] = PackedVector3Array{ Vector3(0, 0, 0),
															Vector3(1, 0, 0),
															Vector3(1, 0, 1),
															Vector3(0, 0, 1),
															Vector3(0, 1, 0),
															Vector3(1, 1, 0),
															Vector3(1, 1, 1),
															Vector3(0, 1, 1) };
		arr[RenderingServer::ARRAY_COLOR] = PackedColorArray{ Color(0,1,0,0),
													Color(0,1,0,0),
													Color(0,1,0,0),
													Color(0,1,0,0),
													Color(0,1,0,0),
													Color(0,1,0,0),
													Color(0,1,0,0),
													Color(0,1,0,0) };
		arr[RenderingServer::ARRAY_INDEX] = PackedInt32Array{0, 1,
													1, 2,
													2, 3,
													3, 0,

													4, 5,
													5, 6,
													6, 7,
													7, 4,

													0, 4,
													1, 5,
													2, 6,
													3, 7};

		debug_wrapper->meshArr.append_array(arr);

		debug_wrapper->xform = Transform3D(Basis(), Vector3());
		debug_wrapper->xform.scale(Vector3(node_size, node_size, node_size));
		debug_wrapper->xform.origin = gd_node_offset;
		debug_wrapper->aabb = AABB(gd_node_offset, Vector3(node_size, node_size, node_size));

		debug_obj_ptr->rs->instance_set_base(debug_wrapper->instanceRID.rid, debug_wrapper->meshRID.rid);
		debug_obj_ptr->rs->mesh_add_surface_from_arrays(debug_wrapper->meshRID.rid, RenderingServer::PRIMITIVE_LINES, debug_wrapper->meshArr, Array(), Dictionary());
		debug_obj_ptr->rs->mesh_surface_set_material(debug_wrapper->meshRID.rid, 0, debug_obj_ptr->debug_wireframe_material.rid);
		debug_obj_ptr->rs->instance_set_transform(debug_wrapper->instanceRID.rid, debug_wrapper->xform);
		debug_obj_ptr->rs->instance_set_scenario(debug_wrapper->instanceRID.rid, debug_obj_ptr->scenario.rid);
		debug_obj_ptr->rs->sync();

		debug_obj_ptr->debug_octree_wireframes.push_back(debug_wrapper);

		printf("Node Position: %s\n", glm::to_string(node_offset).c_str());
	}

	Vector3 transform_coordinates_step(Vector3 position, int chunk_world_size, int half_chunk_world_size)
	{
		return Vector3();
	}


	void goxlap_engine_gd::set_camera(Camera3D* p_camera) {
		this->camera = p_camera;
		Size2i window_size = DisplayServer::get_singleton()->window_get_size();
		float near = this->camera->get_near();
		float far = this->camera->get_far();
		float fov = this->camera->get_fov();
		float aspect = float(window_size.width) / float(window_size.height);
		goxlap::setup_goxlap_camera_proj_matrix(window_size.width, window_size.height, near, far, fov, aspect);
		Transform3D tform = this->camera->get_camera_transform();
		Vector3 x_col_gd = tform.basis.get_column(0);
		Vector3 y_col_gd = tform.basis.get_column(1);
		Vector3 z_col_gd = tform.basis.get_column(2);

		Vector3 pos = tform.get_origin();

		glm::vec3 xCol = glm::vec3(x_col_gd.x, x_col_gd.y, x_col_gd.z);
		glm::vec3 yCol = glm::vec3(y_col_gd.x, y_col_gd.y, y_col_gd.z);
		glm::vec3 zCol = glm::vec3(z_col_gd.x, z_col_gd.y, z_col_gd.z);
		glm::vec3 translation = glm::vec3(pos.x, pos.y, pos.z);

		goxlap::setup_goxlap_camera_view_matrix(xCol, yCol, zCol, translation);
		Projection cam_proj = this->camera->get_camera_projection();
		Plane near_gd = cam_proj.get_projection_plane(Projection::Planes::PLANE_NEAR);
		goxlap::plane near_plane = { glm::vec3(near_gd.normal.x, near_gd.normal.y, near_gd.normal.z), near_gd.d };

		Plane far_gd = cam_proj.get_projection_plane(Projection::Planes::PLANE_FAR);
		goxlap::plane far_plane = { glm::vec3(far_gd.normal.x, far_gd.normal.y, far_gd.normal.z), far_gd.d };

		Plane left_gd = cam_proj.get_projection_plane(Projection::Planes::PLANE_LEFT);
		goxlap::plane left = { glm::vec3(left_gd.normal.x, left_gd.normal.y, left_gd.normal.z), left_gd.d };

		Plane top_gd = cam_proj.get_projection_plane(Projection::Planes::PLANE_TOP);
		goxlap::plane top = { glm::vec3(top_gd.normal.x, top_gd.normal.y, top_gd.normal.z), top_gd.d };

		Plane right_gd = cam_proj.get_projection_plane(Projection::Planes::PLANE_RIGHT);
		goxlap::plane right = {glm::vec3(right_gd.normal.x, right_gd.normal.y, right_gd.normal.z), right_gd.d};

		Plane bottom_gd = cam_proj.get_projection_plane(Projection::Planes::PLANE_BOTTOM);
		goxlap::plane bottom = { glm::vec3(bottom_gd.normal.x, bottom_gd.normal.y, bottom_gd.normal.z), bottom_gd.d };

		goxlap::setup_goxlap_camera_frustum_planes(left, right, top, bottom, near_plane, far_plane);
		goxlap::setup_fastnoise();
		float val = goxlap::retrieve_noise(5.0f, 3.0f);
		print_line("----------------------------------------- FAST NOISE TEST: " + rtos(val)+" "+itos(sizeof(goxlap::packed_vertex)));
		rs = RenderingServer::get_singleton();
		
	}
	Camera3D* goxlap_engine_gd::get_camera() const {
		return this->camera;
	}
	void goxlap_engine_gd::set_material_rid(RID material)
	{
		this->material = { material,GodotTypes::Material };
	}
	void goxlap_engine_gd::set_scenario_rid(RID scenario)
	{
		this->scenario = { scenario, GodotTypes::Scenario };
		print_line("scenario rid: " + itos(this->scenario.rid.get_id()));
	}
	void goxlap_engine_gd::print_something(RID material, RID scenario) {
		Transform3D tform = this->camera->get_camera_transform();
		print_line("TESTING THE TFORM: " + tform);
		String test_str = String(glm::to_string(goxlap::cam_props.gfx_props.view).c_str());
		print_line("Testing glm print " + test_str);
		Projection p = this->camera->get_camera_projection();
		goxlap::voxel_object voxel_object;
		goxlap::point_voxel_mesh_data mesh_data;
		//int count = goxlap::test_voxel_data(&voxel_object,&mesh_data,&create_debug_octree_proxy,this);
		print_line("VALUE: " + itos((uintptr_t)voxel_object.chunk));
		//print_line("amount of visible voxels: " + itos(count));
		print_line("Amound of voxels: " + itos(mesh_data.vert_data.size()));
		uint8_t val = (uint8_t)voxel_object.chunk->voxel[7];
		print_line("Test Value: " + itos(val));
		val = (uint8_t)voxel_object.chunk->voxel[32];
		print_line("Test Value2: " + itos(val));
		print_line("Test size of mesh struct: " + itos(sizeof(GoxlapGodotMeshWrapper)));

		rsInstance = { rs->instance_create(),GodotTypes::VisualInstance };
		mesh = { rs->mesh_create(),GodotTypes::Mesh };
		shader = { material,GodotTypes::Shader };
		
		PackedInt32Array packed_mesh_vert_data;
		packed_mesh_vert_data.resize(mesh_data.vert_data.size());
		memcpy(packed_mesh_vert_data.ptrw(), mesh_data.vert_data.data(), sizeof(goxlap::packed_vertex) * mesh_data.vert_data.size());

		Array mesh_arr;
		mesh_arr.resize(RenderingServer::ARRAY_MAX);
		mesh_arr[RenderingServer::ARRAY_INDEX] = packed_mesh_vert_data;
		
		AABB aabb = AABB(Vector3(0, 0, 0), Vector3(10, 10, 10));
		rs->instance_set_base(rsInstance.rid, mesh.rid);
		rs->mesh_add_surface_from_arrays(mesh.rid, RenderingServer::PRIMITIVE_POINTS, mesh_arr, Array(), Dictionary(), 1 << (RenderingServer::ARRAY_INDEX + 1 + 15));
		rs->mesh_surface_set_material(mesh.rid, 0, shader.rid);
		rs->mesh_set_custom_aabb(mesh.rid, aabb);
		Transform3D xForm = Transform3D(Basis(), Vector3(0, 0, 0));
		rs->instance_set_transform(rsInstance.rid, xForm);
		rs->instance_set_scenario(rsInstance.rid, this->scenario.rid);
		rs->sync();
		print_line("Test size packed array: " + itos(sizeof(PackedInt32Array)));
		print_line("PACKED SIZE: " + itos(packed_mesh_vert_data.size()));
		GoxlapGodotMeshWrapper meshwrapper;
		meshwrapper.aabb = aabb;
		meshwrapper.meshArr = mesh_arr;
		meshwrapper.meshRID = mesh;
		meshwrapper.shaderRID = shader;
		meshwrapper.xform = xForm;
		meshwrapper.meshVerts = packed_mesh_vert_data;

		voxelMeshes.push_back(meshwrapper);
	}

	void goxlap_engine_gd::create_debug_material()
	{
		
		Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
		debug_standard_material = material;

		StandardMaterial3D* ptr;
		debug_standard_material.reference_ptr(ptr);
		ptr->set_shading_mode(BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);
		ptr->set_flag(BaseMaterial3D::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		debug_wireframe_material = { ptr->get_rid(),GodotTypes::Material };
	}

	void goxlap_engine_gd::create_voxel_object(Vector3 origin, uint32_t vox_object_size)
	{
		GoxlapGodotMeshWrapper meshwrapper;
		meshwrapper.instance = { rs->instance_create(),GodotTypes::VisualInstance }; 
		meshwrapper.meshRID = { rs->mesh_create(),GodotTypes::Mesh };
		meshwrapper.meshArr.resize(RenderingServer::ARRAY_MAX);
		meshwrapper.shaderRID = { this->material.rid,GodotTypes::Shader };
		Vector3i chunk_origin = (Vector3i(origin) / 2) * 2;
		meshwrapper.xform = Transform3D(Basis(), chunk_origin);
		print_line("Size octree node: " + itos(sizeof(goxlap::node)));
		print_line("Size of page node: " + itos(sizeof(goxlap::octree_page)));
		create_debug_material();

		goxlap::voxel_object voxel_object;
		voxel_object.voxel_size = 0.0625f;
		voxel_object.voxel_object_size = vox_object_size;
		voxel_object.world_pos = { 0,0,0 };

		//Mem and pages
		goxlap::voxel_object_init(&voxel_object, 1000 * 1000 * 1000);
		goxlap::setup_voxel_object_pages(&voxel_object);

		goxlap::point_voxel_mesh_data mesh_data;

		int count = goxlap::test_voxel_data(origin.x, origin.y, origin.z ,&voxel_object, &mesh_data, create_debug_octree,this);
		print_line("amount of visible voxels: " + itos(count));

		PackedInt32Array packed_mesh_vertData;
		packed_mesh_vertData.resize(mesh_data.vert_data.size());
		memcpy(packed_mesh_vertData.ptrw(), mesh_data.vert_data.data(), sizeof(goxlap::packed_vertex) * mesh_data.vert_data.size());
		meshwrapper.meshArr[RenderingServer::ARRAY_INDEX] = packed_mesh_vertData;
		print_line("size of packed arr :" + itos(packed_mesh_vertData.size()));

		AABB aabb = AABB(Vector3(0, 0, 0), Vector3(10, 10, 10));
		rs->instance_set_base(meshwrapper.instance.rid, meshwrapper.meshRID.rid);
		rs->mesh_add_surface_from_arrays(meshwrapper.meshRID.rid, RenderingServer::PRIMITIVE_POINTS, meshwrapper.meshArr, Array(), Dictionary(), 1 << (RenderingServer::ARRAY_INDEX + 1 + 15));
		rs->mesh_surface_set_material(meshwrapper.meshRID.rid, 0, meshwrapper.shaderRID.rid);
		rs->mesh_set_custom_aabb(meshwrapper.meshRID.rid, aabb);
		rs->instance_set_transform(meshwrapper.instance.rid, meshwrapper.xform);
		rs->instance_set_scenario(meshwrapper.instance.rid, this->scenario.rid);
		rs->sync();

		voxelMeshes.push_back(meshwrapper);
	}



	void goxlap_engine_gd::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_camera", "camera"), &goxlap_engine_gd::set_camera);
		ClassDB::bind_method(D_METHOD("get_camera"), &goxlap_engine_gd::get_camera);
		ClassDB::bind_method(D_METHOD("print_something"), &goxlap_engine_gd::print_something);
		ClassDB::bind_method(D_METHOD("set_material_rid"), &goxlap_engine_gd::set_material_rid);
		ClassDB::bind_method(D_METHOD("set_scenario_rid"), &goxlap_engine_gd::set_scenario_rid);
		ClassDB::bind_method(D_METHOD("create_voxel_object"), &goxlap_engine_gd::create_voxel_object);
	}

}

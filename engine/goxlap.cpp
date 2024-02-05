#include "goxlap.h"
#include <stdlib.h>

namespace goxlap {
	camera_properties cam_props;
	fl_mem_alloc mem_alloc;
	memory_pools mem_pools;
	voxel_object* vox_object;

	void setup_goxlap_camera_proj_matrix(int screen_x, int screen_y, float near, float far, float fov, float aspect) {
		glm::mat4 projection_mat = glm::perspective(fov, aspect, near, far);
		cam_props.near = near;
		cam_props.far = far;
		cam_props.gfx_props.screen_x = screen_x;
		cam_props.gfx_props.screen_y = screen_y;
		cam_props.gfx_props.projection = projection_mat;
	}
	void setup_goxlap_camera_view_matrix(glm::vec3 xCol, glm::vec3 yCol, glm::vec3 zCol, glm::vec3 translation) {
		cam_props.position = translation;
		glm::mat4 view_matrix = glm::mat4();
		view_matrix[0] = glm::vec4(xCol, 0);
		view_matrix[1] = glm::vec4(yCol, 0);
		view_matrix[2] = glm::vec4(zCol, 0);
		view_matrix[3] = glm::vec4(translation, 1);
		cam_props.gfx_props.view = view_matrix;
	}
	void setup_goxlap_camera_frustum_planes(plane left, plane right, plane top, plane bottom, plane near, plane far) {
		cam_props.planes.left = left;
		cam_props.planes.right = right;
		cam_props.planes.top = top;
		cam_props.planes.bottom = bottom;
		cam_props.planes.near = near;
		cam_props.planes.far = far;
	}

	void setup_goxlap_initial_heap_memory(uint32_t mesh_vert_pool_cap, uint32_t chunk_struct_pool_cap, uint32_t chunk_data_uncomp_pool_cap, uint32_t chunk_data_comp_pool_cap, uint32_t node_data_pool_cap, uint32_t page_data_pool_cap,uint32_t working_mem_pool_cap)
	{
		mem_pools.mesh_vert_pool.init(mesh_vert_pool_cap);
		mem_pools.chunk_struct_pool.init(chunk_struct_pool_cap);
		mem_pools.chunk_data_uncompressed_pool.init(chunk_data_uncomp_pool_cap);
		mem_pools.chunk_data_compressed_pool.init(chunk_data_comp_pool_cap);
		mem_pools.octree_mem_pool.init(node_data_pool_cap);
		mem_pools.page_mem_pool.init(page_data_pool_cap);
		mem_pools.working_mem_pool.init(working_mem_pool_cap);
		
	}

	void setup_fastnoise() {
		noise_initialize();

	}
	float retrieve_noise(float x, float y) {
		return noise_get_2D(x, y);
	}


	bit_buffer* bit_buffer_init(int buffer_size)
	{
		bit_buffer* buffer = (bit_buffer*)mem_pools.working_mem_pool.fl_alloc(sizeof(bit_buffer));
		buffer->buffer_size = buffer_size;
		buffer->data = (uint32_t*)mem_pools.working_mem_pool.fl_alloc(buffer_size /32);

		return buffer;
	}

	int bit_buffer_read(bit_buffer* buffer, int bit_index, int bit_length) {
		const uint32_t data = buffer->data[bit_index >> 5];
		const uint32_t to = bit_index & 31;
		const uint32_t from = bit_index - bit_length + 1;
		uint32_t mask = ~create_bit_slice_mask(from, to);
		uint32_t res = (data & mask) >> (bit_index - bit_length);


		return res;
	}


	void bit_buffer_set(bit_buffer* buffer, int bit_index, int bit_length, int bits) {
		const uint32_t data = buffer->data[bit_index >> 5];
		const uint32_t to = bit_index & 31;
		const uint32_t from = bit_index - bit_length + 1;
		uint32_t mask = create_bit_slice_mask(from, to);
		uint32_t res = data & mask;
		uint32_t data_set = bits << (bit_index - bit_length);
		res = res | data_set;

		buffer->data[bit_index >> 5] = res;
	}


	voxel_chunk_palette* palette_chunk_init(int chunk_size, int size, glm::vec3 position)
	{
		voxel_chunk_palette* chunk = (voxel_chunk_palette*)mem_pools.working_mem_pool.fl_alloc(sizeof(voxel_chunk_palette));
		chunk->chunk_pos = position;
		chunk->indices_length = 1;
		chunk->palette_count = 0;
		chunk->palette_arr_count = 1 << chunk->indices_length;
		chunk->palette = (palette_entry*)mem_pools.working_mem_pool.fl_alloc(sizeof(palette_entry) * chunk->palette_arr_count);
		chunk->data = bit_buffer_init(chunk_size * chunk_size * chunk_size * chunk->indices_length);

		return chunk;
	}

	void palette_chunk_set(voxel_chunk_palette* chunk, int index, voxel_data data)
	{
		const int indices_length = chunk->indices_length;
		int palette_index = bit_buffer_read(chunk->data, index * indices_length, indices_length);
		palette_entry* curr_entry = &chunk->palette[palette_index];
		curr_entry->ref_count -= 1;

		int replace = 0;

		//Search the palette array for the voxel data type
		for (int i = 0; i < chunk->palette_arr_count; i++) {
			if(chunk->palette[i].type == data){
				replace = i;
				break;
			}
		}

		//Proceed with updating the ref count
		if (replace != -1) {
			bit_buffer_set(chunk->data, index * indices_length, indices_length,replace);
			chunk->palette[replace].ref_count += 1;
			return;
		}
		//Attempt to overwrite the palette if it 0, since its not being referenced
		if (replace == 0) {
			curr_entry->type = data;
			curr_entry->ref_count = 1;
			return;
		}

		//Final case where we need to create a new entry
		int new_entry = palette_chunk_new_entry(chunk);

		chunk->palette[new_entry].type = data;
		chunk->palette[new_entry].ref_count = 1;
		bit_buffer_set(chunk->data, index * indices_length, indices_length, new_entry);
		chunk->palette_count += 1;
	}

	int palette_chunk_new_entry(voxel_chunk_palette* chunk)
	{
		return 0;
	}

	//------------------------VOXEL OBJECT METHODS----------------------------------
	void voxel_object_init(voxel_object* vox_obj, int memory_size)
	{
		int mesh_vert_pool_cap = memory_size * 0.2f;
		int chunk_struct_pool_cap = memory_size * 0.2f;
		int chunk_data_uncompressed_cap = memory_size * 0.2f;
		int chunk_data_compressed_cap = memory_size * 0.2f;
		int working_memory_pool_cap = memory_size * 0.2f;

		int octree_memory_pool_cap = sizeof(node) * (memory_size * 0.14f);
		int octree_page_memory_pool_cap = sizeof(octree_page) * (memory_size * 0.14f);
		setup_goxlap_initial_heap_memory(mesh_vert_pool_cap, chunk_struct_pool_cap, chunk_data_uncompressed_cap, chunk_data_compressed_cap, octree_memory_pool_cap, octree_page_memory_pool_cap,working_memory_pool_cap);

		vox_object = vox_obj;
		octree_page* page = (octree_page*)mem_pools.octree_mem_pool.fl_alloc(sizeof(octree_page));
		page->origin = (glm::ivec3)vox_object->world_pos+glm::ivec3(vox_object->octree_page_size/2);
		vox_obj->pages.emplace(page->origin, page);
		
	}

	void voxel_object_insert_chunk(voxel_object* vox_obj, voxel_chunk* chunk, debug_callback callback, void* debug_object) {
		
		const int page_step_size = 128;
		glm::vec3 pos = glm::vec3(chunk->chunk_pos);
		glm::ivec3 octree_page_pos = (glm::ivec3)ceiling(chunk->chunk_pos, 128) - glm::ivec3(64,64,64);
		printf("OCTREE PAGE POS: %s \n",glm::to_string(octree_page_pos).c_str());
		if (vox_object->pages.find(octree_page_pos) == vox_obj->pages.end()) {
			octree_page* page = (octree_page*)mem_pools.octree_mem_pool.fl_alloc(sizeof(octree_page));;
			page->origin = octree_page_pos;
			vox_obj->pages.emplace(page->origin, page);
		}
		octree_page* page = vox_obj->pages.at(octree_page_pos);

		//if (page->tree.empty()) {
		//	node* root_node = (node*)mem_pools.octree_mem_pool.fl_alloc(sizeof(node));
		//	page->tree.push_back(root_node);
		//}

		int32_t level = 0;
		int current_octree_idx = 0;
		glm::vec3 parent_position_offset = glm::vec3(page->origin);
		float node_size = page_step_size;
		//This will be the size of the voxel along 1 dimension because all voxel chunks will be same across all 3 dimensions
		const float world_chunk_size = vox_obj->chunk_size * vox_object->voxel_size;
		//Pass info to debug function:
		callback(debug_object, 2, node_size, parent_position_offset);
		//TODO: make the level a variable outside of hardcoded method
		while (level < 6) {


			glm::ivec3 child_pos_indicator = {
				pos.x >= parent_position_offset.x,
				pos.y >= parent_position_offset.y,
				pos.z >= parent_position_offset.z,
			};
			glm::ivec3 child_sign_indicator = {
				pos.x >= parent_position_offset.x ? 1 : -1,
				pos.y >= parent_position_offset.y ? 1 : -1,
				pos.z >= parent_position_offset.z ? 1 : -1,
			};
			glm::vec3 child_pos_offset = {
				child_sign_indicator.x * (node_size * 0.25f),
				child_sign_indicator.y * (node_size * 0.25f),
				child_sign_indicator.z * (node_size * 0.25f),
			};
			child_pos_offset = child_pos_offset + parent_position_offset;

			int child_index = (child_pos_indicator.z << 2) | (child_pos_indicator.y << 1) | child_pos_indicator.x;
			node* child_node = (node*)mem_pools.octree_mem_pool.fl_alloc(sizeof(node));
			page->tree.push_back(child_node);
			int child_node_pos = page->tree.size()-1;
			page->tree[current_octree_idx]->children[child_index] = child_node_pos;

			current_octree_idx = child_node_pos;
			level++;
			node_size = node_size * 0.5f;
			parent_position_offset = child_pos_offset;
			callback(debug_object, 2, node_size, parent_position_offset);
		}
		if (world_chunk_size >= node_size) {
			page->tree[current_octree_idx]->voxel_chunk = chunk;
		}
		printf("Level: %d, tree size: %d \n",level, page->tree.size());
		vox_obj->pages.emplace(page->origin, page);
	}


	void voxel_object_create_octree_page_data(voxel_object* vox_obj, glm::vec3 octree_page_pos)
	{

	}

	//----------------------------------------------------------------------------
	uint32_t flatten_3d_to_1d(uint8_t x, uint8_t y, uint8_t z,uint8_t bit_width)
	{
		return uint32_t( ((z<<bit_width)|y) << bit_width | x);
	}

	packed_vertex pack_data_to_vert(uint8_t data, uint8_t x, uint8_t y, uint8_t z, uint8_t bit_width)
	{
		return packed_vertex( ((((data << bit_width)|z) << bit_width )|y) << bit_width | x  );
	}

	glm::vec3 ceiling(glm::vec3 position, float significance) {
		printf("POSITON CEILING: %s \n", glm::to_string(position).c_str());
		int q = (position.y > 0) ? 1 + (position.y - 1) / significance : (position.y / significance);
		printf("test quotient %d \n", q);
		return significance * glm::ceil(position / significance);
	}

	static inline glm::ivec3 step_offset(glm::ivec3 position, int offset, int half_offset)
	{
		printf("POSITON CEILING: %s \n", glm::to_string(position).c_str());
		return (((position)/offset)*offset) + half_offset;
	}

	unsigned int create_bit_slice_mask(int from, int to) {
		return ~((UINT_MAX >> (CHAR_BIT * sizeof(int) - to)) & (UINT_MAX << (from - 1)));
	}

	uint8_t sample_voxel(voxel_chunk* vox_chunk, int x, int y, int z,uint8_t chunk_size,uint8_t bit_width,uint8_t direction)
	{
		int mod_chunk_size = chunk_size - 1;
		if (x < 0 || x == chunk_size || y < 0 || y == chunk_size || z < 0 || z == chunk_size) {
			if (vox_chunk->neighbors == nullptr) {
				return 0;
			}
			uint8_t x1 = x & (mod_chunk_size);
			uint8_t y1 = y & (mod_chunk_size);
			uint8_t z1 = z & (mod_chunk_size);
			voxel_data* data = vox_chunk->neighbors[direction].voxel;
			return data[flatten_3d_to_1d(x1, y1, z1, bit_width)];
		}
		return vox_chunk->voxel[flatten_3d_to_1d(x, y, z,bit_width)];
	}

	void setup_voxel_object_pages(voxel_object* object)
	{
		const int max_dimension = object->voxel_object_size;
		const int step = object->octree_page_size;

		glm::ivec3 base_origin = object->world_pos;
		glm::ivec3 half_page_size = glm::ivec3(object->octree_page_size / 2);

		for (int i = step; i <= max_dimension; i += step) {
			for (int j = step; j <= max_dimension; j += step) {
				for (int k = step; k <= max_dimension; k += step) {
					glm::ivec3 position = base_origin + (glm::ivec3(i, j, k)-half_page_size);
					octree_page* page = (octree_page*)mem_pools.page_mem_pool.fl_alloc(sizeof(octree_page));
					page->origin = position;
					object->pages.emplace(position, page);
				}
			}
		}
	}

    int test_voxel_data(const float x,const float y, const float z,voxel_object* object, point_voxel_mesh_data* mesh_data, debug_callback callback, void* debug_object)
	{
		mem_alloc.init(32 * 1024 * 1024);

		
		uint8_t bit_width;
		object->chunk_size = 32;
		object->chunk_bit_width = 5;
		bit_width = object->chunk_bit_width;
		object->world_pos = glm::vec3(128, 128, 128);

		const float chunk_world_size = object->chunk_size * object->voxel_size;
		const float chunk_half_world_size = chunk_world_size * 0.5f;

		voxel_chunk* chunk = (voxel_chunk*)mem_pools.chunk_struct_pool.fl_alloc(sizeof(voxel_chunk));
		chunk->chunk_pos = step_offset(glm::vec3(x, y, z), chunk_world_size, chunk_half_world_size);

		voxel_object_insert_chunk(object, chunk,callback,debug_object);
		printf("tree size: %d \n", (uint32_t)object->pages[glm::ivec3(64,64,64)]->tree.size());
		chunk->voxel = (voxel_data*)mem_alloc.fl_alloc(32 * 32 * 32);

		object->chunk = chunk;
		const int max = 1 << 15;
		int countChoc = 0;
		for (int i = 0; i < max; i++) {
			//if(i%2==0)
			chunk->voxel[i] = 1;
			//else
			//	chunk.voxel[i] = 0;
		}
		
		uint8_t axis[3];
		uint16_t indexer = 0;
		voxel_data dir_vox[6];
		int counter = 0;
		for (axis[2] = 0; axis[2] < object->chunk_size; axis[2]++)
		for (axis[1] = 0; axis[1] < object->chunk_size; axis[1]++)
		for (axis[0] = 0; axis[0] < object->chunk_size; axis[0]++) {
			voxel_data data = chunk->voxel[indexer];
			uint8_t shift_index = 0;
			dir_vox[0] = sample_voxel(chunk, axis[0] + 1, axis[1], axis[2], 32, bit_width, 0);
			dir_vox[1] = sample_voxel(chunk, axis[0] - 1, axis[1], axis[2], 32, bit_width, 1);
			dir_vox[2] = sample_voxel(chunk, axis[0], axis[1] + 1, axis[2], 32, bit_width, 2);
			dir_vox[3] = sample_voxel(chunk, axis[0], axis[1] - 1, axis[2], 32, bit_width, 3);
			dir_vox[4] = sample_voxel(chunk, axis[0], axis[1], axis[2] + 1, 32, bit_width, 4);
			dir_vox[5] = sample_voxel(chunk, axis[0], axis[1], axis[2] - 1, 32, bit_width, 5);
			//printf("%u , %u , %u , %u , %u , %u , %u \n", data ,dir_vox[0], dir_vox[1], dir_vox[2], dir_vox[3], dir_vox[4], dir_vox[5]);
			bool is_visible = !(dir_vox[0] & dir_vox[1] & dir_vox[2] & dir_vox[3] & dir_vox[4] & dir_vox[5]);
			if ((is_visible & data)) {
				counter++;
				mesh_data->vert_data.push_back(pack_data_to_vert(data,axis[0],axis[1],axis[2],bit_width));
			}
			indexer++;
		}
		return counter;
	}

	void cleanup_goxlap_memory()
	{
	}
	
}

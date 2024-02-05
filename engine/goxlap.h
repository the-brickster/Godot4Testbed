#ifndef GOXLAP_H
#define GOXLAP_H


#include "../include/glm/glm.hpp"

#include "../include/glm/mat4x4.hpp"
#include "../include/glm/ext/matrix_clip_space.hpp"
#include "../include/glm/gtx/string_cast.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "../include/glm/gtx/hash.hpp"
#include "dummy_generator.h"
#include "memory_allocator.h"

#include <cstdarg>
#include <vector>
#include <unordered_map>

namespace goxlap {

	//------------------------GRAPHICS PROPERTIES-----------------------------------
	typedef struct gfx_properties {
		int screen_x;
		int screen_y;
		glm::mat4 projection;
		glm::mat4 inverse_projection;

		glm::mat4 view;

		glm::mat4 inv_proj_view_rot;
		glm::mat4 view_projection;
	}gfx_properties;

	typedef struct plane {
		glm::vec3 normal;
		float distance;
	}plane;

	typedef struct frustum_planes {
		plane left;
		plane right;
		plane top;
		plane bottom;
		plane near;
		plane far;
	}frustum_planes;

	typedef struct camera_properties {
		float near;
		float far;
		glm::vec3 position;
		frustum_planes planes;
		gfx_properties gfx_props;
	}camera_properties;
	//------------------------UTILITY STRUCTS---------------------------------------

	typedef struct memory_pools {
		//We will see if the data in this pool can be directly referenced, mesh data will just be a list of ints
		fl_mem_alloc mesh_vert_pool;

		//Just holds the structs for the chunks
		fl_mem_alloc chunk_struct_pool;

		//Uncompressed memory pool for voxel data
		fl_mem_alloc chunk_data_uncompressed_pool;

		//RLE compressed memory pool for voxel data
		fl_mem_alloc chunk_data_compressed_pool;

		//Working buffer for unpacking data and doing work on
		fl_mem_alloc working_mem_pool;

		//Octree Node memory pool
		fl_mem_alloc octree_mem_pool;

		//Octree page memory pool
		fl_mem_alloc page_mem_pool;

	}memory_pools;

	typedef struct bit_buffer {
		uint32_t buffer_size; //This is the size in bits not bytes
		uint32_t* data; //This is the pointer
	}bit_buffer;

	//Allocate a bit_buffer backed by a byte pointer to a block of memory
	//since its backed by a byte we will need 
	bit_buffer* bit_buffer_init(int buffer_size);

	//bit_index - this is the index to the bit buffer
	//bit_length -> length of the slice we are setting
	//bits -> this is the actual bits we are setting
	void bit_buffer_set(bit_buffer* buffer,int bit_index, int bit_length, int bits);

	int bit_buffer_read(bit_buffer* buffer,int bit_index, int bit_length);

	//------------------------VOXEL DATA STRUCTS------------------------------------
	//Debug callback for functions here
	typedef void (*debug_callback)(void*,int, ...);

	//5 bits x, 5 bits y, 5 bits z, 9 bit Flags, 8 bits material
	typedef uint32_t packed_vertex;
	typedef uint8_t voxel_data;

	//Palette Entry for voxel types
	typedef struct palette_entry {
		int ref_count;
		voxel_data type;
	}palette_entry;

	typedef struct point_voxel_mesh_data {
		std::vector<packed_vertex> vert_data;
	}point_voxel_mesh_data;

	typedef struct voxel_chunk {
		glm::vec3 chunk_pos;
		voxel_chunk* neighbors = nullptr;
		voxel_data* voxel = nullptr;
		voxel_data* voxel_rle = nullptr;
		uint32_t* voxel_mask = nullptr;
		uint32_t voxel_mask_size = 0;
		
	}voxel_chunk;

	typedef struct voxel_chunk_palette {
		glm::vec3 chunk_pos;
		bit_buffer* data;
		palette_entry* palette;
		uint32_t indices_length;
		uint32_t palette_arr_count;
		uint8_t palette_count;
	};

	typedef struct node {

		//Children of all the current node, if all of the indexes are -1 then its a leaf node
		int children[8] = {-1,-1,-1,-1,-1,-1,-1,-1};

		//this will be the downsampled mask data
		uint32_t* voxel_mask = nullptr;
		//Size of voxel mask
		uint32_t voxel_mask_size = 0;

		//If the current node is not null then it is a leaf node
		voxel_chunk* voxel_chunk = nullptr;
		~node() {
			printf("CALLING NODE DESTRUCTOR!\n");
		}
	};

	typedef struct octree_page {
		glm::vec3 origin;
		std::vector<node*> tree;
		~octree_page() {
			printf("CALLING PAGE DESTRUCTOR!\n");
		}
	}octree_page;

	//Collection of voxel chunks making an object
	//We will try a max depth of 6 for octree pages
	typedef struct voxel_object {
		//Location in world space of voxel object
		glm::vec3 world_pos;
		//Octree Page list
		std::unordered_map<const glm::vec3,octree_page*,std::hash<glm::ivec3>> pages;
		//Chunk sizes should be a power of 2
		uint8_t chunk_size;
		//The amount of bits chunk_size-1 takes
		uint8_t chunk_bit_width;
		//Chunks list TODO: we will change this later to use a octree datastructure
		voxel_chunk* chunk;
		//Voxel Mask data for rendering
		uint32_t* voxel_mask;
		//size of the voxel in meters
		float voxel_size;
		//size of the voxel object in meters
		uint32_t voxel_object_size;
		//Max octree page depth
		uint32_t max_octree_depth_size = 6;
		//octree page size in meters
		uint32_t octree_page_size = 128;
	}voxel_object;

	//------------------------GLOBAL VARS-------------------------------------------
	extern camera_properties cam_props;
	extern fl_mem_alloc mem_alloc;
	extern memory_pools mem_pools;
	extern voxel_object* vox_object;
	//------------------------VOXEL OBJECT METHODS----------------------------------
	//------PALETTE CHUNK METHODS----------
	voxel_chunk_palette* palette_chunk_init(int chunk_size,int size, glm::vec3 position);
	void palette_chunk_set(voxel_chunk_palette* chunk, int index, voxel_data data);
	voxel_data palette_chunk_get(voxel_chunk_palette* chunk, int index);
	int palette_chunk_new_entry(voxel_chunk_palette* chunk);
	void palette_chunk_grow_palette(voxel_chunk_palette* chunk);
	void palette_chunk_fit_palette(voxel_chunk_palette* chunk);
	//-------------------------------------

	void voxel_object_init(voxel_object* vox_obj, int memory_size);

	void voxel_object_insert_chunk(voxel_object* vox_obj,voxel_chunk* chunk, debug_callback callback, void* debug_object);

	void voxel_object_get_chunk(voxel_object* vox_obj, voxel_chunk* chunk, glm::vec3 chunk_pos);

	voxel_data voxel_object_get_voxel(glm::vec3 voxel);

	void voxel_object_create_octree_page_data(voxel_object* vox_obj, glm::vec3 octree_page_pos);

	//------------------------UTILITY METHODS---------------------------------------

	uint32_t flatten_3d_to_1d(uint8_t x, uint8_t y, uint8_t z, uint8_t bit_width);

	packed_vertex pack_data_to_vert(uint8_t data, uint8_t x, uint8_t y, uint8_t z, uint8_t bit_width);

	glm::vec3 ceiling(glm::vec3 position, float significance);

	static inline glm::ivec3 step_offset(glm::ivec3 position, int offset, int half_offset);

	static inline unsigned int create_bit_slice_mask(int from, int to);
	//------------------------SETUP METHODS-----------------------------------------

	void setup_goxlap_camera_proj_matrix(int screen_x, int screen_y,float near, float far, float fov, float aspect);
	void setup_goxlap_camera_view_matrix(glm::vec3 xCol, glm::vec3 yCol, glm::vec3 zCol, glm::vec3 translation);
	void setup_goxlap_camera_frustum_planes(plane left, plane right, plane top, plane bottom, plane near, plane far);

	void setup_goxlap_initial_heap_memory(uint32_t mesh_vert_pool_cap, uint32_t chunk_struct_pool_cap, uint32_t chunk_data_uncomp_pool_cap, uint32_t chunk_data_comp_pool_cap, uint32_t node_data_pool_cap, uint32_t page_data_pool_cap, uint32_t working_mem_pool_cap);
	void free_all_goxlap_initial_heap_memory();

	void setup_fastnoise();
	float retrieve_noise(float x, float y);

	uint8_t sample_voxel(voxel_chunk* vox_chunk, int x, int y, int z, uint8_t chunk_size, uint8_t bit_width, uint8_t direction);

	void setup_voxel_object_pages(voxel_object* object);

	//Has a callback funtion for the debugging view
	int test_voxel_data(const float x, const float y, const float z,voxel_object* object, point_voxel_mesh_data* mesh_data, debug_callback callback, void* debug_object);
	

	//------------------------CLEAN UP METHODS--------------------------------------

	void cleanup_goxlap_memory();
}
#endif // GOXLAP_H


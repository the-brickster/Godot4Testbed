#define RESIZEABLE_ALLOCATOR 0
#define RESIZEABLE_MALLOC_ALLOCATOR 1

#ifndef MEMORY_ALLOCATOR_H
#include "godot_imports.h"
#include <vector>
#include <list>

namespace goxlap {

	typedef struct heap_stats {
		size_t max_size;
		size_t curr_used;
		size_t peak_alloc_size;
	}heap_stats;


	typedef struct fl_node {
		bool is_malloced = false;
		int prev = -1;
		int next = -1;
		size_t size = 0;
		void* block = nullptr;
	}fl_node;

	typedef struct alloc_node {
		size_t size = 0;
	};

	typedef struct fl_mem_alloc {
		heap_stats stats;
		std::vector<fl_node*> free_nodes;
		std::list<alloc_node*> alloc_nodes;
		void* base_ptr;

		//The capacity will be in bytes similar to malloc
		void init(size_t capacity) {

			base_ptr = memalloc(capacity);
			stats.max_size = capacity;
			stats.curr_used = 0;
			stats.peak_alloc_size = 0;

			fl_node* header = (fl_node*)base_ptr;
			size_t size = sizeof(fl_node);
			size_t address = (size_t)base_ptr;
			address = address + size;
			header->block = (void*)address;
			header->size = capacity - size;
			header->prev = -1;
			header->next = -1;
			free_nodes.push_back(header);
		}
		~fl_mem_alloc() {
			stats.curr_used = 0;
			stats.max_size = 0;
			stats.peak_alloc_size = 0;
			free_nodes.clear();
			alloc_nodes.clear();
			memfree(base_ptr);
			printf("CALLING DESTRUCTOR");
		}
		//Will currently just reallocate larger space, but free block will not know
		void* fl_alloc(size_t capacity) {
			if ((stats.curr_used+capacity) >= stats.max_size || capacity >= stats.max_size) {
				#if RESIZEABLE_ALLOCATOR
								int new_size = capacity * 2 + stats.max_size * 2;

								void* new_ptr = memrealloc(base_ptr, new_size);

								fl_update_allocator(new_ptr, new_size);
				#elif RESIZEABLE_MALLOC_ALLOCATOR
					int new_size = stats.max_size+capacity * 2;
					size_t size = sizeof(fl_node);
					void* new_mem = memalloc(size + new_size);
					size_t address_offset = (size_t)new_mem + size;

					fl_node* new_free_node = (fl_node*)new_mem;
					new_free_node->is_malloced = true;
					new_free_node->size = new_size;
					new_free_node->block = (void*)address_offset;
					new_free_node->prev = free_nodes.size() - 1;
					new_free_node->next = -1;

					free_nodes.push_back(new_free_node);

				#else
								return nullptr;
				#endif // RES
			}
			fl_node* node;
			size_t alloc_cap = capacity + sizeof(alloc_node);
			//Find first fit, even if it means we have to break a block
			for (int i = 0; i < free_nodes.size(); i++) {

				if (free_nodes[i] != nullptr && free_nodes[i]->size >= alloc_cap) {

					if (free_nodes[i]->size - alloc_cap > sizeof(fl_node)) {
						size_t block_ptr_address = ((size_t)free_nodes[i]->block) + free_nodes[i]->size;
						block_ptr_address = (block_ptr_address - alloc_cap);
						alloc_node* alloc_node_ptr = (alloc_node*)block_ptr_address;
						alloc_node_ptr->size = capacity;
						void* ptr = (void*)(block_ptr_address + sizeof(alloc_node));

						free_nodes[i]->size = free_nodes[i]->size - alloc_cap;

						alloc_nodes.push_back(alloc_node_ptr);
						stats.curr_used += alloc_cap;
						stats.peak_alloc_size += alloc_cap;
						return ptr;
					}
					else if (free_nodes[i]->size - alloc_cap == sizeof(fl_node)) {
						if (free_nodes[i]->prev != -1) {

							int idx = free_nodes[i]->prev;
							free_nodes[idx]->next = free_nodes[i]->next;
						}
						if (free_nodes[i]->next != -1) {
							int idx = free_nodes[i]->next;
							free_nodes[idx]->prev = free_nodes[i]->prev;
						}
						alloc_node* alloc_node_ptr = (alloc_node*)free_nodes[i];
						alloc_node_ptr->size = capacity;
						size_t address = (size_t)(alloc_node_ptr)+sizeof(alloc_node);
						void* ptr = (void*)address;
						free_nodes[i] = nullptr;
						alloc_nodes.push_back(alloc_node_ptr);
						stats.curr_used += alloc_cap;
						stats.peak_alloc_size += alloc_cap;
						return ptr;
					}
				}
			}

			return NULL;
		};
		void fl_free(void* ptr) {
			if (ptr == NULL) {
				return;
			}
			size_t address = ((size_t)ptr) - sizeof(alloc_node);
			alloc_node* allocated_node = (alloc_node*)address;
			size_t block_size = allocated_node->size;
			//The node is too small to deallocate
			if ((block_size + sizeof(alloc_node)) - sizeof(fl_node) < sizeof(fl_node)) {
				return;
			}
			fl_node* free_node = (fl_node*)address;
			free_node->size = (block_size + sizeof(alloc_node)) - sizeof(fl_node);
			free_node->prev = -1;
			free_node->next = -1;
			free_node->block = (void*)((size_t)free_node + sizeof(fl_node));
			if (free_nodes.size() == 1) {
				fl_node* head = free_nodes[0];
				if (!fl_try_coalesce(head, free_node)) {
					head->next = 1;
					free_node->prev = 0;
					free_nodes.push_back(free_node);
					alloc_nodes.remove(allocated_node);
					stats.curr_used -= (block_size + sizeof(alloc_node));
					return;
				}
			}

			size_t best_prev_address = SIZE_MAX;
			size_t best_next_address = 0;

			for (int i = 0; i < free_nodes.size(); i++) {
				if (free_nodes[i] == nullptr) {
					continue;
				}
				fl_node* curr = free_nodes[i];
				size_t curr_address = (size_t)curr;
				bool prev_coalesce = fl_try_coalesce(curr, free_node);
				bool next_coalesce = fl_try_coalesce(free_node, curr);
				if (prev_coalesce || next_coalesce) {
					alloc_nodes.remove(allocated_node);
					return;
				}
				if (best_prev_address > curr_address && curr_address < address) {
					best_prev_address = curr_address;
					free_node->prev = i;
				}
				if (best_next_address < curr_address && curr_address > address) {
					best_next_address = curr_address;
					free_node->next = i;
				}
			}
			stats.curr_used -= (block_size + sizeof(alloc_node));
			alloc_nodes.remove(allocated_node);
			free_nodes.push_back(free_node);
		}

		bool fl_try_coalesce(fl_node* curr, fl_node* next) {
			size_t curr_address = (size_t)curr;
			size_t curr_next_address = curr_address + sizeof(fl_node) + curr->size;
			size_t next_address = (size_t)next;

			if (curr_next_address != next_address) {
				return false;
			}
			curr->size = curr->size + sizeof(fl_node) + next->size;
			return true;
		}
		void fl_update_allocator(void* ptr, int new_size) {
			void* working_ptr = ptr;
			if (ptr == NULL) {
				working_ptr = base_ptr;
			}
			std::vector<fl_node*> new_free_node_list;
			std::list<alloc_node*> new_alloc_node_list;
			size_t ptr_address = (size_t)working_ptr;
			size_t base_ptr_address = (size_t)base_ptr;

			size_t free_chunk_offset = new_size - stats.max_size;
			fl_node* extended_node = (fl_node*)(ptr_address + stats.max_size);
			extended_node->next = -1;
			extended_node->prev = -1;
			extended_node->size = (free_chunk_offset - sizeof(fl_node));
			extended_node->block = (void*)(((size_t)extended_node) + sizeof(fl_node));


			int i = 0;
			int new_idx = 0;
			while (i != -1) {
				fl_node* curr = free_nodes[i];
				size_t curr_address = (size_t)curr;
				size_t cur_ptr_offset = curr_address - base_ptr_address;

				fl_node* copy_node = (fl_node*)(ptr_address + cur_ptr_offset);
				//copy_node->size = curr->size;
				copy_node->block = (void*)((size_t)copy_node + sizeof(fl_node));
				i = copy_node->next;
				if (copy_node->next != -1) {
					copy_node->next = new_idx + 1;
				}
				if (copy_node->prev != -1) {
					copy_node->prev = new_idx - 1;
				}
				new_idx++;

				new_free_node_list.push_back(copy_node);
			}

			if (new_free_node_list.size() > 0) {
				fl_node* top = new_free_node_list[new_free_node_list.size() - 1];
				top->next = new_free_node_list.size();

				extended_node->prev = new_free_node_list.size() - 1;
				new_free_node_list.push_back(extended_node);
			}

			std::list<alloc_node*>::iterator it;
			for (it = alloc_nodes.begin(); it != alloc_nodes.end(); ++it) {
				alloc_node* curr_node = *it;
				size_t curr_address = (size_t)curr_node;

				size_t curr_alloc_offset = curr_address - base_ptr_address;

				alloc_node* copy_alloc_node = (alloc_node*)(ptr_address + curr_alloc_offset);
				new_alloc_node_list.push_back(copy_alloc_node);
			}
			stats.max_size = new_size;
			free_nodes = new_free_node_list;
			alloc_nodes = new_alloc_node_list;
			base_ptr = ptr;
		}
	}fl_mem_alloc;

}

#endif

#ifndef _DPLIST_H_
#define _DPLIST_H_


typedef enum {false, true} bool;

typedef struct dplist dplist_t;

typedef struct dplist_node dplist_node_t;

dplist_t * dpl_create (
			  void * (*element_copy)(void * element),
			  void (*element_free)(void ** element),
			  int (*element_compare)(void * x, void * y)
			);

void dpl_free(dplist_t ** list, bool free_element);
dplist_t * dpl_insert_at_index(dplist_t * list, void * element, int index, bool insert_copy);
dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool free_element);
int dpl_size( dplist_t * list );
dplist_node_t * dpl_get_reference_at_index( dplist_t * list, int index );
void * dpl_get_element_at_index( dplist_t * list, int index );
int dpl_get_index_of_element( dplist_t * list, void * element );
dplist_node_t * dpl_get_first_reference( dplist_t * list );
dplist_node_t * dpl_get_last_reference( dplist_t * list );
dplist_node_t * dpl_get_next_reference( dplist_t * list, dplist_node_t * reference );
dplist_node_t * dpl_get_previous_reference( dplist_t * list, dplist_node_t * reference );
void * dpl_get_element_at_reference( dplist_t * list, dplist_node_t * reference );
dplist_node_t * dpl_get_reference_of_element( dplist_t * list, void * element );
int dpl_get_index_of_reference( dplist_t * list, dplist_node_t * reference );
dplist_t * dpl_insert_at_reference( dplist_t * list, void * element, dplist_node_t * reference, bool insert_copy );
dplist_t * dpl_insert_sorted( dplist_t * list, void * element, bool insert_copy );
dplist_t * dpl_remove_at_reference( dplist_t * list, dplist_node_t * reference, bool free_element );
dplist_t * dpl_remove_element( dplist_t * list, void * element, bool free_element );

#endif  // _DPLIST_H_

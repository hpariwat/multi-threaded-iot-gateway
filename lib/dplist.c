#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "dplist.h"

#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 //error due to a list operation applied on a NULL list

#ifdef DEBUG
	#define DEBUG_PRINTF(...) \
		do { \
			fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__); \
			fprintf(stderr,__VA_ARGS__); \
			fflush(stderr); \
        } while(0)
#else
	#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition, err_code) \
	do { \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n"); \
            assert(!(condition)); \
	} while(0)

struct dplist_node {
	dplist_node_t * prev, * next;
	void * element;
};

struct dplist {
	dplist_node_t * head;
	void * (*element_copy)(void * src_element);
	void (*element_free)(void ** element);
	int (*element_compare)(void * x, void * y);
};


dplist_t * dpl_create (// callback functions
				void * (*element_copy)(void * src_element),
				void (*element_free)(void ** element),
				int (*element_compare)(void * x, void * y)
				)
{
	dplist_t * list;
	list = malloc(sizeof(struct dplist));
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_MEMORY_ERROR);
	list->head = NULL;
	list->element_copy = element_copy;
	list->element_free = element_free;
	list->element_compare = element_compare;
	return list;
}

void dpl_free(dplist_t ** list, bool free_element)
{
	assert(*list != NULL);
	dplist_node_t * current = (*list)->head;
	dplist_node_t * next;

	while (current != NULL) {
		next = current->next;

		if (free_element)
			(*list)->element_free( &(current->element) );

		free(current);
		current = next;
	}

	free(*list);
	*list = NULL;
}

dplist_t * dpl_insert_at_index(dplist_t * list, void * element, int index, bool insert_copy)
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	dplist_node_t * ref_at_index;
	dplist_node_t * list_node;

	list_node = malloc(sizeof(dplist_node_t));
	DPLIST_ERR_HANDLER(list_node == NULL, DPLIST_MEMORY_ERROR);

	if (insert_copy)
		list_node->element = list->element_copy(element);
	else
		list_node->element = element;

	if (list->head == NULL) { // insert into an empty list
		list_node->prev = NULL;
		list_node->next = NULL;
		list->head = list_node;

	} else if (index <= 0) { // insert at the beginning of the list
		list_node->prev = NULL;
		list_node->next = list->head;
		list->head->prev = list_node;
		list->head = list_node;

	} else { // if index > 0

		ref_at_index = dpl_get_reference_at_index(list, index);
		assert(ref_at_index != NULL);

		if (index < dpl_size(list)){ // insert at the middle of the list
			list_node->prev = ref_at_index->prev;
			list_node->next = ref_at_index;
			ref_at_index->prev->next = list_node;
			ref_at_index->prev = list_node;

		} else { // insert at the end of the list
			assert(ref_at_index->next == NULL);
			list_node->next = NULL;
			list_node->prev = ref_at_index;
			ref_at_index->next = list_node;
		}
	}
	return list;
}

dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool free_element)
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	dplist_node_t * ref_at_index;

	if (list->head == NULL)
		return list;

	if (index <= 0) {
		ref_at_index = list->head;

		if (ref_at_index->next != NULL) { // if there are still some nodes left
			list->head = ref_at_index->next;
			ref_at_index->next->prev = NULL;

		} else // if there is only one node
			list->head = NULL;

	} else {
		ref_at_index = dpl_get_reference_at_index(list, index);
		assert(ref_at_index != NULL);

		if (ref_at_index->next != NULL) { // if there are still some nodes left
			ref_at_index->prev->next = ref_at_index->next;
			ref_at_index->next->prev = ref_at_index->prev;

		} else {
			if (ref_at_index->prev != NULL) {
				ref_at_index->prev->next = NULL;
			} else // if there is only one node
				list->head = NULL;
		}
	}

	if (free_element)
		list->element_free( &(ref_at_index->element) );

	free(ref_at_index);
	return list;
}

int dpl_size( dplist_t * list )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL)
		return 0;

	int count = 1;
	dplist_node_t * dummy = list->head;

	for (count = 1; dummy->next != NULL; count++)
		dummy = dummy->next;

	return count;
}

dplist_node_t * dpl_get_reference_at_index( dplist_t * list, int index )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL)
		return NULL;

	dplist_node_t * dummy = list->head;

	for (int i = 0; i < index && dummy->next != NULL; i++)
		dummy = dummy->next;

	return dummy;
}

void * dpl_get_element_at_index( dplist_t * list, int index )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL)
		return (void*)0;

	dplist_node_t * dummy = list->head;

	for (int i = 0; i < index && dummy->next != NULL; i++)
		dummy = dummy->next;

	return dummy->element;
}

int dpl_get_index_of_element( dplist_t * list, void * element )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL)
		return -1;

	int i = 0;
	dplist_node_t * dummy = list->head;

	for (i = 0; list->element_compare(dummy->element, element) != 0; i++) {
		if (dummy->next != NULL)
			dummy = dummy->next;
		else
			return -1;
	}

	return i;
}

dplist_node_t * dpl_get_first_reference( dplist_t * list )
{
    DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL)
		return NULL;

	return list->head;
}

dplist_node_t * dpl_get_last_reference( dplist_t * list )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL)
		return NULL;

    return dpl_get_reference_at_index(list, dpl_size(list));
}

dplist_node_t * dpl_get_next_reference( dplist_t * list, dplist_node_t * reference )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL || reference == NULL)
		return NULL;

	int index = dpl_get_index_of_element(list, reference->element);

	if (index  == -1)
		return NULL;

	return dpl_get_reference_at_index(list, index)->next;
}

dplist_node_t * dpl_get_previous_reference( dplist_t * list, dplist_node_t * reference )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL || reference == NULL)
		return NULL;

	int index = dpl_get_index_of_element(list, reference->element);

	if (index  == -1)
		return NULL;

	return dpl_get_reference_at_index(list, index)->prev;
}

void * dpl_get_element_at_reference( dplist_t * list, dplist_node_t * reference )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL)
		return NULL;

	if (reference == NULL)
		return dpl_get_reference_at_index(list, dpl_size(list))->element;

	if (dpl_get_index_of_element(list, reference->element) == -1)
		return NULL;

	return reference->element;
}

dplist_node_t * dpl_get_reference_of_element( dplist_t * list, void * element )
{
    DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL)
		return NULL;

	int index = dpl_get_index_of_element(list, element);

	if (index  == -1)
		return NULL;

	return dpl_get_reference_at_index(list, index);
}

int dpl_get_index_of_reference( dplist_t * list, dplist_node_t * reference )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL)
		return -1;

	if (reference == NULL)
		return dpl_size(list) - 1;

	int index = dpl_get_index_of_element(list, reference->element);

	if (index == -1)
		return -1;

	return index;
}

dplist_t * dpl_insert_at_reference( dplist_t * list, void * element, dplist_node_t * reference, bool insert_copy )
{
    DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (reference == NULL)
		return dpl_insert_at_index(list, element, dpl_size(list), insert_copy);

	int index = dpl_get_index_of_element(list, reference->element);

	if (index == -1)
		return list;

	return dpl_insert_at_index(list, element, index, insert_copy);
}

dplist_t * dpl_insert_sorted( dplist_t * list, void * element, bool insert_copy )
{
    DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (list->head == NULL)
		return dpl_insert_at_index(list, element, 0, insert_copy);

	dplist_node_t * dummy = list->head;
	int index;

	if (dummy->next == NULL) {
		if ( list->element_compare(dummy->element, element) >= 0 )
			return dpl_insert_at_index(list, element, 0, insert_copy);
		else
			return dpl_insert_at_index(list, element, 1, insert_copy);
	}

	for (index = 0; dummy->next != NULL; index++) {
		if ( list->element_compare(dummy->element, element) >= 0 )
			return dpl_insert_at_index(list, element, index, insert_copy);
		else
			dummy = dummy->next;
	}

	if ( list->element_compare(dummy->element, element) >= 0 )
		return dpl_insert_at_index(list, element, index, insert_copy);
	else
		return dpl_insert_at_index(list, element, index + 1, insert_copy);
}

dplist_t * dpl_remove_at_reference( dplist_t * list, dplist_node_t * reference, bool free_element )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	if (reference == NULL)
		return dpl_remove_at_index(list, dpl_size(list), free_element);

	int index = dpl_get_index_of_element(list, reference->element);

	if (index == -1)
		return list;

	return dpl_remove_at_index(list, index, free_element);
}

dplist_t * dpl_remove_element( dplist_t * list, void * element, bool free_element )
{
	DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

	int index = dpl_get_index_of_element(list, element);

	if (index == -1)
		return list;

	return dpl_remove_at_index(list, index, free_element);
}

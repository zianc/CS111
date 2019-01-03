
// NAME: Haiying Huang
// EMAIL: hhaiying1998@outlook.com
// ID: 804757410
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include "SortedList.h"


/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *	The specified element will be inserted in to
 *	the specified list, which will be kept sorted
 *	in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
	SortedListElement_t* curr = list->next; 
	while (curr != list && strcmp(curr->key, element->key) <= 0)
		curr = curr->next; 
	// find the first element larger than the element to be inserted

	if (opt_yield & INSERT_YIELD)
		sched_yield();

	element->prev = curr->prev;
	element->next = curr;
	curr->prev->next = element;
	curr->prev = element;
}


/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *	The specified element will be removed from whatever
 *	list it is currently in.
 *
 *	Before doing the deletion, we check to make sure that
 *	next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete(SortedListElement_t *element)
{
	if (! element->key)
		return 1; // attempt to delete the header

	if (element->prev->next != element ||
		element->next->prev != element)
		// find corrupted pointer
		return 1;

	element->prev->next = element->next;
	if (opt_yield & DELETE_YIELD)
		sched_yield();
	element->next->prev = element->prev;
	return 0;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *	The specified list will be searched for an
 *	element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
	if (! key)
		return NULL; // cannot delete the header

	SortedListElement_t* curr = list->next;
	while (curr != list && strcmp(curr->key, key) <= 0)
	{
		if (opt_yield & LOOKUP_YIELD)
			sched_yield();

		if (strcmp(curr->key, key) == 0)
			return curr;
		curr = curr->next;
	}
	// no matched element is found
	return NULL;
}


/*
 * SortedList_length ... count elements in a sorted list
 *	While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *	   -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list)
{
	SortedListElement_t* curr = list->next;
	if (curr->prev != list)
		return -1;

	int count = 0;
	while (curr != list)
	{
		if (curr->next->prev != curr)
			return -1;
		count += 1;
		curr = curr->next;
		if (opt_yield & LOOKUP_YIELD)
			sched_yield();
	}
	return count;
}

#ifndef __LIST_H__
#define __LIST_H__

#include <types.h>
#define offset(struct_type,member) (int32)(&((struct_type*)0)->member)
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
	 (struct_type*)((int32)elem_ptr - offset(struct_type, struct_member_name))
#define element_node_offset(type, node, key) ((int32)(&((type *)0)->key) - (int32)(&((type *)0)->node))
#define element_node_key(node, offset) *(int32 *)((int32)node + offset)

struct list_node
{
    struct list_node* pre;
    struct list_node* next;
};

struct list
{
    struct list_node head;
    struct list_node tail;
};

typedef bool (function)(struct list_node*, uint32 arg);

void list_init (struct list*);
void list_insert_before(struct list_node* before, struct list_node* elem);
void list_push(struct list* plist, struct list_node* elem);
void list_iterate(struct list* plist);
void list_append(struct list* plist, struct list_node* elem);
void list_remove(struct list_node* pelem);
struct list_node* list_pop(struct list* plist);
bool list_empty(struct list* plist);
uint32 list_len(struct list* plist);
struct list_node* list_traversal(struct list* plist, function func, uint32 arg);
bool elem_find(struct list* plist, struct list_node* obj_elem);
struct list_node *list_pop_back(struct list *plist);
bool list_search(struct list *list, struct list_node *node);
void list_append_lock(struct list* plist, struct list_node* elem);
void list_push_lock(struct list* plist, struct list_node* elem);
void list_remove_lock(struct list_node* pelem);
void list_insert_before_lock(struct list_node* before, struct list_node* elem);
struct list_node* list_pop_lock(struct list* plist);
void list_insert_sort(struct list *list, struct list_node *node, int32 offset);
#endif

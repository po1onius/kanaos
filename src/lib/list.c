#include <list.h>
#include <intr.h>
#include <assert.h>

/* 初始化双向链表list */
void list_init (struct list* list)
{
    list->head.pre = NULL;
    list->head.next = &list->tail;
    list->tail.pre = &list->head;
    list->tail.next = NULL;
}

/* 把链表元素elem插入在元素before之前 */
void list_insert_before(struct list_node* before, struct list_node* elem)
{


/* 将before前驱元素的后继元素更新为elem, 暂时使before脱离链表*/
    before->pre->next = elem;

/* 更新elem自己的前驱结点为before的前驱,
 * 更新elem自己的后继结点为before, 于是before又回到链表 */
    elem->pre = before->pre;
    elem->next = before;

/* 更新before的前驱结点为elem */
    before->pre = elem;


}

/* 添加元素到列表队首,类似栈push操作 */
void list_push(struct list* plist, struct list_node* elem)
{
    list_insert_before(plist->head.next, elem); // 在队头插入elem
}

void list_push_lock(struct list* plist, struct list_node* elem)
{
    list_insert_before_lock(plist->head.next, elem); // 在队头插入elem
}

/* 追加元素到链表队尾,类似队列的先进先出操作 */
void list_append(struct list* plist, struct list_node* elem)
{
    list_insert_before(&plist->tail, elem);     // 在队尾的前面插入
}

void list_append_lock(struct list* plist, struct list_node* elem)
{
    list_insert_before_lock(&plist->tail, elem);     // 在队尾的前面插入
}

/* 使元素pelem脱离链表 */
void list_remove(struct list_node* pelem)
{
    assert(pelem->pre != NULL);
    assert(pelem->next != NULL);
    pelem->pre->next = pelem->next;
    pelem->next->pre = pelem->pre;
    pelem->next = NULL;
    pelem->pre = NULL;
}

/* 将链表第一个元素弹出并返回,类似栈的pop操作 */
struct list_node* list_pop(struct list* plist)
{
    struct list_node* elem = plist->head.next;
    list_remove(elem);
    return elem;
}

struct list_node* list_pop_lock(struct list* plist)
{
    struct list_node* elem = plist->head.next;
    list_remove_lock(elem);
    return elem;
}

// 移除尾结点前的结点
struct list_node *list_pop_back(struct list *plist)
{
    assert(!list_empty(plist));

    struct list_node *node = plist->tail.pre;
    list_remove(node);

    return node;
}

// 查找链表中结点是否存在
bool list_search(struct list *list, struct list_node *node)
{
    struct list_node *next = list->head.next;
    while (next != &list->tail)
    {
        if (next == node)
        {
            return true;
        }
        next = next->next;
    }
    return false;
}

/* 从链表中查找元素obj_elem,成功时返回true,失败时返回false */
bool elem_find(struct list* plist, struct list_node* obj_elem)
{
    struct list_node* elem = plist->head.next;
    while (elem != &plist->tail)
    {
        if (elem == obj_elem)
        {
            return true;
        }
        elem = elem->next;
    }
    return false;
}

/* 把列表plist中的每个元素elem和arg传给回调函数func,
 * arg给func用来判断elem是否符合条件.
 * 本函数的功能是遍历列表内所有元素,逐个判断是否有符合条件的元素。
 * 找到符合条件的元素返回元素指针,否则返回NULL. */
struct list_node* list_traversal(struct list* plist, function func, uint32 arg)
{
    struct list_node* elem = plist->head.next;
/* 如果队列为空,就必然没有符合条件的结点,故直接返回NULL */
    if (list_empty(plist))
    {
        return NULL;
    }

    while (elem != &plist->tail)
    {
        if (func(elem, arg))
        {		  // func返回ture则认为该元素在回调函数中符合条件,命中,故停止继续遍历
            return elem;
        }					  // 若回调函数func返回true,则继续遍历
        elem = elem->next;
    }
    return NULL;
}

/* 返回链表长度 */
uint32 list_len(struct list* plist)
{
    struct list_node* elem = plist->head.next;
    uint32 length = 0;
    while (elem != &plist->tail)
    {
        length++;
        elem = elem->next;
    }
    return length;
}

/* 判断链表是否为空,空时返回true,否则返回false */
bool list_empty(struct list* plist) // 判断队列是否为空
{
    return (plist->head.next == &plist->tail ? true : false);
}

void list_remove_lock(struct list_node* pelem)
{
    enum intr_status old_status = intr_disable();
    list_remove(pelem);
    intr_set_status(old_status);
}

void list_insert_before_lock(struct list_node* before, struct list_node* elem)
{
    enum intr_status old_status = intr_disable();
    list_insert_before(before, elem);
    intr_set_status(old_status);
}

void list_insert_sort(struct list *list, struct list_node *node, int32 offset)
{
    struct list_node *anchor = &list->tail;
    int32 key = element_node_key(node, offset);
    for (struct list_node *ptr = list->head.next; ptr != &list->tail; ptr = ptr->next)
    {
        int32 compare = element_node_key(ptr, offset);
        if (compare > key)
        {
            anchor = ptr;
            break;
        }
    }

    assert(node->next == NULL);
    assert(node->pre == NULL);

    // 插入链表
    list_insert_before_lock(anchor, node);
}
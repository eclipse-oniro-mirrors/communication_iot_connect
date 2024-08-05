/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef UTILS_LIST_H
#define UTILS_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ListEntryDef {
    struct ListEntryDef *next;
    struct ListEntryDef *prev;
} ListEntry;

/* 声明时初始化 */
#define LIST_DECLARE_INIT(head) {head, head}

/* 初始化 */
#define LIST_INIT(item) do {   \
    (item)->next = (item)->prev = (item);   \
} while (0)

/* 判断链表释放为空 */
#define LIST_EMPTY(head) ((head)->next == (head))

#define LIST_NOT_INIT(head) ((head)->next == NULL || (head)->prev == NULL)

/* 获取队首元素 */
#define LIST_HEAD(head) (LIST_EMPTY(head) ? NULL : (head)->next)

/* 获取队尾元素 */
#define LIST_TAIL(head) (LIST_EMPTY(head) ? NULL : (head)->prev)

/* 获取下一个元素 */
#define LIST_NEXT(item) ((item)->next)

/* 获取上一个元素 */
#define LIST_PREV(item) ((item)->prev)

/* 在指定位置后面插入 */
#define LIST_INSERT(item, where) do {  \
    (item)->next = (where)->next;    \
    (item)->prev = (where);  \
    (where)->next = (item);  \
    (item)->next->prev = (item); \
} while (0)

/* 在指定位置前面插入 */
#define LIST_INSERT_BEFORE(item, where) \
    LIST_INSERT(item, (where)->prev)

/* 从链表中删除 */
#define LIST_REMOVE(item) do {  \
    (item)->prev->next = (item)->next;  \
    (item)->next->prev = (item)->prev;  \
} while (0)

/* 遍历列表 */
#define LIST_FOR_EACH_ITEM(item, head)  \
    for ((item) = (head)->next; (item) != (head); (item) = (item)->next)

/* 安全遍历列表，循环遍历删除时使用 */
#define LIST_FOR_EACH_ITEM_SAFE(item, tmp, head)   \
    for ((item) = (head)->next, (tmp) = (item)->next;  \
         (item) != (head);  \
         (item) = (tmp), (tmp) = (item)->next)

/* 逆序遍历列表 */
#define LIST_FOR_EACH_ITEM_REV(item, head)  \
    for ((item) = (head)->prev; (item) != (head); (item) = (item)->prev)

/* 根据成员的指针地址计算所属结构体类型对象的地址 */
#define CONTAINER_OF(memberAddr, type, member)  \
        ((type *)((char *)(memberAddr) - (char *)(&((type *)0)->member)))

#ifdef __cplusplus
}
#endif
#endif

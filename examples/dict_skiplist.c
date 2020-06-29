#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wuy_dict.h"
#include "wuy_skiplist.h"

/*
 * In this example, we will add this struct into a dict
 * with key=id value=score, and insert this struct into
 * a skiplist sorted by (score+id).
 */
struct student_score {
	uint32_t		id;
	float			score;

	wuy_dict_node_t 	dict_node; /* embed this to use dict */

	wuy_skiplist_node_t	skiplist_node; /* embed this to use skiplist */
};

static bool ss_less(const void *a, const void *b)
{
	const struct student_score *sa = a;
	const struct student_score *sb = b;

	if (sa->score == sb->score) {
		return sa->id < sb->id;
	}
	return sa->score > sb->score; /* in increase order */
}	

int main()
{
	/* create a dict, with uint32 key */
	wuy_dict_t *dict = wuy_dict_new_type(WUY_DICT_KEY_UINT32,
			offsetof(struct student_score, id),
			offsetof(struct student_score, dict_node));

	/* create a skiplist, since we want to sort students with same
	 * score by ID, so we do not use WUY_SKIPLIST_KEY_FLOAT, while
	 * we define compare function ss_less. */
	wuy_skiplist_t *skiplist = wuy_skiplist_new_func(ss_less,
			offsetof(struct student_score, skiplist_node), 8);

	struct student_score nodes[10];

	/* init and insert nodes */
	printf("insert nodes:\n");
	for (int i = 0; i < 10; i++) {
		struct student_score *ss = &nodes[i];
		ss->id = i;
		ss->score = (float)(random() % 1000) / 10;

		wuy_dict_add(dict, ss);
		wuy_skiplist_insert(skiplist, ss);

		printf("  id=%d score=%.1f\n", ss->id, ss->score);
	}

	struct student_score *ss, *fail;

	/* iterate the skiplist */
	printf("\niterate the skiplist by score:\n");
	wuy_skiplist_iter(skiplist, ss) {
		printf("  id=%d score=%.1f\n", ss->id, ss->score);
	}

	/* search dict by id. Since the dict is create by WUY_DICT_KEY_UINT32,
	 * we just pass the key value to wuy_dict_get() to search. */
	ss = wuy_dict_get(dict, 3);
	printf("\nsearch dict by id=%d : score=%.1f\n", ss->id, ss->score);

	/* delete node from dict. You must make sure that the node is in dict! */
	wuy_dict_delete(dict, ss);
	printf("\ndelete from dict: you make sure the node is in dict.\n");

	/* search it again which will fail */
	fail = wuy_dict_get(dict, 3);
	printf("\nsearch the deleted key will fail: %p\n", fail);

	/* delete node from skiplist. Return NULL if the node is not in. */
	bool ret = wuy_skiplist_delete(skiplist, ss);
	printf("\ndelete from skiplist: %d\n", ret);

	ret = wuy_skiplist_delete(skiplist, ss);
	printf("\ndelete not-linked node from skiplist will fail: %d\n", ret);

	/* delete by key from dict. Return NULL if not found. */
	ss = wuy_dict_del_key(dict, 4);
	printf("\ndelete from dict by key id=%d : score=%.1f\n", ss->id, ss->score);

	fail = wuy_dict_del_key(dict, 4);
	printf("\ndelete the deleted key will fail: %p\n", fail);

	/* search from skiplist by score. Since the skiplist is create by use-defined
	 * functions, we have to pass the data-struct as search key. */
	struct student_score search_score = { .id = ss->id, .score = ss->score };
	ss = wuy_skiplist_search(skiplist, &search_score);
	printf("\nsearch from skiplist by score=%.1f: id=%d\n", ss->score, ss->id);

	/* delete by key from skiplist */
	ss = wuy_skiplist_del_key(skiplist, &search_score);
	printf("\ndelete from skiplist by score score=%.1f, id=%d\n", ss->score, ss->id);

	/* iterate the skiplist */
	printf("\niterate skiplist again after deleting:\n");
	wuy_skiplist_iter(skiplist, ss) {
		printf("  id=%d score=%.1f\n", ss->id, ss->score);
	}

	printf("done.\n");

	return 0;
}

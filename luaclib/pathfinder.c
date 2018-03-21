

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "list.h"
#include "minheap.h"
#include "pathfinder.h"
#ifndef _MSC_VER
#include <stdbool.h>
#else
#define inline __inline
#define false 0
#endif

#define MARK_MAX 64

#define SQUARE(val) (val*val)
#define GOAL_COST(from,to,cost) (abs(from->x - to->x) * cost + abs(from->z - to->z) * cost)
#define DX(A,B) (A->x - B->x)
#define DZ(A,B) (A->z - B->z)

typedef struct node {
	struct list_node list_node;
	struct element elt;
	struct node   *parent;
	int 		   x;
	int 		   z;
	int			   block;
	float 		   G;
	float 		   H;
	float 		   F;
} node_t;

typedef struct pathfinder {
	int width;
	int heigh;
	node_t *node;
	char *data;
	char mask[MARK_MAX];

	struct minheap* openlist;
	struct list closelist;
	struct list neighbors;
} pathfinder_t;

static int direction[8][2] = {
	{ -1, 0 },
	{ 1, 0 },
	{ 0, -1 },
	{ 0, 1 },
	{ -1, -1 },
	{ -1, 1 },
	{ 1, -1 },
	{ 1, 1 },
};

#define CAST(elt) (node_t*)((int8_t*)elt - sizeof(struct list_node))

static inline node_t*
find_node(pathfinder_t* finder,int x,int z) {
	if (x < 0 || x >= finder->width || z < 0 || z >= finder->heigh)
		return NULL;
	return &finder->node[x*finder->heigh + z];
}

static inline struct list *
find_neighbors(pathfinder_t * finder, struct node * node) {
	assert(list_empty(&finder->neighbors) == 1);

	int i;
	for (i = 0; i < 8; i++) {
		int x = node->x + direction[i][0];
		int z = node->z + direction[i][1];
		node_t * neighbor = find_node(finder, x, z);
		if (neighbor) {
			if (neighbor->list_node.pre || neighbor->list_node.next)
				continue;
			if (neighbor->block != 0)
				list_push(&finder->neighbors, (struct list_node*)neighbor);
		}
	}
	
	if (list_empty(&finder->neighbors))
		return NULL;

	return &finder->neighbors;
}

static inline float
neighbor_cost(node_t * from, node_t * to) {
	int dx = from->x - to->x;
	int dz = from->z - to->z;
	int i;
	for (i = 0; i < 8; ++i) {
		if (direction[i][0] == dx && direction[i][1] == dz)
			break;
	}
	if (i < 4)
		return 50.0f;
	return 60.0f;
}

static inline void
clear_node(node_t* node) {
	node->parent = NULL;
	node->F = node->G = node->H = 0;
	node->elt.index = 0;
}

static inline void
heap_clear(struct element* elt) {
	node_t *node = CAST(elt);
	clear_node(node);
}

static inline void
reset(pathfinder_t* finder) {
	node_t * node = NULL;
	while ((node = (node_t*)list_pop(&finder->closelist))) {
		clear_node(node);
	}
	minheap_clear(finder->openlist, heap_clear);
}

static inline int
movable(pathfinder_t* finder, int x, int z, int ignore) {
	node_t *node = find_node(finder, x, z);
	if (node == NULL)
		return 0;
	if (ignore)
		return node->block != 0;
	return finder->mask[node->block] == 1;
}

static inline int
less(struct element * left, struct element * right) {
	node_t *l = CAST(left);
	node_t *r = CAST(right);
	return l->F < r->F;
}

void
make_path(pathfinder_t *finder, node_t *current, node_t *from, finder_cb cb, void* ud) {
	cb(ud, current->x, current->z);

	node_t * parent = current->parent;
	assert(parent != NULL);

	int dx0 = DX(current, parent);
	int dz0 = DZ(current, parent);

	current = parent;
	while (current) {
		if (current != from) {
			parent = current->parent;
			if (parent != NULL) {
				int dx1 = DX(current, parent);
				int dz1 = DZ(current, parent);
				if (dx0 != dx1 || dz0 != dz1) {
					cb(ud, current->x, current->z);
					dx0 = dx1;
					dz0 = dz1;
				}
			} else {
				cb(ud, current->x, current->z);
				break;
			}

		} else {
			cb(ud, current->x, current->z);
			break;
		}
		current = current->parent;
	}
}

pathfinder_t* 
finder_create(int width, int heigh, char* data) {
	pathfinder_t *finder = (pathfinder_t*)malloc(sizeof(*finder));
	memset(finder, 0, sizeof(*finder));

	finder->width = width;
	finder->heigh = heigh;

	finder->node = (node_t*)malloc(width * heigh * sizeof(node_t));
	memset(finder->node, 0, width * heigh * sizeof(node_t));

	finder->data = (char*)malloc(width * heigh);
	memset(finder->data, 0, width * heigh);
	memcpy(finder->data, data, width * heigh);

	int i = 0;
	int j = 0;
	for (; i < finder->width; ++i) {
		for (j = 0; j < finder->heigh; ++j) {
			int index = i*finder->heigh + j;
			node_t *node = &finder->node[index];
			node->x = i;
			node->z = j;
			node->block = finder->data[index];
		}
	}

	finder->openlist = minheap_create(50 * 50, less);
	list_init(&finder->closelist);
	list_init(&finder->neighbors);

	return finder;
}

void 
finder_release(pathfinder_t* finder) {
	free(finder->node);
	free(finder->data);
	minheap_release(finder->openlist);
	free(finder);
}

int 
finder_find(pathfinder_t * finder, int x0, int z0, int x1, int z1, finder_cb cb, void* cb_args, finder_dump dump, void* dump_args,float cost) {
	node_t * from = find_node(finder, x0, z0);
	node_t * to = find_node(finder, x1, z1);

	if (!from || !to || from == to || to->block == 0)
		return 0;

	minheap_push(finder->openlist, &from->elt);

	node_t * current = NULL;

	for (;;) {
		struct element * elt = minheap_pop(finder->openlist);
		if (!elt) {
			reset(finder);
			return 0;
		}
		current = CAST(elt);
		list_push(&finder->closelist, (struct list_node *)current);

		if (current == to) {
			make_path(finder, current, from, cb, cb_args);
			reset(finder);
			return 1;
		}

		struct list * neighbors = find_neighbors(finder, current);
		if (neighbors) {
			node_t * node;
			while ((node = (node_t*)list_pop(neighbors))) {
				if (node->elt.index) {
					int nG = current->G + neighbor_cost(current, node);
					if (nG < node->G) {
						node->G = nG;
						node->F = node->G + node->H;
						node->parent = current;
						minheap_change(finder->openlist, &node->elt);
					}
				} else {
					node->parent = current;
					node->G = current->G + neighbor_cost(current, node);
					node->H = GOAL_COST(node, to,cost);
					node->F = node->G + node->H;
					minheap_push(finder->openlist, &node->elt);
					if (dump != NULL)
						dump(dump_args, node->x, node->z);
				}
			}
		}
	}
}

void 
finder_raycast(pathfinder_t* finder, int x0, int z0, int x1, int z1, int ignore, int* resultx, int* resultz, finder_dump dump, void* ud) {
	float fx0 = x0 + 0.5f;
	float fz0 = z0 + 0.5f;
	float fx1 = x1 + 0.5f;
	float fz1 = z1 + 0.5f;
	float rx = fx0;
	float rz = fz0;
	int founded = 0;

	float slope = (fz1 - fz0) / (fx1 - fx0);
	if (fx0 == fx1) {
		if (z0 < z1) {
			float z = z0;
			for (; z <= z1; z++) {
				if (dump != NULL)
					dump(ud, x0, z);
				if (movable(finder, x0, z, ignore) == 0) {
					founded = 1;
					break;
				} else {
					rx = x0;
					rz = z;
				}
			}
		} else {
			float z = z0;
			for (; z >= z1; z--) {
				if (dump != NULL)
					dump(ud, x0, z);
				if (movable(finder, x0, z, ignore) == 0) {
					founded = 1;
					break;
				} else {
					rx = x0;
					rz = z;
				}
			}
		}
	} else {
		if (abs(slope) < 1) {
			if (fx1 >= fx0) {
				float inc = 1;
				float x = fx0;
				for (; x <= fx1; x += inc) {
					float z = slope * (x - fx0) + fz0;
					if (dump != NULL)
						dump(ud, x, z);

					if (movable(finder, x, z, ignore) == 0) {
						founded = 1;
						break;
					} else {
						rx = x;
						rz = z;
					}
				}
			} else {
				float inc = -1;
				float x = fx0;
				founded = 0;
				for (; x >= fx1; x += inc) {
					float z = slope * (x - fx0) + fz0;
					if (dump != NULL)
						dump(ud, x, z);
					if (movable(finder, x, z, ignore) == 0) {
						founded = 1;
						break;
					} else {
						rx = x;
						rz = z;
					}
				}
			}
		} else {
			if (fz1 >= fz0) {
				float inc = 1;
				float z = fz0;
				founded = 0;
				for (; z <= fz1; z += inc) {
					float x = (z - fz0) / slope + fx0;
					if (dump != NULL)
						dump(ud, x, z);
					if (movable(finder, x, z, ignore) == 0) {
						founded = 1;
						break;
					} else {
						rx = x;
						rz = z;
					}
				}
			} else {
				float inc = -1;
				float z = fz0;
				founded = 0;
				for (; z >= fz1; z += inc) {
					float x = (z - fz0) / slope + fx0;
					if (dump != NULL)
						dump(ud, x, z);
					if (movable(finder, x, z, ignore) == 0) {
						founded = 1;
						break;
					} else {
						rx = x;
						rz = z;
					}
				}
			}
		}
	}

	if (founded == 0 && movable(finder, (int)fx1, (int)fz1, ignore) == 1) {
		rx = (float)x1;
		rz = (float)z1;
	}

	*resultx = (int)rx;
	*resultz = (int)rz;
}

int 
finder_movable(pathfinder_t * finder, int x, int z, int ignore) {
	return movable(finder, x, z, ignore);
}

void 
finder_mask_set(pathfinder_t * finder, int index, int enable) {
	if (index < 0 || index >= MARK_MAX) {
		return;
	}
	finder->mask[index] = enable;
}

void 
finder_mask_reset(pathfinder_t * finder) {
	int i = 0;
	for (; i < MARK_MAX; i++)
		finder->mask[i] = 0;
}
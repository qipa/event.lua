#ifndef PATHFINDER_H
#define PATHFINDER_H

struct pathfinder;

typedef void(*finder_cb)(void *ud, int x, int z);
typedef void(*finder_dump)(void* ud, int x, int z);

struct pathfinder* finder_create(int width, int heigh, char* data);
void finder_release(struct pathfinder* finder);
void finder_raycast(struct pathfinder* finder, int x0, int z0, int x1, int z1, int ignore, int* resultx, int* resultz, finder_dump dump, void* ud);
int finder_find(struct pathfinder * finder, int x0, int z0, int x1, int z1, finder_cb cb, void* cb_args, finder_dump dump, void* dump_args,float cost);
int finder_movable(struct pathfinder * finder, int x, int z, int ignore);
void finder_mask_set(struct pathfinder * finder, int mask_index, int enable);
void finder_mask_reset(struct pathfinder * finder);


#endif
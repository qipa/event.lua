#include "nav.h"

inline double
cross(struct vector3* vt1,struct vector3* vt2) {
	return vt1->z * vt2->x - vt1->x * vt2->z;
}

inline void 
cross_point(struct vector3* a,struct vector3* b,struct vector3* c,struct vector3* d,struct vector3* result) {
	result->x = ((b->x - a->x) * (c->x - d->x) * (c->z - a->z) - c->x * (b->x - a->x) * (c->z - d->z) + a->x * (b->z - a->z) * (c->x - d->x))/((b->z - a->z)*(c->x - d->x) - (b->x - a->x) * (c->z - d->z));
	result->z = ((b->z - a->z) * (c->z - d->z) * (c->x - a->x) - c->z * (b->z - a->z) * (c->x - d->x) + a->z * (b->x - a->x) * (c->z - d->z))/((b->x - a->x)*(c->z - d->z) - (b->z - a->z) * (c->x - d->x));
}

inline void 
vector3_copy(struct vector3* dst,struct vector3* src) {
	dst->x = src->x;
	dst->y = src->y;
	dst->z = src->z;
}

inline void 
vector3_sub(struct vector3* a,struct vector3* b,struct vector3* result) {
	result->x = a->x - b->x;
	result->y = a->y - b->y;
	result->z = a->z - b->z;
}

void 
set_mask(struct nav_mesh_mask* ctx,int mask,int enable) {
	if (mask >= ctx->size) {
		ctx->size *= 2;
		ctx->mask = realloc(ctx->mask,sizeof(int) * ctx->size);
	}
	ctx->mask[mask] = enable;
}

bool 
inside_poly(struct nav_mesh_context* mesh_ctx, int* poly, int size, struct vector3* vt3) {
	int forward = 0;
	int i;
	for (i = 0;i < size;i++) {
		struct vector3* vt1 = &mesh_ctx->vertices[poly[i]];
		struct vector3* vt2 = &mesh_ctx->vertices[poly[(i+1)%size]];

		struct vector3 vt21;
		vt21.x = vt2->x - vt1->x;
		vt21.y = 0;
		vt21.z = vt2->z - vt1->z;

		struct vector3 vt31;
		vt31.x = vt3->x - vt1->x;
		vt31.y = 0;
		vt31.z = vt3->z - vt1->z;

		double y = cross(&vt21,&vt31);
		if (y == 0)
			continue;

		if (forward == 0)
			forward = y > 0? 1:-1;
		else {
			if (forward == 1 && y < 0)
				return false;
			else if (forward == -1 && y > 0)
				return false;
		}
	}
	return true;
}

inline bool 
inside_node(struct nav_mesh_context* mesh_ctx,int polyId,double x,double y,double z) {
	struct nav_node* nav_node = &mesh_ctx->node[polyId];
	struct vector3 vt;
	vt.x = x;
	vt.y = y;
	vt.z = z;
	return inside_poly(mesh_ctx,nav_node->poly,nav_node->size,&vt);
}

struct nav_node* 
get_node_with_pos(struct nav_mesh_context* ctx,double x,double y,double z) {
	if (x < ctx->lt.x || x > ctx->br.x)
		return NULL;
	if (z < ctx->lt.z || z > ctx->br.z)
		return NULL;

	if (ctx->tile == NULL) {
		int i;
		for (i = 0; i < ctx->size;i++) {
			if (inside_node(ctx,i,x,y,z))
				return &ctx->node[i];
		}
		return NULL;
	}

	int x_index = x - ctx->lt.x;
	int z_index = z - ctx->lt.z;
	int index = x_index + z_index * ctx->width;
	struct nav_tile* tile = &ctx->tile[index];
	int i;
	for (i = 0;i < tile->offset;i++) {
		if (inside_node(ctx, tile->node[i], x, y, z))
			return &ctx->node[tile->node[i]];
	}
	return NULL;
}

struct list* 
get_link(struct nav_mesh_context* mesh_ctx, struct nav_node* node) {
	int i;
	for (i = 0; i < node->size;i++) {
		int border_index = node->border[i];
		struct nav_border* border = get_border(mesh_ctx, border_index);

		int linked = -1;
		if (border->node[0] == node->id)
			linked = border->node[1];
		else
			linked = border->node[0];

		if (linked == -1)
			continue;
		
		struct nav_node* tmp = get_node(mesh_ctx,linked);
		if (tmp->list_head.pre || tmp->list_head.next)
			continue;

		if (get_mask(mesh_ctx->mask_ctx,tmp->mask)) {
			list_push((&mesh_ctx->linked),((struct list_node*)tmp));
			tmp->reserve = border->opposite;
			vector3_copy(&tmp->pos, &border->center);
		}
	}

	if (list_empty((&mesh_ctx->linked)))
		return NULL;
	
	return &mesh_ctx->linked;
}

static inline double 
G_COST(struct nav_node* from,struct nav_node* to) {
	double dx = from->pos.x - to->pos.x;
	//double dy = from->center.y - to->center.y;
	double dy = 0;
	double dz = from->pos.z - to->pos.z;
	return sqrt(dx*dx + dy* dy + dz* dz) * GRATE;
}

static inline double 
H_COST(struct nav_node* from, struct vector3* to) {
	double dx = from->center.x - to->x;
	//double dy = from->center.y - to->y;
	double dy = 0;
	double dz = from->center.z - to->z;
	return sqrt(dx*dx + dy* dy + dz* dz) * HRATE;
}

//FIXME:共线问题
bool 
raycast(struct nav_mesh_context* ctx, struct vector3* pt0, struct vector3* pt1, struct vector3* result,search_dumper dumper, void* userdata) {
	struct nav_node* curr_node = get_node_with_pos(ctx,pt0->x,pt0->y,pt0->z);

	struct vector3 vt10;
	vector3_sub(pt1,pt0,&vt10);

	while (curr_node) {
		if (inside_node(ctx, curr_node->id, pt1->x, pt1->y, pt1->z)) {
			vector3_copy(result,pt1);
			return true;
		}

		bool crossed = false;
		int i;
		for (i = 0; i < curr_node->size; i++) {
			struct nav_border* border = get_border(ctx, curr_node->border[i]);

			struct vector3* pt3 = &ctx->vertices[border->a];
			struct vector3* pt4 = &ctx->vertices[border->b];

			struct vector3 vt30,vt40;
			vector3_sub(pt3,pt0,&vt30);
			vector3_sub(pt4,pt0,&vt40);

			double direct_a = cross(&vt30,&vt10);
			double direct_b = cross(&vt40,&vt10);

			if ((direct_a < 0 && direct_b > 0) || (direct_a == 0 && direct_b > 0) || (direct_a < 0 && direct_b == 0)) {
				int next = -1;
				if (border->node[0] !=-1) {
					if (border->node[0] == curr_node->id)
						next = border->node[1];
					else
						next = border->node[0];
				}
				else
					assert(border->node[1] == curr_node->id);
				
				if (next == -1) {
					cross_point(pt3,pt4,pt1,pt0,result);
					return true;
				}
				else {
					struct nav_node* next_node = get_node(ctx,next);
					if (get_mask(ctx->mask_ctx, next_node->mask) == 0) {
						cross_point(pt3,pt4,pt1,pt0,result);
						return true;
					}
					if (dumper)
						dumper(userdata, next);

					crossed = true;
					curr_node = next_node;
					break;
				}
			}
		}
		if (!crossed)
			assert(0);
	}
	return false;
}


static inline void 
clear_node(struct nav_node* n) {
	n->link_parent = NULL; 
	n->link_border = -1; 
	n->F = n->G = n->H = 0; 
	n->elt.index = 0;
}

static inline void 
heap_clear(struct element* elt) {
	struct nav_node *node = cast_node(elt);
	clear_node(node);
}

static inline void 
reset(struct nav_mesh_context* ctx) {
	struct nav_node * node = NULL;
	while ((node = (struct nav_node*)list_pop(&ctx->closelist))) {
		clear_node(node);
	}
	minheap_clear((ctx)->openlist, heap_clear);
}

struct nav_node* 
next_border(struct nav_mesh_context* ctx,struct nav_node* node,struct vector3* wp,int *link_border) {
	struct vector3 vt0,vt1;
	*link_border = node->link_border;
	while (*link_border != -1) {
		struct nav_border* border = get_border(ctx,*link_border);
		vector3_sub(&ctx->vertices[border->a],wp,&vt0);
		vector3_sub(&ctx->vertices[border->b],wp,&vt1);
		if ((vt0.x == 0 && vt0.z == 0) || (vt1.x == 0 && vt1.z == 0)) {
			node = node->link_parent;
			*link_border = node->link_border;
		}
		else
			break;
	}
	if (*link_border != -1)
		return node;
	
	return NULL;
}

static inline void 
path_init(struct nav_mesh_context* mesh_ctx) {
	mesh_ctx->result.offset = 0;
}

void 
path_add(struct nav_mesh_context* mesh_ctx,struct vector3* wp) {
	if (mesh_ctx->result.offset >= mesh_ctx->result.size) {
		mesh_ctx->result.size *= 2;
		mesh_ctx->result.wp = (struct vector3*)realloc(mesh_ctx->result.wp,sizeof(struct vector3)*mesh_ctx->result.size);
	}

	mesh_ctx->result.wp[mesh_ctx->result.offset].x = wp->x;
	mesh_ctx->result.wp[mesh_ctx->result.offset].z = wp->z;
	mesh_ctx->result.offset++;
}

void 
make_waypoint(struct nav_mesh_context* mesh_ctx,struct vector3* pt0,struct vector3* pt1,struct nav_node * node) {
	path_add(mesh_ctx, pt1);

	struct vector3* pt_wp = pt1;

	int link_border = node->link_border;

	struct nav_border* border = get_border(mesh_ctx,link_border);

	struct vector3 pt_left,pt_right;
	vector3_copy(&pt_left,&mesh_ctx->vertices[border->a]);
	vector3_copy(&pt_right,&mesh_ctx->vertices[border->b]);

	struct vector3 vt_left,vt_right;
	vector3_sub(&pt_left,pt_wp,&vt_left);
	vector3_sub(&pt_right,pt_wp,&vt_right);

	struct nav_node* left_node = node->link_parent;
	struct nav_node* right_node = node->link_parent;

	struct nav_node* tmp = node->link_parent;
	while (tmp)
	{
		int link_border = tmp->link_border;
		if (link_border == -1)
		{
			struct vector3 tmp_target;
			tmp_target.x = pt0->x - pt_wp->x;
			tmp_target.z = pt0->z - pt_wp->z;

			double forward_a = cross(&vt_left,&tmp_target);
			double forward_b = cross(&vt_right,&tmp_target);

			if (forward_a < 0 && forward_b > 0)
			{
				path_add(mesh_ctx, pt0);
				break;
			}
			else
			{
				if (forward_a > 0 && forward_b > 0)
				{
					pt_wp->x = pt_left.x;
					pt_wp->z = pt_left.z;

					path_add(mesh_ctx, pt_wp);

					left_node = next_border(mesh_ctx,left_node,pt_wp,&link_border);
					if (left_node == NULL)
					{
						path_add(mesh_ctx, pt0);
						break;
					}
					
					border = get_border(mesh_ctx,link_border);
					pt_left.x = mesh_ctx->vertices[border->a].x;
					pt_left.z = mesh_ctx->vertices[border->a].z;

					pt_right.x = mesh_ctx->vertices[border->b].x;
					pt_right.z = mesh_ctx->vertices[border->b].z;

					vt_left.x = pt_left.x - pt_wp->x;
					vt_left.z = pt_left.z - pt_wp->z;

					vt_right.x = pt_right.x - pt_wp->x;
					vt_right.z = pt_right.z - pt_wp->z;

					tmp = left_node->link_parent;
					left_node = tmp;
					right_node = tmp;
					continue;
				}
				else if (forward_a < 0 && forward_b < 0)
				{
					pt_wp->x = pt_right.x;
					pt_wp->z = pt_right.z;

					path_add(mesh_ctx, pt_wp);

					right_node = next_border(mesh_ctx,right_node,pt_wp,&link_border);
					if (right_node == NULL)
					{
						path_add(mesh_ctx, pt0);
						break;
					}
					
					border = get_border(mesh_ctx,link_border);
					pt_left.x = mesh_ctx->vertices[border->a].x;
					pt_left.z = mesh_ctx->vertices[border->a].z;

					pt_right.x = mesh_ctx->vertices[border->b].x;
					pt_right.z = mesh_ctx->vertices[border->b].z;

					vt_left.x = pt_left.x - pt_wp->x;
					vt_left.z = pt_left.z - pt_wp->z;

					vt_right.x = pt_right.x - pt_wp->x;
					vt_right.z = pt_right.z - pt_wp->z;

					tmp = right_node->link_parent;
					left_node = tmp;
					right_node = tmp;
					continue;
				}
				break;
			}

		}

		border = get_border(mesh_ctx,link_border);

		struct vector3 tmp_pt_left,tmp_pt_right;
		vector3_copy(&tmp_pt_left,&mesh_ctx->vertices[border->a]);
		vector3_copy(&tmp_pt_right,&mesh_ctx->vertices[border->b]);

		struct vector3 tmp_vt_left,tmp_vt_right;
		vector3_sub(&tmp_pt_left,pt_wp,&tmp_vt_left);
		vector3_sub(&tmp_pt_right,pt_wp,&tmp_vt_right);

		double forward_left_a = cross(&vt_left,&tmp_vt_left);
		double forward_left_b = cross(&vt_right,&tmp_vt_left);
		double forward_right_a = cross(&vt_left,&tmp_vt_right);
		double forward_right_b = cross(&vt_right,&tmp_vt_right);

		if (forward_left_a < 0 && forward_left_b > 0)
		{
			left_node = tmp->link_parent;
			vector3_copy(&pt_left,&tmp_pt_left);
			vector3_sub(&pt_left,pt_wp,&vt_left);
		}

		if (forward_right_a < 0 && forward_right_b > 0)
		{
			right_node = tmp->link_parent;
			vector3_copy(&pt_right,&tmp_pt_right);
			vector3_sub(&pt_right,pt_wp,&vt_right);
		}

		if (forward_left_a > 0 && forward_left_b > 0 && forward_right_a > 0 && forward_right_b > 0)
		{
			vector3_copy(pt_wp,&pt_left);

			left_node = next_border(mesh_ctx,left_node,pt_wp,&link_border);
			if (left_node == NULL)
			{
				path_add(mesh_ctx, pt0);
				break;
			}
			
			border = get_border(mesh_ctx,link_border);
			vector3_copy(&pt_left,&mesh_ctx->vertices[border->a]);
			vector3_copy(&pt_right,&mesh_ctx->vertices[border->b]);

			vector3_sub(&mesh_ctx->vertices[border->a],pt_wp,&vt_left);
			vector3_sub(&mesh_ctx->vertices[border->b],pt_wp,&vt_right);

			path_add(mesh_ctx, pt_wp);

			tmp = left_node->link_parent;
			left_node = tmp;
			right_node = tmp;

			continue;
		}

		if (forward_left_a < 0 && forward_left_b < 0 && forward_right_a < 0 && forward_right_b < 0)
		{
			vector3_copy(pt_wp,&pt_right);

			right_node = next_border(mesh_ctx,right_node,pt_wp,&link_border);
			if (right_node == NULL)
			{
				path_add(mesh_ctx, pt0);
				break;
			}
			
			border = get_border(mesh_ctx,link_border);
			vector3_copy(&pt_left,&mesh_ctx->vertices[border->a]);
			vector3_copy(&pt_right,&mesh_ctx->vertices[border->b]);

			vector3_sub(&mesh_ctx->vertices[border->a],pt_wp,&vt_left);
			vector3_sub(&mesh_ctx->vertices[border->b],pt_wp,&vt_right);

			path_add(mesh_ctx, pt_wp);

			tmp = right_node->link_parent;
			left_node = tmp;
			right_node = tmp;
			continue;
		}

		tmp = tmp->link_parent;
	}
}

struct nav_path* 
astar_find(struct nav_mesh_context* mesh_ctx, struct vector3* pt_start, struct vector3* pt_over, search_dumper dumper, void* userdata) {
	path_init(mesh_ctx);

	struct nav_node* node_start = get_node_with_pos(mesh_ctx, pt_start->x, pt_start->y, pt_start->z);
	struct nav_node* node_over = get_node_with_pos(mesh_ctx, pt_over->x, pt_over->y, pt_over->z);

	if (!node_start || !node_over)
		return NULL;

	if (node_start == node_over) {
		path_add(mesh_ctx, pt_start);
		path_add(mesh_ctx, pt_over);
		return &mesh_ctx->result;
	}

	vector3_copy(&node_start->pos, pt_start);
	
	minheap_push(mesh_ctx->openlist, &node_start->elt);

	struct nav_node* node_current = NULL;
	for (;;) {
		struct element* elt = minheap_pop(mesh_ctx->openlist);
		if (!elt) {
			reset(mesh_ctx);
			return NULL;
		}
		node_current = cast_node(elt);
		
		if (node_current == node_over) {
			make_waypoint(mesh_ctx, pt_start, pt_over, node_current);
			reset(mesh_ctx);
			clear_node(node_current);
			return &mesh_ctx->result;
		}

		list_push(&mesh_ctx->closelist, (struct list_node*)node_current);

		struct list* linked = get_link(mesh_ctx, node_current);
		if (linked) {
			struct nav_node* linked_node;
			while ((linked_node = (struct nav_node*)list_pop(linked))) {
				if (linked_node->elt.index) {
					double nG = node_current->G + G_COST(node_current, linked_node);
					if (nG < linked_node->G) {
						linked_node->G = nG;
						linked_node->F = linked_node->G + linked_node->H;
						linked_node->link_parent = node_current;
						linked_node->link_border = linked_node->reserve;
						minheap_change(mesh_ctx->openlist, &linked_node->elt);
					}
				} else {
					linked_node->G = node_current->G + G_COST(node_current, linked_node);
					linked_node->H = H_COST(linked_node,pt_over);
					linked_node->F = linked_node->G + linked_node->H;
					linked_node->link_parent = node_current;
					linked_node->link_border = linked_node->reserve;
					minheap_push(mesh_ctx->openlist, &linked_node->elt);
					if (dumper != NULL)
						dumper(userdata, linked_node->id);
				}
			}
		}
	}
}

struct vector3* 
around_movable(struct nav_mesh_context* ctx,double x,double z, int range,search_dumper dumper,void* userdata) {
	if (ctx->tile == NULL)
		return NULL;

	if (x < ctx->lt.x || x > ctx->br.x)
		return NULL;
	if (z < ctx->lt.z || z > ctx->br.z)
		return NULL;

	int x_index = x - ctx->lt.x;
	int z_index = z - ctx->lt.z;

	int r;
	for (r = 1; r <= range; ++r) {
		int x_min = x_index - r;
		int x_max = x_index + r;
		int z_min = z_index - r;
		int z_max = z_index + r;

		int x,z;

		int z_range[2] = { z_min, z_max };

		int j;
		for (j = 0; j < 2;j++) {
			z = z_range[j];
			if (z < 0 || z >= ctx->heigh)
				continue;
			
			int x_begin = x_min < 0 ? 0 : x_min;
			int x_end = x_max >= ctx->width ? ctx->width-1 : x_max;
			for (x = x_begin; x <= x_end; x++) {
				int index = x + z * ctx->width;
				struct nav_tile* tile = &ctx->tile[index];
				if (dumper)
					dumper(userdata, index);
				int i;
				for (i = 0; i < tile->offset; i++) {
					if (inside_node(ctx, tile->node[i], tile->center.x, tile->center.y, tile->center.z))
						return &tile->center;
				}
			}
		}

		int x_range[2] = { x_min, x_max };

		for (j = 0; j < 2;j++) {
			x = x_range[j];
			if (x < 0 || x >= ctx->width)
				continue;

			int z_begin = z_min + 1 < 0 ? 0 : z_min + 1;
			int z_end = z_max >= ctx->heigh ? ctx->heigh-1 : z_max;
			for (z = z_begin; z < z_end; z++) {
				int index = x + z * ctx->width;
				struct nav_tile* tile = &ctx->tile[index];
				if (dumper)
					dumper(userdata, index);
				int i;
				for (i = 0; i < tile->offset; i++) {
					if (inside_node(ctx, tile->node[i], tile->center.x, tile->center.y, tile->center.z))
						return &tile->center;
				}
			}
		}
	}

	return NULL;
}

bool 
point_movable(struct nav_mesh_context* ctx, double x, double z) {
	struct nav_node* node = get_node_with_pos(ctx, x, 0, z);
	if (node) {
		if (get_mask(ctx->mask_ctx, node->mask) == 1) {
			return true;
		}
	}
	return false;
}
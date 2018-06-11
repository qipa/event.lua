

#define AOI_ENTITY 		1
#define AOI_LOW_BOUND 	2
#define AOI_HIGH_BOUND 	4

typedef struct position {
	int x;
	int z;
} position_t;


typedef struct linklist {
	linknode_t* head;
	linknode_t* tail;
} linklist_t;

struct aoi_entity;
struct aoi_trigger;

typedef struct aoi_object {
	int uid;
	struct aoi_entity* entity;
	struct aoi_trigger* trigger;

	struct aoi_object* enter_head;
	struct aoi_object* enter_tail;

	struct aoi_object* leave_head;
	struct aoi_object* leave_tail;

	struct aoi_object* next;
	struct aoi_object* prev;
} aoi_object_t;

typedef struct linknode {
	aoi_object_t* owner;
	struct linknode* prev;
	struct linknode* next;
	position_t pos;
	uint8_t flag;
} linknode_t;

typedef struct aoi_entity {
	position_t center;
	linknode_t node[2];
} aoi_entity_t;

typedef struct aoi_trigger {
	position_t center;
	linknode_t node[4];
	int range;
} aoi_trigger_t;

typedef struct aoi_context {
	linklist_t linklist[2];
} aoi_context_t;

void
insert_node(aoi_context_t* aoi_ctx,int xorz,linknode_t* linknode) {
	linknode_t* first
	if (xorz == 0) {
		first = &aoi_ctx->linklist[0];
	} else {
		first = &aoi_ctx->linklist[1];
	}
	if (first->head == NULL) {
		assert(first->head == first->tail);
		first->head = first->tail = linknode;
	} else {
		linknode->next = first->head->next;
		first->head->next->prev = linknode;
		first->head = linknode;
	}
}

static inline void
exchange_x(linknode_t* node,linknode_t* node_next,int dir) {
	node->next = node_next->next;
	if (node_next->next) {
		node_next->next->prev = node;
	}

	linknode_t* tmp_prev = node->prev;
	node_next->prev = tmp_prev;
	if (tmp_prev) {
		tmp_prev->next = node_next;
	}
	node_next->next = node;
	node->prev = node_next;

	if (node->flag & AOI_ENTITY && node_next->flag & AOI_LOW_BOUND) {

	} else if (node->flag & AOI_ENTITY && node_next->flag & AOI_HIGH_BOUND) {

	} else if (node->flag & AOI_LOW_BOUND && node_next->flag & AOI_ENTITY) {

	} else if (node->flag & AOI_HIGH_BOUND && node_next->flag & AOI_ENTITY) {

	}
}

static inline void
exchange_z(linknode_t* node,linknode_t* node_next,int dir) {

}

int
shuffle_x(aoi_context_t* aoi_ctx,linknode_t* node,int x,int dir) {
	linknode_t* first = &aoi_ctx->linklist[0];
	node->pos.x = x;
	while(node->next != NULL && node->pos.x > node->next->pos.x) {
		linknode_t* node_next = node->next;
		exchange_x(node,node_next);
	}

	while(node->prev != NULL && node->pos.x < node->prev->pos.x) {
		exchange_x(node->prev,node);
	}
}

int
shuffle_z(aoi_context_t* aoi_ctx,linknode_t* node,int z,int dir) {
	linknode_t* first = &aoi_ctx->linklist[1];
	node->pos.z = z;
	while(node->next != NULL && node->pos.z > node->next->pos.z) {
		linknode_t* node_next = node->next;
		exchange_z(node,node_next);
	}

	while(node->prev != NULL && node->pos.z < node->prev->pos.z) {
		exchange_z(node->prev,node);
	}
}

void
shuffle_entity(aoi_context_t* aoi_ctx,aoi_entity_t* entity,int x,int z) {
	int dir;
	if (entity->node[0].pos.x < x)
		dir = -1;
	else
		dir = 1;
	shuffle_x(aoi_ctx,&entity->node[0],x,dir);

	if (entity->node[0].pos.z < z)
		dir = -1;
	else
		dir = 1;
	shuffle_z(aoi_ctx,&entity->node[1],z,dir);
}

void
shuffle_trigger(aoi_context_t* aoi_ctx,aoi_trigger_t* trigger,int x,int z) {
	int dir;
	if (trigger->node[0].pos.x < x)
		dir = -1;
	else
		dir = 1;
	shuffle_x(aoi_ctx,&trigger->node[0],x,dir);

	if (trigger->node[2].pos.x < x)
		dir = -1;
	else
		dir = 1;
	shuffle_x(aoi_ctx,&trigger->node[2],x,dir);

	if (trigger->node[1].pos.z < z)
		dir = -1;
	else
		dir = 1;

	shuffle_z(aoi_ctx,&trigger->node[1],z,dir);

	if (trigger->node[3].pos.z < z)
		dir = -1;
	else
		dir = 1;

	shuffle_z(aoi_ctx,&trigger->node[3],z,dir);
}

void
create_entity(aoi_context_t* aoi_ctx,aoi_object_t* aoi_object,int x,int z) {
	aoi_object->entity = malloc(sizeof(aoi_entity_t));
	memset(aoi_object->entity,0,sizeof(aoi_entity_t));
	
	aoi_object->entity->center.x = x;
	aoi_object->entity->center.z = z;
	
	aoi_object->entity->owner = aoi_object;
	
	aoi_object->entity->node[0].flag |= AOI_ENTITY;
	aoi_object->entity->node[1].flag |= AOI_ENTITY;

	aoi_object->entity->node[0].pos.x = -1000;
	aoi_object->entity->node[0].pos.z = -1000;

	aoi_object->entity->node[1].pos.x = -1000;
	aoi_object->entity->node[1].pos.z = -1000;

	insert_node(aoi_ctx,0,&aoi_object->entity->node[0]);
	insert_node(aoi_ctx,1,&aoi_object->entity->node[1]);

	shuffle_entity(aoi_ctx,aoi_object->entity,x,z);
}

void
create_trigger(aoi_context_t* aoi_ctx,aoi_object_t* aoi_object,int x,int z,int range) {
	aoi_object->trigger = malloc(sizeof(aoi_trigger_t));
	memset(aoi_object->trigger,0,sizeof(aoi_trigger_t));
	
	aoi_object->trigger->range = range;
	
	aoi_object->trigger->center.x = x;
	aoi_object->trigger->center.z = z;
	
	aoi_object->trigger->owner = aoi_object;

	aoi_object->trigger->node[0].flag |= AOI_LOW_BOUND;
	aoi_object->trigger->node[1].flag |= AOI_LOW_BOUND;

	aoi_object->trigger->node[0].pos.x = -1000;
	aoi_object->trigger->node[0].pos.z = -1000;

	aoi_object->trigger->node[1].pos.x = -1000;
	aoi_object->trigger->node[1].pos.z = -1000;

	aoi_object->trigger->node[2].flag |= AOI_HIGH_BOUND;
	aoi_object->trigger->node[3].flag |= AOI_HIGH_BOUND;

	aoi_object->trigger->node[2].pos.x = -1000;
	aoi_object->trigger->node[2].pos.z = -1000;

	aoi_object->trigger->node[3].pos.x = -1000;
	aoi_object->trigger->node[3].pos.z = -1000;

	insert_node(aoi_ctx,0,&aoi_object->trigger->node[0]);
	insert_node(aoi_ctx,0,&aoi_object->trigger->node[2]);

	insert_node(aoi_ctx,1,&aoi_object->trigger->node[1]);
	insert_node(aoi_ctx,1,&aoi_object->trigger->node[3]);

	shuffle_trigger(aoi_ctx,aoi_object->trigger,x,z);

}

void
delete_entity(aoi_context_t* aoi_ctx,aoi_object_t* aoi_object) {

}

void
delete_trigger(aoi_context_t* aoi_ctx,aoi_object_t* aoi_object) {

}

void
move_entity(aoi_context_t* aoi_ctx,aoi_object_t* aoi_object,int x,int z) {
	shuffle_entity(aoi_ctx,aoi_object->entity,x,z);
}

void
move_trigger(aoi_context_t* aoi_ctx,aoi_object_t* aoi_object,int x,int z) {
	shuffle_trigger(aoi_ctx,aoi_object->trigger,x,z);
}
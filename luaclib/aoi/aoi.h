

#define AOI_ENTITY 		1
#define AOI_LOW_BOUND 	2
#define AOI_HIGH_BOUND 	4

typedef struct position {

} position_t;


typedef struct linknode {
	struct linknode* prev;
	struct linknode* next;
	position_t pos;
	uint8_t flag;
	aoi_context_t* owner;
} linknode_t;

typedef struct linklist {
	linknode_t* head;
	linknode_t* tail;
} linklist_t;


struct aoi_entity;
struct aoi_trigger;

typedef struct aoi_object {
	struct aoi_entity* entity;
	struct aoi_trigger* trigger;
} aoi_object_t;

typedef struct aoi_entity {
	position_t center;
	linknode_t node[2];
	aoi_object_t* owner;
} aoi_entity_t;

typedef struct aoi_trigger {
	position_t center;
	linknode_t node[4];
	int range;
	aoi_object_t* owner;
} aoi_trigger_t;

typedef struct aoi_context {
	linklist_t linklist[2];
} aoi_context_t;

void
insert_node(aoi_context_t* aoi_ctx,int xorz,linknode_t* linknode) {
	if (xorz == 0) {
		linknode_t* first = &aoi_ctx->linklist[0];
		if (first->head == NULL) {
			assert(first->head == first->tail);
			first->head = first->tail = linknode;
		} else {
			
		}
	} else {

	}
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

	aoi_object->entity->node[0].pos.x = x;
	aoi_object->entity->node[0].pos.z = z;

	aoi_object->entity->node[1].pos.x = x;
	aoi_object->entity->node[1].pos.z = z;

	insert_node(aoi_ctx,0,&aoi_object->entity->node[0]);
	insert_node(aoi_ctx,1,&aoi_object->entity->node[1]);
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

	aoi_object->trigger->node[0].pos.x = x - range;
	aoi_object->trigger->node[0].pos.z = z;

	aoi_object->trigger->node[1].pos.x = x;
	aoi_object->trigger->node[1].pos.z = z - range;

	aoi_object->trigger->node[2].flag |= AOI_HIGH_BOUND;
	aoi_object->trigger->node[3].flag |= AOI_HIGH_BOUND;

	aoi_object->trigger->node[2].pos.x = x + range;
	aoi_object->trigger->node[2].pos.z = z;

	aoi_object->trigger->node[3].pos.x = x;
	aoi_object->trigger->node[3].pos.z = z + range;

	insert_node(aoi_ctx,0,&aoi_object->trigger->node[0]);
	insert_node(aoi_ctx,0,&aoi_object->trigger->node[2]);

	insert_node(aoi_ctx,1,&aoi_object->trigger->node[1]);
	insert_node(aoi_ctx,1,&aoi_object->trigger->node[3]);
}

void
delete_entity(aoi_context_t* aoi_ctx,aoi_object_t* aoi_object) {

}

void
delete_trigger(aoi_context_t* aoi_ctx,aoi_object_t* aoi_object) {

}

void
move_entity(aoi_context_t* aoi_ctx,aoi_object_t* aoi_object,int x,int z) {

}

void
move_trigger(aoi_context_t* aoi_ctx,aoi_object_t* aoi_object,int x,int z) {
	
}
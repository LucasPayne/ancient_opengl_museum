#ifndef ENTITIES_H 
#define ENTITIES_H 
#include "mathematics.h"

struct Entity_s;
struct Behaviour_s;
typedef void (*EntityUpdate)(struct Entity_s *, struct Behaviour_s *);

// x,y are in coordinates (0,0) at the bottom-left, (1,1) at the top-right.
typedef void (*MouseListener)(struct Entity_s *, struct Behaviour_s *, int, int, float, float); // entity, behaviour, button, state, x, y
typedef void (*MouseMotionListener)(struct Entity_s *, struct Behaviour_s *, float, float); // entity, behaviour, x, y
typedef void (*KeyListener)(struct Entity_s *, struct Behaviour_s *, unsigned char); // entity, behaviour, key

typedef struct Behaviour_s {
    int type; // identifier for the type of this behaviour (each type has the same functions as well as the same struct of data).
    struct Entity_s *entity; // Pointer to the entity it is attached to.
    bool active;
    EntityUpdate update;
    MouseListener mouse_listener;
    MouseMotionListener mouse_motion_listener;
    KeyListener key_listener;
    void *data; // Optional state that the behaviour can have.
} Behaviour;
// Iterating over behaviours of a certain type is just done by brute force.
// These two macros allow the syntax
// for_behaviour(Collider, b, e)
//     ... do something with the behaviour, b, and the entity it is attached to, e.
// end_for_behaviour()


typedef struct BehaviourList_s {
    int type;
    int size;
    int length;
    struct Behaviour_s *list;
} BehaviourList;

extern struct Entity_s *entity_list;
extern int entity_list_size;
extern int entity_list_length;
extern BehaviourList *behaviour_lists;


#define for_behaviour(BEHAVIOUR_TYPE,BEHAVIOUR_LVALUE,ENTITY_LVALUE)\
{\
    BehaviourList *list = &behaviour_lists[( BEHAVIOUR_TYPE ## ID )];\
    for (int ___i = 0; ___i < list->length; ___i++) {\
        BEHAVIOUR_TYPE *BEHAVIOUR_LVALUE = (BEHAVIOUR_TYPE *) list->list[___i].data;\
        Entity *ENTITY_LVALUE = list->list[___i].entity;\
        {
#define end_for_behaviour()\
        }\
    }\
}

#define MAX_NUM_ENTITY_BEHAVIOURS 20
typedef struct Entity_s {
    struct Entity_s *prev;
    struct Entity_s *next;
    vec3 position;
    bool euler_controlled;
    vec3 euler_angles;
    mat3x3 orientation; // Access this only with the entity_orientation function, as the entity may be euler-controlled, meaning this needs to be derived.
    vec3 center;
    float scale;

    Behaviour *behaviours[MAX_NUM_ENTITY_BEHAVIOURS];
    int num_behaviours;
} Entity;

mat4x4 entity_matrix(Entity *entity);
mat3x3 entity_orientation(Entity *entity);
void prepare_entity_matrix(Entity *entity);

Entity *add_entity(vec3 position, vec3 euler_angles);
Behaviour *add_behaviour(Entity *entity, EntityUpdate update, size_t data_size, int behaviour_type_id);
Behaviour *get_behaviour(Entity *e, int type);

void init_entity_system(void);

#endif // ENTITIES_H 

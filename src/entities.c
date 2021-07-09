#include "museum.h"

BehaviourList *behaviour_lists = NULL;
int entity_list_size;
int entity_list_length;
Entity *entity_list = NULL;

void init_entity_system(void)
{
    behaviour_lists = malloc(sizeof(BehaviourList) * NUM_BEHAVIOUR_TYPES);
    mem_check(behaviour_lists);
    for (int i = 0; i < NUM_BEHAVIOUR_TYPES; i++) {
        BehaviourList *list = &behaviour_lists[i];
        list->type = i;
        list->size = 128;
        list->length = 0;
        list->list = calloc(1, sizeof(Behaviour) * list->size);
        mem_check(list->list);
    }
    entity_list_size = 512;
    entity_list_length = 0;
    entity_list = calloc(1, sizeof(Entity) * entity_list_size);
    mem_check(entity_list);
}

// Derive the matrix which transforms points in model/entity-space to world-space.
mat4x4 entity_matrix(Entity *entity)
{
    // For convenience, entities can be set as "euler-controlled". If their orientation is based off of euler angles, this avoids numerical problems
    // by deriving the matrix from stored angles, rather than updating a 3x3 matrix based on time-stepped axis rotations.
    mat3x3 orientation = entity_orientation(entity);

    mat4x4 matrix = {0};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            matrix.vals[4*i + j] = orientation.vals[3*i + j];
        }
    }
    matrix.vals[4*3+0] = X(entity->position);
    matrix.vals[4*3+1] = Y(entity->position);
    matrix.vals[4*3+2] = Z(entity->position);
    matrix.vals[4*3+3] = 1;

    mat4x4 scale_matrix;
    fill_mat4x4_rmaj(scale_matrix, entity->scale,0,0,0,
                                   0,entity->scale,0,0,
                                   0,0,entity->scale,0,
                                   0,0,0,1);
    right_multiply_mat4x4(&matrix, &scale_matrix);
    mat4x4 off_center_matrix;
    float cx,cy,cz;
    cx = X(entity->center);
    cy = Y(entity->center);
    cz = Z(entity->center);
    fill_mat4x4_rmaj(off_center_matrix, 1,0,0,-cx,
                                        0,1,0,-cy,
                                        0,0,1,-cz,
                                        0,0,0,1);
    right_multiply_mat4x4(&matrix, &off_center_matrix);
    return matrix;
}

mat3x3 entity_orientation(Entity *entity)
{
    if (entity->euler_controlled) return euler_rotation_mat3x3(X(entity->euler_angles), Y(entity->euler_angles), Z(entity->euler_angles));
    return entity->orientation;
}

Entity *add_entity(vec3 position, vec3 euler_angles)
{
    if (entity_list_length == entity_list_size) {
        entity_list_size *= 2;
        entity_list = realloc(entity_list, entity_list_size);
        mem_check(entity_list);
    }
    Entity *e = &entity_list[entity_list_length ++];
    memset(e, 0, sizeof(Entity));
    e->position = position;
    e->orientation = euler_rotation_mat3x3(X(euler_angles), Y(euler_angles), Z(euler_angles));
    e->center = vec3_zero();
    e->scale = 1;
    e->euler_angles = euler_angles; // Set entity->euler_controlled = true for these to control the entity orientation.
    return e;
}

Behaviour *add_behaviour(Entity *entity, EntityUpdate update, size_t data_size, int type)
{
    if (entity->num_behaviours == MAX_NUM_ENTITY_BEHAVIOURS) {
        fprintf(stderr, "ERROR: The maximum number of entity behaviours is %d.\n", MAX_NUM_ENTITY_BEHAVIOURS);
        exit(EXIT_FAILURE);
    }
    BehaviourList *list = &behaviour_lists[type];
    if (list->length == list->size) {
        list->size *= 2;
        list->list = realloc(list->list, list->size);
        mem_check(list->list);
    }
    Behaviour *b = &list->list[list->length ++];
    b->entity = entity;
    b->update = update;
    b->active = true;
    b->type = type;
    if (data_size == 0) {
        b->data = NULL;
    } else {
        b->data = calloc(1, data_size);
        mem_check(b->data);
    }
    entity->behaviours[entity->num_behaviours ++] = b;
    return b;
}

void prepare_entity_matrix(Entity *entity)
{
    mat4x4 matrix = entity_matrix(entity);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view_matrix.vals);
    glMultMatrixf(matrix.vals);
}

Behaviour *get_behaviour(Entity *e, int type)
{
    for (int i = 0; i < MAX_NUM_ENTITY_BEHAVIOURS; i++) {
        if (e->behaviours[i] != NULL && e->behaviours[i]->type == type) return e->behaviours[i];
    }
    return NULL;
}

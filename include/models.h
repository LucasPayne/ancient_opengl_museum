#ifndef MODELS_H
#define MODELS_H

Model load_OFF_model(char *filename);
Model polyhedron_to_model(Polyhedron polyhedron);
Model make_surface_of_revolution(float *xs, float *ys, int num_points, int tessellation);
Model make_capsule(float radius, float height);
Model make_cylinder(float radius, float height);
Model make_half_capsule(float radius, float height);
Model make_tessellated_block(float w, float h, float d, int tw, int th, int td);
Model make_tessellated_block_with_uvs(float w, float h, float d, int tw, int th, int td, float texture_size);
Model make_icosahedron(float radius);
Model make_dodecahedron(float radius);
Model make_tetrahedron(float size);
Model make_octahedron(float size);

bool ray_model_intersection(vec3 origin, vec3 direction, Model *model, mat4x4 model_matrix, vec3 *intersection);

void compute_uvs_cylindrical(Model *model, float x_size, float y_size);
void compute_uvs_orthogonal(Model *model, vec3 right, vec3 up, float x_size, float y_size);

#endif // MODELS_H

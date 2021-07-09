#ifndef MATHEMATICS_H
#define MATHEMATICS_H
/*================================================================================
    Mathematics module with matrix and vector routines.
    All matrices are column-major.
    The coordinate system is right-handed.
================================================================================*/
#define X(VECTOR) (( VECTOR ).vals[0])
#define Y(VECTOR) (( VECTOR ).vals[1])
#define Z(VECTOR) (( VECTOR ).vals[2])
#define W(VECTOR) (( VECTOR ).vals[3])

// Unpack vectors into parameter lists.
#define UNPACK_VEC3(V) X(V),Y(V),Z(V)

// 2-vectors.
//================================================================================
typedef struct vec2_s {
    float vals[2];
} vec2;
vec2 new_vec2(float x, float y);

// 3-vectors.
//================================================================================
typedef struct vec3_s {
    float vals[3];
} vec3;
vec3 new_vec3(float x, float y, float z);
vec3 vec3_zero(void);
#define vec3_dot(A,B) ( X( A )*X( B ) + Y( A )*Y( B ) + Z( A )*Z( B ) )
vec3 vec3_add(vec3 a, vec3 b);
vec3 vec3_mul(vec3 a, float x);
vec3 vec3_sub(vec3 a, vec3 b);
vec3 vec3_neg(vec3 a);
vec3 vec3_cross(vec3 u, vec3 v);
float vec3_length(vec3 v);
vec3 vec3_normalize(vec3 v);
vec3 vec3_lerp(vec3 u, vec3 v, float t);
vec3 rand_vec3(float size);

// 4-vectors.
//================================================================================
typedef struct vec4_s {
    float vals[4];
} vec4;
vec4 new_vec4(float x, float y, float z, float w);
vec4 vec4_zero(void);
#define vec4_dot(A,B) ( X( A )*X( B ) + Y( A )*Y( B ) + Z( A )*Z( B ) + W( A )*W( B ))
vec4 vec4_add(vec4 a, vec4 b);
vec4 vec4_mul(vec4 a, float x);
vec4 vec4_sub(vec4 a, vec4 b);
vec4 vec4_neg(vec4 a);

// 3x3 matrices.
//================================================================================
typedef struct mat3x3_s {
    float vals[9];
} mat3x3;
#define fill_mat3x3_cmaj(MATRIX,A,B,C,D,E,F,G,H,I)\
{\
    ( MATRIX ).vals[0] = ( A );\
    ( MATRIX ).vals[1] = ( B );\
    ( MATRIX ).vals[2] = ( C );\
    ( MATRIX ).vals[3] = ( D );\
    ( MATRIX ).vals[4] = ( E );\
    ( MATRIX ).vals[5] = ( F );\
    ( MATRIX ).vals[6] = ( G );\
    ( MATRIX ).vals[7] = ( H );\
    ( MATRIX ).vals[8] = ( I );\
}
#define fill_mat3x3_rmaj(MATRIX,A,B,C,D,E,F,G,H,I)\
{\
    ( MATRIX ).vals[0] = ( A );\
    ( MATRIX ).vals[3] = ( B );\
    ( MATRIX ).vals[6] = ( C );\
    ( MATRIX ).vals[1] = ( D );\
    ( MATRIX ).vals[4] = ( E );\
    ( MATRIX ).vals[7] = ( F );\
    ( MATRIX ).vals[2] = ( G );\
    ( MATRIX ).vals[5] = ( H );\
    ( MATRIX ).vals[8] = ( I );\
}
mat3x3 identity_mat3x3(void);
mat3x3 mat3x3_transpose(mat3x3 m);
float mat3x3_determinant(mat3x3 m);
mat3x3 mat3x3_inverse(mat3x3 m);
void right_multiply_mat3x3(mat3x3 *matrix, mat3x3 *B);
mat3x3 mat3x3_multiply(mat3x3 A, mat3x3 B);
mat3x3 mat3x3_multiply3(mat3x3 A, mat3x3 B, mat3x3 C);
mat3x3 mat3x3_multiply4(mat3x3 A, mat3x3 B, mat3x3 C, mat3x3 D);
mat3x3 euler_rotation_mat3x3(float theta_x, float theta_y, float theta_z);
vec3 matrix_vec3(mat3x3 m, vec3 v);
mat3x3 mat3x3_add(mat3x3 A, mat3x3 B);
mat3x3 mat3x3_mul(mat3x3 m, float x);
void mat3x3_orthonormalize(mat3x3 *m);
mat3x3 axis_angle_rotate_mat3x3(mat3x3 matrix, vec3 axis, float theta);

// 4x4 matrices.
//================================================================================
typedef struct mat4x4_s {
    float vals[16];
} mat4x4;
#define fill_mat4x4_cmaj(MATRIX,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P)\
{\
    ( MATRIX ).vals[0] = ( A );\
    ( MATRIX ).vals[1] = ( B );\
    ( MATRIX ).vals[2] = ( C );\
    ( MATRIX ).vals[3] = ( D );\
    ( MATRIX ).vals[4] = ( E );\
    ( MATRIX ).vals[5] = ( F );\
    ( MATRIX ).vals[6] = ( G );\
    ( MATRIX ).vals[7] = ( H );\
    ( MATRIX ).vals[8] = ( I );\
    ( MATRIX ).vals[9] = ( J );\
    ( MATRIX ).vals[10] = ( K );\
    ( MATRIX ).vals[11] = ( L );\
    ( MATRIX ).vals[12] = ( M );\
    ( MATRIX ).vals[13] = ( N );\
    ( MATRIX ).vals[14] = ( O );\
    ( MATRIX ).vals[15] = ( P );\
}
#define fill_mat4x4_rmaj(MATRIX,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P)\
{\
    ( MATRIX ).vals[0] = ( A );\
    ( MATRIX ).vals[4] = ( B );\
    ( MATRIX ).vals[8] = ( C );\
    ( MATRIX ).vals[12] = ( D );\
    ( MATRIX ).vals[1] = ( E );\
    ( MATRIX ).vals[5] = ( F );\
    ( MATRIX ).vals[9] = ( G );\
    ( MATRIX ).vals[13] = ( H );\
    ( MATRIX ).vals[2] = ( I );\
    ( MATRIX ).vals[6] = ( J );\
    ( MATRIX ).vals[10] = ( K );\
    ( MATRIX ).vals[14] = ( L );\
    ( MATRIX ).vals[3] = ( M );\
    ( MATRIX ).vals[7] = ( N );\
    ( MATRIX ).vals[11] = ( O );\
    ( MATRIX ).vals[15] = ( P );\
}
mat4x4 identity_mat4x4(void);
mat4x4 mat4x4_transpose(mat4x4 m);
void right_multiply_mat4x4(mat4x4 *matrix, mat4x4 *B);
mat4x4 mat4x4_multiply(mat4x4 A, mat4x4 B);
mat4x4 mat4x4_multiply3(mat4x4 A, mat4x4 B, mat4x4 C);
mat4x4 mat4x4_multiply4(mat4x4 A, mat4x4 B, mat4x4 C, mat4x4 D);
vec4 matrix_vec4(mat4x4 matrix, vec4 v);
// "Rigid" 4x4 matrix routines. A "rigid" matrix is one that represents a frame of reference.
mat4x4 rigid_mat4x4_inverse(mat4x4 m);
vec3 rigid_matrix_vec3(mat4x4 matrix, vec3 v);
vec3 translation_vector_rigid_mat4x4(mat4x4 m);
mat3x3 rotation_part_rigid_mat4x4(mat4x4 m);
mat4x4 mat4x4_lookat(vec3 origin, vec3 look_at, vec3 approx_up);

// Printing functions.
//================================================================================
void print_vec3(vec3 v);
void print_vec4(vec4 v);
void print_mat3x3(mat3x3 m);
void print_mat4x4(mat4x4 m);

#endif // MATHEMATICS_H

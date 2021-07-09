#ifndef TEXTURES_H
#define TEXTURES_H

typedef struct RGBImageData_s {
    uint8_t *image; // length: 3*width*height.
    int width;
    int height;
} RGBImageData;
RGBImageData load_rgb_bmp(FILE *file);

typedef struct Texture_s {
    GLuint texture_id;
} Texture;
Texture load_texture(char *path);

typedef struct SkyBox_s {
    Texture top;
    Texture bottom;
    Texture right;
    Texture left;
    Texture forward;
    Texture back;
    float size;
} SkyBox;
SkyBox *add_skybox(Entity *e, char *top_texture, char *bottom_texture, char *right_texture, char *left_texture, char *forward_texture, char *back_texture, float size);
void skybox_update(Entity *e, Behaviour *b);

#endif // TEXTURES_H

#include "museum.h"

/*
    This code has been converted into C and modified from the loadBMP function given in the labs.
*/
RGBImageData load_rgb_bmp(FILE *file)
{
#define load_error(ERROR_STRING) {\
    fprintf(stderr, "Error loading BMP file: " ERROR_STRING "\n");\
    exit(EXIT_FAILURE);\
}
    uint32_t width, height;
    uint16_t planes, bpp;
#define header_length (18 + 12 + 24)
    char header[header_length] = {0};
    if (fread(header, header_length, 1, file) == 0) load_error("Could not read header.");
    if (header[0] != 'B' || header[1] != 'M') load_error("Incorrect magic number.");
    width = *((uint32_t *) (header + 18));
    height = *((uint32_t *) (header + 22));
    planes = *((uint16_t *) (header + 26));
    bpp = *((uint16_t *) (header + 28));
    // printf("Width: %d\nHeight: %d\nplanes: %d\nbpp: %d\n", width, height, planes, bpp);

    int num_bytes = bpp / 8;
    size_t size = width*height * num_bytes;
    // printf("size: %zu\n", size);
    uint8_t *image = malloc(size);
    mem_check(image);
    if (fread(image, size, 1, file) == 0) load_error("Could not read image data.");
    RGBImageData image_data;
    if (num_bytes == 3) {
        // Assume this BMP is BGR, so just swap the first and third bytes.
        // Swap r and b values.
        for(int i = 0; i < width*height; i++) {
            int temp = image[3*i];
            image[3*i] = image[3*i+2];
            image[3*i+2] = temp;
        }
        image_data.image = image;
    } else if (num_bytes == 4) {
        // Assume this is BGRA.
        image_data.image = malloc(3*width*height);
        mem_check(image_data.image);
        for (int i = 0; i < width*height; i++) {
            image_data.image[3*i+0] = image[4*i+2];
            image_data.image[3*i+1] = image[4*i+1];
            image_data.image[3*i+2] = image[4*i+0];
        }
        free(image);
    } else load_error("Invalid number of bytes per pixel.");
    image_data.width = width;
    image_data.height = height;
    return image_data;
}
Texture load_texture(char *path)
{
    // Textures can only be loaded from BMP files. These BMP files must also be uncompressed, use minimal BMP functionality, and be 8-bit-channel RGB.
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "ERROR: Could not find file \"%s\" when attempting to load texture.\n", path);
        exit(EXIT_FAILURE);
    }

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Make sure texture-related state allows the correct loading of textures.
    // https://stackoverflow.com/questions/7380773/glteximage2d-segfault-related-to-width-height
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    // glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    // glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    RGBImageData image_data = load_rgb_bmp(file);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image_data.width, image_data.height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data.image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

    glBindTexture(GL_TEXTURE_2D, 0);
    free(image_data.image);
    fclose(file);

    Texture texture;
    texture.texture_id = texture_id;
    return texture;
}

SkyBox *add_skybox(Entity *e, char *top_texture, char *bottom_texture, char *right_texture, char *left_texture, char *forward_texture, char *back_texture, float size)
{
    SkyBox *box = (SkyBox *) add_behaviour(e, skybox_update, sizeof(SkyBox), NoID)->data;
    box->top = load_texture(top_texture);
    box->bottom = load_texture(bottom_texture);
    box->right = load_texture(right_texture);
    box->left = load_texture(left_texture);
    box->forward = load_texture(forward_texture);
    box->back = load_texture(back_texture);
    box->size = size;
    return box;
}
void skybox_update(Entity *e, Behaviour *b)
{
    SkyBox *box = (SkyBox *) b->data;
    prepare_entity_matrix(e);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    Texture textures[6] = { box->top, box->bottom, box->right, box->left, box->forward, box->back };
    static struct {
        vec3 corners[4];
        float corner_uvs[8];
    } sides[6] = {
        // {0,1,0, 1,1,0, 1,1,1, 0,1,1,   0,1,1,1,0,1,0,0},
        // {0,0,0, 1,0,0, 1,0,1, 0,0,1,   0,0,1,0,1,1,0,1},
        // {1,0,1, 1,0,0, 1,1,0, 1,1,1,   1,0,0,0,0,1,1,1},
        // {0,0,1, 0,0,0, 0,1,0, 0,1,1,   0,0,1,0,1,1,0,1},
        // {0,0,1, 1,0,1, 1,1,1, 0,1,1,   1,0,0,0,0,1,1,1},
        // {0,0,0, 1,0,0, 1,1,0, 0,1,0,   0,0,1,0,1,1,0,1},
        #define D 0.01
        //{0,1-D,0, 1,1-D,0, 1,1-D,1, 0,1-D,1,   0,1,1,1,0,1,0,0},
        {0,1-D,0, 1,1-D,0, 1,1-D,1, 0,1-D,1,   1,1,0,1,0,0,1,0},
        {0,D,0, 1,D,0, 1,D,1, 0,D,1,   0,0,1,0,1,1,0,1},
        {1-D,0,1, 1-D,0,0, 1-D,1,0, 1-D,1,1,   1,0,0,0,0,1,1,1},
        {D,0,1, D,0,0, D,1,0, D,1,1,   0,0,1,0,1,1,0,1},
        {0,0,1-D, 1,0,1-D, 1,1,1-D, 0,1,1-D,   1,0,0,0,0,1,1,1},
        {0,0,D, 1,0,D, 1,1,D, 0,1,D,   0,0,1,0,1,1,0,1},
    };

    for (int i = 0; i < 6; i++) {
        glBindTexture(GL_TEXTURE_2D, textures[i].texture_id);
        glBegin(GL_QUADS);
        glColor3f(1,1,1);
        for (int j = 0; j < 4; j++) {
            glTexCoord2f(sides[i].corner_uvs[2*j], sides[i].corner_uvs[2*j+1]);
            vec3 corner = vec3_mul(vec3_sub(sides[i].corners[j], new_vec3(0.5,0.5,0.5)), box->size);
            glVertex3f(UNPACK_VEC3(corner));
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
}

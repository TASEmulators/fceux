#ifndef _ES2_H_
#define _ES2_H_
#include "em.h"
#include "es2util.h"
#include "ntsc.h"

// TODO: Set elsewhere?
#define DBG_MODE 0
#if DBG_MODE
#define DBG(x_) x_;
extern void updateUniformsDebug();
#else
#define DBG(x_)
#endif

// Set overscan on left and right sides as 12px (total 24px).
#define OVERSCAN_W  12
#define IDX_W       (INPUT_W + 2*OVERSCAN_W)
#define IDX_H       INPUT_H

#define SCREEN_W    (NUM_SUBPS * INPUT_W)
#define SCREEN_H    (4 * IDX_H)

// Texture units.
// For normal pipeline:
#define IDX_I           0
#define DEEMP_I         1
#define LOOKUP_I        2
#define RGB_I           3
#define SHARPEN_I       4
// For TV pipeline:
#define TV_I            0
#define DOWNSAMPLE0_I   4 // Used temporarily.
#define DOWNSAMPLE1_I   1
#define DOWNSAMPLE2_I   5 // Used temporarily.
#define DOWNSAMPLE3_I   2
#define DOWNSAMPLE4_I   6 // Used temporarily.
#define DOWNSAMPLE5_I   3
// Shared by both pipelines:
#define STRETCH_I       5
#define NOISE_I         6

#define TEX(i_) (GL_TEXTURE0+(i_))

// Uniforms.
#define ES2UniformsSpec \
U(RGB, BRIGHTNESS, brightness)\
U(RGB, CONTRAST, contrast)\
U(RGB, COLOR, color)\
U(RGB, GAMMA, gamma)\
U(RGB, NOISE_AMP, noiseAmp)\
U(RGB, NOISE_RND, noiseRnd)\
U(RGB, IDX_TEX, idxTex)\
U(RGB, DEEMP_TEX, deempTex)\
U(RGB, LOOKUP_TEX, lookupTex)\
U(RGB, NOISE_TEX, noiseTex)\
U(RGB, MINS, mins)\
U(RGB, MAXS, maxs)

typedef enum t_ES2Unif {
U_NONE = -1,
#define U(prog_, id_, name_) U_ ## id_,
ES2UniformsSpec
#undef U
U_COUNT
} ES2Unif;

// Uniform locations.
typedef struct t_ES2Uniforms
{
    GLint u[2 * U_COUNT];

    GLint _downsample_offsets_loc;
    GLint _downsample_downsampleTex_loc;

    GLint _sharpen_kernel_loc;
    GLint _sharpen_convergence_loc;

    GLint _stretch_scanlines_loc;
    GLint _stretch_smoothenOffs_loc;

    GLint _screen_uv_scale_loc;
    GLint _screen_border_uv_offs_loc;
    GLint _screen_mvp_loc;

    GLint _tv_mvp_loc;

    GLint _combine_glow_loc;

    GLint _direct_v_scale_loc;
    GLint _direct_uv_offset_loc;

} ES2Uniforms;

typedef struct t_ES2
{
    GLuint idx_tex;     // Input indexed color texture (NES palette, 256x240).
    GLuint deemp_tex;   // Input de-emphasis bits per row (240x1).
    GLuint lookup_tex;  // Palette to voltage levels lookup texture.

    GLuint noise_tex;   // Noise texture.

    GLuint rgb_fb;      // Framebuffer for output RGB texture generation.
    GLuint rgb_tex;     // Output RGB texture.
    GLuint rgb_prog;    // Shader for RGB generation.

    GLuint ntsc_prog;    // Shader for NTSC emulation.

    GLuint sharpen_fb;   // Framebuffer for sharpened RGB texture.
    GLuint sharpen_tex;  // Sharpened RGB texture.
    GLuint sharpen_prog; // Shader for sharpening.

    GLuint stretch_fb;   // Framebuffer for stretched RGB texture.
    GLuint stretch_tex;  // Output stretched RGB texture.
    GLuint stretch_prog; // Shader for stretched RGB.

    GLuint screen_prog;  // Shader for screen.

    GLuint tv_fb;        // Framebuffer to render screen and TV.
    GLuint tv_tex;       // Texture for screen/TV framebuffer.
    GLuint tv_prog;      // Shader for tv.

    GLuint combine_prog; // Shader for combine.

    GLuint direct_prog;  // Shader for rendering texture directly to the screen.

    GLint viewport[4];  // Screen viewport.

    GLfloat mvp_mat[4*4]; // Perspective MVP matrix for the meshes.

    GLuint downsample_fb[6];  // Framebuffers for downscaling.
    GLuint downsample_tex[6]; // Downsample textures.
    GLuint downsample_prog;   // Shader for downscaling.

    ES2Mesh quad_mesh;
    ES2Mesh screen_mesh;
    ES2Mesh tv_mesh;

    GLubyte overscan_color;   // Current overscan color (background/zero color).
    GLubyte *overscan_pixels; // Temporary overscan pixels (1x240).

} ES2;

extern ES2 s_p;
extern ES2Uniforms s_u;
extern const char ES2_common_shader_src[];

int ES2_Init(const char* canvasQuerySelector, double aspect);
void ES2_UpdateController(int idx, double v);
void ES2_SetViewport(int width, int height);
void ES2_VideoChanged();
void ES2_Render(uint8 *pixels, uint8 *row_deemp, uint8 overscan_color);

void ES2_TVInit();
void ES2_TVVideoChanged();
void ES2_TVRender();

#endif

/* FCE Ultra - NES/Famicom Emulator - Emscripten OpenGL ES 2.0 driver
 *
 * Copyright notice for this file:
 *  Copyright (C) 2015 Valtteri "tsone" Heikkila
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "es2.h"
#include <cmath>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
#include "es2util.h"
#include "../../fceu.h"
#include "../../ines.h"

// Video config.
std::string em_region = "auto";
static bool em_video_ntsc = true;
static bool em_video_tv = false;
static double em_video_sharpness = 0.2;
static double em_video_scanlines = 0.1;
static double em_video_convergence = 0.4;
static double em_video_noise = 0.3;

#define STR(s_) _STR(s_)
#define _STR(s_) #s_

#define PERSISTENCE_R   0.165 // Red phosphor persistence.
#define PERSISTENCE_G   0.205 // Green "
#define PERSISTENCE_B   0.225 // Blue "

#define NOISE_W     256
#define NOISE_H     256
#define RGB_W       (NUM_SUBPS * IDX_W)

ES2 s_p;
ES2Uniforms s_u;

const char ES2_common_shader_src[] = "precision mediump float;\n";

static const GLint mesh_quad_vert_num = 4;
static const GLint mesh_quad_face_num = 2;
static const GLfloat mesh_quad_verts[] = {
    -1.0f, -1.0f, 0.0f,
     1.0f, -1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f, 0.0f
};
static const GLfloat mesh_quad_uvs[] = {
     0.0f,  0.0f,
     1.0f,  0.0f,
     0.0f,  1.0f,
     1.0f,  1.0f
};
static const GLfloat mesh_quad_norms[] = {
     0.0f,  0.0f, 1.0f,
     0.0f,  0.0f, 1.0f,
     0.0f,  0.0f, 1.0f,
     0.0f,  0.0f, 1.0f
};
static ES2VArray mesh_quad_varrays[] = {
    { 3, GL_FLOAT, 0, (const void*) mesh_quad_verts },
    { 3, GL_FLOAT, 0, (const void*) mesh_quad_norms },
    { 2, GL_FLOAT, 0, (const void*) mesh_quad_uvs }
};

static void updateSharpenKernel()
{
    glUseProgram(s_p.sharpen_prog);
    // TODO: Calculated in multiple places? Pull out function?
    GLfloat v = em_video_ntsc * 0.4 * (em_video_sharpness+0.5);
    GLfloat sharpen_kernel[] = {
        -v, -v, -v,
        1, 0, 0,
        2*v, 1+2*v, 2*v,
        0, 0, 1,
        -v, -v, -v
    };
    glUniform3fv(s_u._sharpen_kernel_loc, 5, sharpen_kernel);
}

// Generate lookup texture.
void genLookupTex()
{
    glActiveTexture(TEX(LOOKUP_I));
    createTex(&s_p.lookup_tex, LOOKUP_W, NUM_COLORS, GL_RGB, GL_NEAREST, GL_CLAMP_TO_EDGE, (void*) ntscGetLookup().texture);
}

// Get uniformly distributed random number in [0,1] range.
static double rand01()
{
    return emscripten_random();
}

static void genNoiseTex()
{
    GLubyte *noise = (GLubyte*) malloc(NOISE_W * NOISE_H);

    // Box-Muller method gaussian noise.
    // Results are clamped to 0..255 range, which skews the distribution slightly.
    const double SIGMA = 0.5/2.0; // Set 95% of noise values in [-0.5,0.5] range.
    const double MU = 0.5; // Offset range by +0.5 to map to [0,1].
    for (int i = 0; i < NOISE_W*NOISE_H; i++) {
        double x;
        do {
            x = rand01();
        } while (x < 1e-7); // Epsilon to avoid log(0).

        double r = SIGMA * sqrt(-2.0 * log10(x));
        r = MU + r*sin(2.0*M_PI * rand01()); // Take real part only, discard complex part as it's related.
        // Clamp result to [0,1].
        noise[i] = (r <= 0) ? 0 : (r >= 1) ? 255 : (int) (0.5 + 255.0*r);
    }

    glActiveTexture(TEX(NOISE_I));
    createTex(&s_p.noise_tex, NOISE_W, NOISE_H, GL_LUMINANCE, GL_LINEAR, GL_REPEAT, noise);

    free(noise);
}

#if DBG_MODE
void updateUniformsDebug()
{
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    int k = glGetUniformLocation(prog, "u_mouse");
    GLfloat mouse[3] = { MouseData[0], MouseData[1], MouseData[2] };
    glUniform3fv(k, 1, mouse);
}
#endif

static void setUnifRGB1i(ES2Unif id, GLint v)
{
    glUseProgram(s_p.rgb_prog);
    glUniform1i(s_u.u[id], v);
    glUseProgram(s_p.ntsc_prog);
    glUniform1i(s_u.u[U_COUNT + id], v);
}

static void setUnifRGB3fv(ES2Unif id, GLfloat *v)
{
    glUseProgram(s_p.rgb_prog);
    glUniform3fv(s_u.u[id], 1, v);
    glUseProgram(s_p.ntsc_prog);
    glUniform3fv(s_u.u[U_COUNT + id], 1, v);
}

static void setUnifRGB1f(ES2Unif id, double v)
{
    glUseProgram(s_p.rgb_prog);
    glUniform1f(s_u.u[id], v);
    glUseProgram(s_p.ntsc_prog);
    glUniform1f(s_u.u[U_COUNT + id], v);
}

static void setUnifRGB2f(ES2Unif id, double a, double b)
{
    glUseProgram(s_p.rgb_prog);
    glUniform2f(s_u.u[id], a, b);
    glUseProgram(s_p.ntsc_prog);
    glUniform2f(s_u.u[U_COUNT + id], a, b);
}

static void updateUniformsRGB()
{
    DBG(updateUniformsDebug())
    setUnifRGB2f(U_NOISE_RND, rand01(), rand01());
}

static void updateUniformsSharpen()
{
    DBG(updateUniformsDebug())
}

static void updateUniformsStretch()
{
    DBG(updateUniformsDebug())
}

static void updateUniformsDirect(int video_changed)
{
    DBG(updateUniformsDebug())
    if (video_changed) {
        glUniform1f(s_u._direct_v_scale_loc, em_scanlines / 240.0f);
    }
    if (em_video_ntsc) {
        glUniform2f(s_u._direct_uv_offset_loc, -0.5f/SCREEN_W, -0.5f/SCREEN_H);
    } else {
        glUniform2f(s_u._direct_uv_offset_loc, 0, 0);
    }
}

static const char* s_unif_names[] =
{
#define U(prog_, id_, name_) "u_" # name_,
ES2UniformsSpec
#undef U
};

static void initUniformsRGB()
{
    for (int i = 0; i < U_COUNT; i++) {
        s_u.u[i] = glGetUniformLocation(s_p.rgb_prog, s_unif_names[i]);
        s_u.u[U_COUNT + i] = glGetUniformLocation(s_p.ntsc_prog, s_unif_names[i]);
    }

    setUnifRGB1i(U_IDX_TEX, IDX_I);
    setUnifRGB1i(U_DEEMP_TEX, DEEMP_I);
    setUnifRGB1i(U_LOOKUP_TEX, LOOKUP_I);
    setUnifRGB1i(U_NOISE_TEX, NOISE_I);

    setUnifRGB3fv(U_MINS, (GLfloat*) ntscGetLookup().yiq_mins);
    setUnifRGB3fv(U_MAXS, (GLfloat*) ntscGetLookup().yiq_maxs);

    updateUniformsRGB();
}

static void initUniformsSharpen()
{
    GLint k;
    GLuint prog = s_p.sharpen_prog;

    k = glGetUniformLocation(prog, "u_rgbTex");
    glUniform1i(k, RGB_I);

    s_u._sharpen_convergence_loc = glGetUniformLocation(prog, "u_convergence");
    s_u._sharpen_kernel_loc = glGetUniformLocation(prog, "u_sharpenKernel");
    updateUniformsSharpen();
}

static void initUniformsStretch()
{
    GLint k;
    GLuint prog = s_p.stretch_prog;

    k = glGetUniformLocation(prog, "u_sharpenTex");
    glUniform1i(k, SHARPEN_I);

    s_u._stretch_scanlines_loc = glGetUniformLocation(prog, "u_scanlines");
    s_u._stretch_smoothenOffs_loc = glGetUniformLocation(prog, "u_smoothenOffs");
    updateUniformsStretch();
}

static void initUniformsDirect()
{
    GLint k;
    GLuint prog = s_p.direct_prog;

    k = glGetUniformLocation(prog, "u_tex");
    glUniform1i(k, STRETCH_I);

    s_u._direct_v_scale_loc = glGetUniformLocation(prog, "u_vScale");
    s_u._direct_uv_offset_loc = glGetUniformLocation(prog, "u_uvOffset");
    updateUniformsDirect(1);
}

static void passRGB()
{
    glBindFramebuffer(GL_FRAMEBUFFER, s_p.rgb_fb);
    glViewport(0, 0, RGB_W, IDX_H);
    updateUniformsRGB();

    if (em_video_tv) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_COLOR);
    }

    if (em_video_ntsc) {
        glUseProgram(s_p.ntsc_prog);
    } else {
        glUseProgram(s_p.rgb_prog);
    }
    meshRender(&s_p.quad_mesh);

    glDisable(GL_BLEND);
}

static void passSharpen()
{
    glBindFramebuffer(GL_FRAMEBUFFER, s_p.sharpen_fb);
    glViewport(0, 0, SCREEN_W, IDX_H);
    glUseProgram(s_p.sharpen_prog);
    updateUniformsSharpen();
    meshRender(&s_p.quad_mesh);
}

static void passStretch()
{
    glBindFramebuffer(GL_FRAMEBUFFER, s_p.stretch_fb);
    glViewport(0, 0, SCREEN_W, SCREEN_H);
    glUseProgram(s_p.stretch_prog);
    updateUniformsStretch();
    meshRender(&s_p.quad_mesh);
}

static void passDirect()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(s_p.viewport[0], s_p.viewport[1], s_p.viewport[2], s_p.viewport[3]);
    glUseProgram(s_p.direct_prog);
    updateUniformsDirect(0);
    meshRender(&s_p.quad_mesh);
}

static void Video_SetBrightness(double brightness)
{
    setUnifRGB1f(U_BRIGHTNESS, 0.15 * brightness);
}

static void Video_SetContrast(double contrast)
{
    setUnifRGB1f(U_CONTRAST, 1 + 0.4 * contrast);
}

static void Video_SetColor(double color)
{
    setUnifRGB1f(U_COLOR, 1 + color);
}

static void Video_SetGamma(double gamma)
{
    constexpr double ntsc_gamma = 2.44;
    constexpr double srgb_gamma = 2.2;
    setUnifRGB1f(U_GAMMA, (ntsc_gamma / srgb_gamma) + 0.3 * gamma);
}

static void Video_SetNoise(double noise)
{
    em_video_noise = noise;

    noise = 0.08 * noise * noise;
    setUnifRGB1f(U_NOISE_AMP, noise);
}

static void Video_SetConvergence(double convergence)
{
    em_video_convergence = convergence;

    glUseProgram(s_p.sharpen_prog);
    convergence = em_video_tv * 2.0 * convergence;
    glUniform1f(s_u._sharpen_convergence_loc, convergence);
}

static void Video_SetSharpness(double sharpness)
{
    em_video_sharpness = sharpness;

    updateSharpenKernel();
}

static void Video_SetScanlines(double scanlines)
{
    em_video_scanlines = scanlines;

    glUseProgram(s_p.stretch_prog);
    scanlines = em_video_tv * 0.45 * scanlines;
    glUniform1f(s_u._stretch_scanlines_loc, scanlines);
}

static void Video_SetGlow(double glow)
{
    glUseProgram(s_p.combine_prog);
    glow = 0.1 * glow;
    glUniform3f(s_u._combine_glow_loc, glow, glow * glow, glow + glow * glow);
}

static void Video_SetNtsc(bool enable)
{
    em_video_ntsc = enable;

    updateSharpenKernel();
    // Stretch pass smoothen UV offset; smoothen if NTSC emulation is enabled.
    glUseProgram(s_p.stretch_prog);
    glUniform2f(s_u._stretch_smoothenOffs_loc, 0, enable * -0.25/IDX_H);
}

static void Video_SetTv(bool enable)
{
    em_video_tv = enable;

    // Enable TV, update dependent uniforms. (Without modifying stored control values.)
    Video_SetNoise(em_video_noise);
    Video_SetScanlines(em_video_scanlines);
    Video_SetConvergence(em_video_convergence);
}

bool Video_SetConfig(const std::string& key, const emscripten::val& value)
{
    if (key == "video-system") {
        Video_SetSystem(value.as<std::string>());
    } else if (key == "video-ntsc") {
        Video_SetNtsc(value.as<double>());
    } else if (key == "video-tv") {
        Video_SetTv(value.as<double>());
    } else if (key == "video-brightness") {
        Video_SetBrightness(value.as<double>());
    } else if (key == "video-contrast") {
        Video_SetContrast(value.as<double>());
    } else if (key == "video-color") {
        Video_SetColor(value.as<double>());
    } else if (key == "video-sharpness") {
        Video_SetSharpness(value.as<double>());
    } else if (key == "video-gamma") {
        Video_SetGamma(value.as<double>());
    } else if (key == "video-noise") {
        Video_SetNoise(value.as<double>());
    } else if (key == "video-convergence") {
        Video_SetConvergence(value.as<double>());
    } else if (key == "video-scanlines") {
        Video_SetScanlines(value.as<double>());
    } else if (key == "video-glow") {
        Video_SetGlow(value.as<double>());
    } else {
        return false;
    }
    return true;
}

void Video_SetSystem(const std::string& region)
{
    if (region == "auto") {
        FCEUI_SetRegion(iNESDetectVidSys()); // Attempt auto-detection.
    } else if (region == "ntsc") {
        FCEUI_SetRegion(0);
    } else if (region == "pal") {
        FCEUI_SetRegion(1);
    } else if (region == "dendy") {
        FCEUI_SetRegion(2);
    } else {
        FCEU_PrintError("Invalid region: '%s'", region.c_str());
        return;
    }

    em_region = region;

    if (!GameInfo) {
        // Required if a game was not loaded.
        FCEUD_VideoChanged();
    }
}

// On failure, return value < 0, otherwise success.
static int ES2_CreateWebGLContext(const char* canvasQuerySelector)
{
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = attr.antialias = attr.premultipliedAlpha = 0;
    attr.depth = attr.stencil = attr.preserveDrawingBuffer  = attr.failIfMajorPerformanceCaveat = 0;
    attr.powerPreference = EM_WEBGL_POWER_PREFERENCE_LOW_POWER;
    attr.enableExtensionsByDefault = 0;
    attr.majorVersion = 1;
    attr.minorVersion = 0;
    attr.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(canvasQuerySelector, &attr);
    if (ctx > 0) {
        emscripten_webgl_make_context_current(ctx);
    }
    return ctx;
}

int ES2_Init(const char* canvasQuerySelector, double aspect)
{
    if (ES2_CreateWebGLContext(canvasQuerySelector) <= 0) {
        return 0;
    }

    // Build perspective MVP matrix.
    GLfloat trans[3] = { 0, 0, -2.5 };
    GLfloat proj[4*4];
    GLfloat view[4*4];
    // Stretch aspect slightly as the TV mesh is wee bit vertically squished.
    aspect *= 1.04;
    mat4Persp(proj, 0.25*M_PI / aspect, aspect, 0.125, 16.0);
    mat4Trans(view, trans);
    mat4Mul(s_p.mvp_mat, proj, view);

    s_p.overscan_pixels = (GLubyte*) malloc(OVERSCAN_W*IDX_H);
    s_p.overscan_color = 0xFE; // Set bogus value to ensure overscan update.

    glGetIntegerv(GL_VIEWPORT, s_p.viewport);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBlendColor(PERSISTENCE_R, PERSISTENCE_G, PERSISTENCE_B, 0);
    glClearColor(0, 0, 0, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_FALSE);
    glStencilMask(GL_FALSE);
    glEnableVertexAttribArray(0);

    createMesh(&s_p.quad_mesh, mesh_quad_vert_num, ARRAY_SIZE(mesh_quad_varrays), mesh_quad_varrays, 2*mesh_quad_face_num, 0);

    // Setup input pixels texture.
    glActiveTexture(TEX(IDX_I));
    createTex(&s_p.idx_tex, IDX_W, IDX_H, GL_LUMINANCE, GL_NEAREST, GL_CLAMP_TO_EDGE, 0);

    // Setup input de-emphasis rows texture.
    glActiveTexture(TEX(DEEMP_I));
    createTex(&s_p.deemp_tex, IDX_H, 1, GL_LUMINANCE, GL_NEAREST, GL_CLAMP_TO_EDGE, 0);

    genLookupTex();
    genNoiseTex();

    // Configure RGB framebuffer.
    glActiveTexture(TEX(RGB_I));
    createFBTex(&s_p.rgb_tex, &s_p.rgb_fb, RGB_W, IDX_H, GL_RGB, GL_NEAREST, GL_CLAMP_TO_EDGE);
    s_p.rgb_prog = buildShaderFile("/data/rgb.vert", "/data/rgb.frag", ES2_common_shader_src);
    s_p.ntsc_prog = buildShaderFile("/data/ntsc.vert", "/data/ntsc.frag", ES2_common_shader_src);
    initUniformsRGB();

    // Setup sharpen framebuffer.
    glActiveTexture(TEX(SHARPEN_I));
    createFBTex(&s_p.sharpen_tex, &s_p.sharpen_fb, SCREEN_W, IDX_H, GL_RGB, GL_NEAREST, GL_CLAMP_TO_EDGE);
    s_p.sharpen_prog = buildShaderFile("/data/sharpen.vert", "/data/sharpen.frag", ES2_common_shader_src);
    initUniformsSharpen();

    // Setup stretch framebuffer.
    glActiveTexture(TEX(STRETCH_I));
    createFBTex(&s_p.stretch_tex, &s_p.stretch_fb, SCREEN_W, SCREEN_H, GL_RGB, GL_LINEAR, GL_CLAMP_TO_EDGE);
    s_p.stretch_prog = buildShaderFile("/data/stretch.vert", "/data/stretch.frag", ES2_common_shader_src);
    initUniformsStretch();

    ES2_TVInit();

    // Setup direct shader.
    s_p.direct_prog = buildShaderFile("/data/direct.vert", "/data/direct.frag", ES2_common_shader_src);
    initUniformsDirect();

    return 1;
}

void ES2_SetViewport(int width, int height)
{
    s_p.viewport[2] = width;
    s_p.viewport[3] = height;
}

void ES2_VideoChanged()
{
    ES2_TVVideoChanged();

    glUseProgram(s_p.direct_prog);
    updateUniformsDirect(1);
}

void ES2_Render(uint8 *pixels, uint8 *row_deemp, uint8 overscan_color)
{
    // Update input pixels.
    glActiveTexture(TEX(IDX_I));
    glBindTexture(GL_TEXTURE_2D, s_p.idx_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, OVERSCAN_W, 0, IDX_W-2*OVERSCAN_W, IDX_H, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
    if (s_p.overscan_color != overscan_color) {
        s_p.overscan_color = overscan_color;

        memset(s_p.overscan_pixels, overscan_color, OVERSCAN_W * IDX_H);

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, OVERSCAN_W, IDX_H, GL_LUMINANCE, GL_UNSIGNED_BYTE, s_p.overscan_pixels);
        glTexSubImage2D(GL_TEXTURE_2D, 0, IDX_W-OVERSCAN_W, 0, OVERSCAN_W, IDX_H, GL_LUMINANCE, GL_UNSIGNED_BYTE, s_p.overscan_pixels);
    }

    // Update input de-emphasis rows.
    glActiveTexture(TEX(DEEMP_I));
    glBindTexture(GL_TEXTURE_2D, s_p.deemp_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, IDX_H, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, row_deemp);

    passRGB();
    passSharpen();
    passStretch();
    if (em_video_tv) {
        ES2_TVRender();
    } else {
        passDirect();
    }
}

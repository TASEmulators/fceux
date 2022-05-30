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
#include "meshes.h"
#include <cmath>
#include "../../fceu.h"

static ES2VArray mesh_screen_varrays[] = {
    { 3, GL_FLOAT, 0, (const void*) mesh_screen_verts },
    { 3, GL_FLOAT, VARRAY_ENCODED_NORMALS, (const void*) mesh_screen_norms },
    { 2, GL_FLOAT, 0, (const void*) mesh_screen_uvs }
};

static ES2VArray mesh_rim_varrays[] = {
    { 3, GL_FLOAT, 0, (const void*) mesh_rim_verts },
    { 3, GL_FLOAT, VARRAY_ENCODED_NORMALS, (const void*) mesh_rim_norms },
    { 3, GL_FLOAT, 0, 0 },
    // { 3, GL_FLOAT, (const void*) mesh_rim_vcols }
    { 3, GL_UNSIGNED_BYTE, 0, (const void*) mesh_rim_vcols }
};

// Texture sample offsets and weights for gaussian w/ radius=8, sigma=4.
// Eliminates aliasing and blurs for "faked" glossy reflections and AO.
static const GLfloat s_downsample_offs[] = { -6.892337f, -4.922505f, -2.953262f, -0.98438f, 0.98438f, 2.953262f, 4.922505f, 6.892337f };
static const GLfloat s_downsample_ws[] = { 0.045894f, 0.096038f, 0.157115f, 0.200954f, 0.200954f, 0.157115f, 0.096038f, 0.045894f };

static const int s_downsample_widths[]  = { SCREEN_W, SCREEN_W/4, SCREEN_W/4,  SCREEN_W/16, SCREEN_W/16, SCREEN_W/64, SCREEN_W/64 };
static const int s_downsample_heights[] = { SCREEN_H, SCREEN_H,   SCREEN_H/4,  SCREEN_H/4,  SCREEN_H/16, SCREEN_H/16, SCREEN_H/64 };

static void updateUniformsScreen(int video_changed)
{
    DBG(updateUniformsDebug())

    if (video_changed) {
        glUniform2f(s_u._screen_uv_scale_loc, 1.0 + 25.0/INPUT_W,
            (em_scanlines/240.0) * (1.0 + 15.0/INPUT_H));
        glUniform2f(s_u._screen_border_uv_offs_loc, 0.5 * (1.0 - 9.5/INPUT_W),
            0.5 * (1.0 - (7.0 + INPUT_H - em_scanlines) / INPUT_H));
    }
}

static void updateUniformsDownsample(int w, int h, int texIdx, int isHorzPass)
{
    DBG(updateUniformsDebug())
    GLfloat offsets[2*8];
    if (isHorzPass) {
        for (int i = 0; i < 8; ++i) {
            offsets[2*i  ] = s_downsample_offs[i] / w;
            offsets[2*i+1] = 0;
        }
    } else {
        for (int i = 0; i < 8; ++i) {
            offsets[2*i  ] = 0;
            offsets[2*i+1] = s_downsample_offs[i] / h;
        }
    }
    glUniform2fv(s_u._downsample_offsets_loc, 8, offsets);
    glUniform1i(s_u._downsample_downsampleTex_loc, texIdx);
}

static void updateUniformsTV()
{
    DBG(updateUniformsDebug())
}

static void updateUniformsCombine()
{
    DBG(updateUniformsDebug())
}

// Generated with following python oneliners:
// from math import *
// def rot(a,b): return [sin(a)*sin(b), -sin(a)*cos(b), -cos(a)]
// def rad(d): return pi*d/180
// rot(rad(90-65),rad(15))
//static GLfloat s_lightDir[] = { -0.109381654946615, 0.40821789367673483, 0.9063077870366499 }; // 90-65, 15
//static GLfloat s_lightDir[] = { -0.1830127018922193, 0.6830127018922193, 0.7071067811865476 }; // 90-45, 15
//static GLfloat s_lightDir[] = { -0.22414386804201336, 0.8365163037378078, 0.5000000000000001 }; // 90-30, 15
static const GLfloat s_lightDir[] = { 0.0, 0.866025, 0.5 }; // 90-30, 0
static const GLfloat s_viewPos[] = { 0, 0, 2.5 };
static const GLfloat s_xAxis[] = { 1, 0, 0 };
static const GLfloat s_shadowPoint[] = { 0, 0.7737, 0.048 };

static void initShading(GLuint prog, float intensity, float diff, float fill, float spec, float m, float fr0, float frexp)
{
    int k = glGetUniformLocation(prog, "u_lightDir");
    glUniform3fv(k, 1, s_lightDir);
    k = glGetUniformLocation(prog, "u_viewPos");
    glUniform3fv(k, 1, s_viewPos);
    k = glGetUniformLocation(prog, "u_material");
    glUniform4f(k, intensity*diff / M_PI, intensity*spec * (m+8.0) / (8.0*M_PI), m, intensity*fill / M_PI);
    k = glGetUniformLocation(prog, "u_fresnel");
    glUniform3f(k, fr0, 1-fr0, frexp);

    GLfloat shadowPlane[4];
    vec3Cross(shadowPlane, s_xAxis, s_lightDir);
    vec3Normalize(shadowPlane, shadowPlane);
    shadowPlane[3] = vec3Dot(shadowPlane, s_shadowPoint);
    k = glGetUniformLocation(prog, "u_shadowPlane");
    glUniform4fv(k, 1, shadowPlane);
}

static void initUniformsScreen()
{
    GLint k;
    GLuint prog = s_p.screen_prog;

    k = glGetUniformLocation(prog, "u_stretchTex");
    glUniform1i(k, STRETCH_I);
    k = glGetUniformLocation(prog, "u_noiseTex");
    glUniform1i(k, NOISE_I);
    k = glGetUniformLocation(prog, "u_mvp");
    glUniformMatrix4fv(k, 1, GL_FALSE, s_p.mvp_mat);

    initShading(prog, 4.0, 0.001, 0.0, 0.065, 41, 0.04, 4);

    s_u._screen_uv_scale_loc = glGetUniformLocation(prog, "u_uvScale");
    s_u._screen_border_uv_offs_loc = glGetUniformLocation(prog, "u_borderUVOffs");
    updateUniformsScreen(1);
}

static void initUniformsTV()
{
    GLint k;
    GLuint prog = s_p.tv_prog;

    k = glGetUniformLocation(prog, "u_downsample1Tex");
    glUniform1i(k, DOWNSAMPLE1_I);
    k = glGetUniformLocation(prog, "u_downsample3Tex");
    glUniform1i(k, DOWNSAMPLE3_I);
    k = glGetUniformLocation(prog, "u_downsample5Tex");
    glUniform1i(k, DOWNSAMPLE5_I);
    k = glGetUniformLocation(prog, "u_noiseTex");
    glUniform1i(k, NOISE_I);

    k = glGetUniformLocation(prog, "u_mvp");
    glUniformMatrix4fv(k, 1, GL_FALSE, s_p.mvp_mat);

    initShading(prog, 4.0, 0.0038, 0.0035, 0.039, 49, 0.03, 4);

    updateUniformsTV();
}

static void initUniformsDownsample()
{
    GLint k;
    GLuint prog = s_p.downsample_prog;

    k = glGetUniformLocation(prog, "u_weights");
    glUniform1fv(k, 8, s_downsample_ws);

    s_u._downsample_offsets_loc = glGetUniformLocation(prog, "u_offsets");
    s_u._downsample_downsampleTex_loc = glGetUniformLocation(prog, "u_downsampleTex");
    updateUniformsDownsample(280, 240, DOWNSAMPLE0_I, 1);
}

static void initUniformsCombine()
{
    GLint k;
    GLuint prog = s_p.combine_prog;

    k = glGetUniformLocation(prog, "u_tvTex");
    glUniform1i(k, TV_I);
    k = glGetUniformLocation(prog, "u_downsample3Tex");
    glUniform1i(k, DOWNSAMPLE3_I);
    k = glGetUniformLocation(prog, "u_downsample5Tex");
    glUniform1i(k, DOWNSAMPLE5_I);
    k = glGetUniformLocation(prog, "u_noiseTex");
    glUniform1i(k, NOISE_I);

    s_u._combine_glow_loc = glGetUniformLocation(prog, "u_glow");
}

static void passDownsamplePart(int i, int src)
{
    glBindFramebuffer(GL_FRAMEBUFFER, s_p.downsample_fb[i]);
    glViewport(0, 0, s_downsample_widths[i+1], s_downsample_heights[i+1]);

    updateUniformsDownsample(s_downsample_widths[i], s_downsample_heights[i], src, !(i & 1));
    meshRender(&s_p.quad_mesh);
}

static void passDownsample()
{
    glUseProgram(s_p.downsample_prog);

    glActiveTexture(TEX(TV_I));
    glBindTexture(GL_TEXTURE_2D, s_p.tv_tex);

    glActiveTexture(TEX(DOWNSAMPLE0_I));
    glBindTexture(GL_TEXTURE_2D, s_p.downsample_tex[0]);
    glActiveTexture(TEX(DOWNSAMPLE1_I));
    glBindTexture(GL_TEXTURE_2D, s_p.downsample_tex[1]);
    glActiveTexture(TEX(DOWNSAMPLE2_I));
    glBindTexture(GL_TEXTURE_2D, s_p.downsample_tex[2]);
    glActiveTexture(TEX(DOWNSAMPLE3_I));
    glBindTexture(GL_TEXTURE_2D, s_p.downsample_tex[3]);
    glActiveTexture(TEX(DOWNSAMPLE4_I));
    glBindTexture(GL_TEXTURE_2D, s_p.downsample_tex[4]);
    glActiveTexture(TEX(DOWNSAMPLE5_I));
    glBindTexture(GL_TEXTURE_2D, s_p.downsample_tex[5]);

    passDownsamplePart(0, TV_I);
    passDownsamplePart(1, DOWNSAMPLE0_I);
    passDownsamplePart(2, DOWNSAMPLE1_I);
    passDownsamplePart(3, DOWNSAMPLE2_I);
    passDownsamplePart(4, DOWNSAMPLE3_I);
    passDownsamplePart(5, DOWNSAMPLE4_I);
}

static void passScreen()
{
    glBindFramebuffer(GL_FRAMEBUFFER, s_p.tv_fb);
    glViewport(0, 0, SCREEN_W, SCREEN_H);
    glUseProgram(s_p.screen_prog);
    updateUniformsScreen(0);
    meshRender(&s_p.screen_mesh);
}

static void passTV()
{
    glBindFramebuffer(GL_FRAMEBUFFER, s_p.tv_fb);
    glViewport(0, 0, SCREEN_W, SCREEN_H);

    glUseProgram(s_p.tv_prog);
    updateUniformsTV();
    meshRender(&s_p.tv_mesh);
}

static void passCombine()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(s_p.viewport[0], s_p.viewport[1], s_p.viewport[2], s_p.viewport[3]);
    glUseProgram(s_p.combine_prog);
    updateUniformsCombine();
    meshRender(&s_p.quad_mesh);
}

static void beginTVPipeline()
{
}

static void endTVPipeline()
{
    glActiveTexture(TEX(IDX_I));
    glBindTexture(GL_TEXTURE_2D, s_p.idx_tex);

    glActiveTexture(TEX(DEEMP_I));
    glBindTexture(GL_TEXTURE_2D, s_p.deemp_tex);

    glActiveTexture(TEX(LOOKUP_I));
    glBindTexture(GL_TEXTURE_2D, s_p.lookup_tex);

    glActiveTexture(TEX(RGB_I));
    glBindTexture(GL_TEXTURE_2D, s_p.rgb_tex);

    glActiveTexture(TEX(STRETCH_I));
    glBindTexture(GL_TEXTURE_2D, s_p.stretch_tex);

    glActiveTexture(TEX(NOISE_I));
    glBindTexture(GL_TEXTURE_2D, s_p.noise_tex);

    glActiveTexture(TEX(SHARPEN_I));
    glBindTexture(GL_TEXTURE_2D, s_p.sharpen_tex);
}

void ES2_TVInit()
{
    // Setup screen/TV framebuffer.
    glActiveTexture(TEX(TV_I));
    createFBTex(&s_p.tv_tex, &s_p.tv_fb, SCREEN_W, SCREEN_H, GL_RGB, GL_LINEAR, GL_CLAMP_TO_EDGE);

    // Setup downsample framebuffers.
    for (int i = 0; i < 6; ++i) {
        // glActiveTexture(TEX(DOWNSAMPLE0_I + i));
        createFBTex(&s_p.downsample_tex[i], &s_p.downsample_fb[i],
            s_downsample_widths[i+1], s_downsample_heights[i+1],
            GL_RGB, GL_LINEAR, GL_CLAMP_TO_EDGE);
    }

    // Setup downsample shader.
    s_p.downsample_prog = buildShaderFile("/data/downsample.vert", "/data/downsample.frag", ES2_common_shader_src);
    initUniformsDownsample();

    // Setup screen shader.
    s_p.screen_prog = buildShaderFile("/data/screen.vert", "/data/screen.frag", ES2_common_shader_src);
    createMesh(&s_p.screen_mesh, mesh_screen_vert_num, ARRAY_SIZE(mesh_screen_varrays), mesh_screen_varrays, 3*mesh_screen_face_num, mesh_screen_faces);
    initUniformsScreen();

    // Setup TV shader.
    s_p.tv_prog = buildShaderFile("/data/tv.vert", "/data/tv.frag", ES2_common_shader_src);
    int num_edges = 0;
    int *edges = createUniqueEdges(&num_edges, mesh_screen_vert_num, 3*mesh_screen_face_num, mesh_screen_faces);
    num_edges *= 2;

    GLfloat *rim_extra = (GLfloat*) malloc(3*sizeof(GLfloat) * mesh_rim_vert_num);
    for (int i = 0; i < mesh_rim_vert_num; ++i) {
        GLfloat p[3];
        vec3Set(p, &mesh_rim_verts[3*i]);
        GLfloat shortest[3] = { 0, 0, 0 };
        double shortestDist = 1000000;
        for (int j = 0; j < num_edges; j += 2) {
            int ai = 3*edges[j];
            int bi = 3*edges[j+1];
            GLfloat diff[3];
            vec3ClosestOnSegment(diff, p, &mesh_screen_verts[ai], &mesh_screen_verts[bi]);
            vec3Sub(diff, diff, p);
            double dist = vec3Length2(diff);
            if (dist < shortestDist) {
                shortestDist = dist;
                vec3Set(shortest, diff);
            }
        }
        // TODO: Could interpolate uv with vert normal here, and not in vertex shader?
        rim_extra[3*i] = shortest[0];
        rim_extra[3*i+1] = shortest[1];
        rim_extra[3*i+2] = shortest[2];
    }
    mesh_rim_varrays[2].data = rim_extra;
    createMesh(&s_p.tv_mesh, mesh_rim_vert_num, ARRAY_SIZE(mesh_rim_varrays), mesh_rim_varrays, 3*mesh_rim_face_num, mesh_rim_faces);
    free(edges);
    free(rim_extra);
    initUniformsTV();

    // Setup combine shader.
    s_p.combine_prog = buildShaderFile("/data/combine.vert", "/data/combine.frag", ES2_common_shader_src);
    initUniformsCombine();

    endTVPipeline();
}

void ES2_TVVideoChanged()
{
    glUseProgram(s_p.screen_prog);
    updateUniformsScreen(1);
}

void ES2_TVRender()
{
    beginTVPipeline();
    passScreen();
    passDownsample();
    passTV();
    passCombine();
    endTVPipeline();
}

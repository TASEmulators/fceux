/* FCE Ultra - NES/Famicom Emulator - Emscripten OpenGL ES 2.0 utils
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
#include "es2util.h"
#include "../../driver.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#define MAX_VARRAYS 4

static int is_varray_enabled[MAX_VARRAYS];

void vec3Set(GLfloat *c, const GLfloat *a)
{
    c[0] = a[0];
    c[1] = a[1];
    c[2] = a[2];
}

void vec3Sub(GLfloat *c, const GLfloat *a, const GLfloat *b)
{
    c[0] = a[0] - b[0];
    c[1] = a[1] - b[1];
    c[2] = a[2] - b[2];
}

void vec3Add(GLfloat *c, const GLfloat *a, const GLfloat *b)
{
    c[0] = a[0] + b[0];
    c[1] = a[1] + b[1];
    c[2] = a[2] + b[2];
}

void vec3MulScalar(GLfloat *c, const GLfloat *a, double s)
{
    c[0] = a[0] * s;
    c[1] = a[1] * s;
    c[2] = a[2] * s;
}

void vec3DivScalar(GLfloat *c, const GLfloat *a, double s)
{
    c[0] = a[0] / s;
    c[1] = a[1] / s;
    c[2] = a[2] / s;
}

double vec3Dot(const GLfloat *a, const GLfloat *b)
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

void vec3Cross(GLfloat *result, const GLfloat *a, const GLfloat *b)
{
    result[0] = a[1]*b[2] - a[2]*b[1];
    result[1] = a[2]*b[0] - a[0]*b[2];
    result[2] = a[0]*b[1] - a[1]*b[0];
}

double vec3Length2(const GLfloat *a)
{
    return vec3Dot(a, a);
}

double vec3Length(const GLfloat *a)
{
    return sqrt(vec3Length2(a));
}

double vec3Normalize(GLfloat *c, const GLfloat *a)
{
    double d = vec3Length(a);
    if (d > 0) {
        vec3DivScalar(c, a, d);
    } else {
        c[0] = c[1] = c[2] = 0;
    }
    return d;
}

double vec3ClosestOnSegment(GLfloat *result, const GLfloat *p, const GLfloat *a, const GLfloat *b)
{
    GLfloat pa[3], ba[3];
    vec3Sub(pa, p, a);
    vec3Sub(ba, b, a);
    double dotbaba = vec3Length2(ba);
    double t = 0;
    if (dotbaba != 0) {
        t = vec3Dot(pa, ba) / dotbaba;
        if (t < 0) t = 0;
        else if (t > 1) t = 1;
    }
    vec3MulScalar(ba, ba, t);
    vec3Add(result, a, ba);
    return t;
}

void mat4Persp(GLfloat *p, double fovy, double aspect, double zNear, double zFar)
{
    const double f = 1.0 / tan(fovy / 2.0);
    memset(p, 0, 4*4*sizeof(GLfloat));
    p[4*0  ] = f / aspect;
    p[4*1+1] = f;
    p[4*2+2] = (zNear+zFar) / (zNear-zFar);
    p[4*2+3] = -1.0;
    p[4*3+2] = (2.0*zNear*zFar) / (zNear-zFar);
}

void mat4Trans(GLfloat *p, GLfloat *trans)
{
    memset(p, 0, 4*4*sizeof(GLfloat));
    p[4*0  ] = 1.0;
    p[4*1+1] = 1.0;
    p[4*2+2] = 1.0;
    p[4*3+3] = 1.0;
    p[4*3  ] = trans[0];
    p[4*3+1] = trans[1];
    p[4*3+2] = trans[2];
}

void mat4Mul(GLfloat *p, const GLfloat *a, const GLfloat *b)
{
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            double v = 0;
            for (int k = 0; k < 4; k++) {
                v += a[4*k + j] * b[4*i + k];
            }
            p[4*i + j] = v;
        }
    }
}

static GLuint compileShader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, 0);
    glCompileShader(shader);

    GLint value;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &value);
    if (value == GL_FALSE) {
        FCEUD_PrintError("Shader compilation failed:");
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &value);
        if (value > 0) {
          char *buf = (char*) malloc(value);
          glGetShaderInfoLog(shader, value, 0, buf);
          FCEUI_printf("%s\n", buf);
          free(buf);
        } else {
            FCEUD_PrintError("No shader info log!");
        }
        FCEUI_printf("Shader source:\n%s\n", src);
    }

    return shader;
}

static GLuint linkShader(GLuint vert_shader, GLuint frag_shader)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert_shader);
    glAttachShader(prog, frag_shader);
    char name[] = "a_0";
    for (int i = 0; i < MAX_VARRAYS; i++) {
        glBindAttribLocation(prog, i, name);
        name[2]++;
    }
    glLinkProgram(prog);
    glUseProgram(prog);
    return prog;
}

static char* prependShader(const char *src, const char *prepend)
{
    char* result = (char*) malloc(strlen(prepend) + strlen(src) + 1);
    strcpy(result, prepend);
    strcat(result, src);
    return result;
}

GLuint buildShader(const char *vert_src, const char *frag_src, const char *prepend_src)
{
    if (prepend_src) {
        vert_src = prependShader(vert_src, prepend_src);
        frag_src = prependShader(frag_src, prepend_src);
    }

    GLuint vert_shader = compileShader(GL_VERTEX_SHADER, vert_src);
    GLuint frag_shader = compileShader(GL_FRAGMENT_SHADER, frag_src);
    GLuint result = linkShader(vert_shader, frag_shader);
    glDetachShader(result, vert_shader);
    glDetachShader(result, frag_shader);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    if (prepend_src) {
        free((char*) vert_src);
        free((char*) frag_src);
    }
    return result;
}

static char *readShaderFile(const char *fn)
{
    FILE *f = fopen(fn, "rb");
    if (!f) {
        FCEUI_printf("ERROR: Can't read shader file: %s\n", fn);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    const size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *result = (char*) malloc(size + 1);
    fread(result, 1, size, f);
    fclose(f);
    result[size] = '\0';
    return result;
}

GLuint buildShaderFile(const char *vert_fn, const char *frag_fn, const char *prepend_src)
{
    char *vert_src = readShaderFile(vert_fn);
    char *frag_src = readShaderFile(frag_fn);
    GLuint ret = 0;
    if (vert_src && frag_src) {
        ret = buildShader(vert_src, frag_src, prepend_src);
    }
    free(vert_src);
    free(frag_src);
    return ret;
}

void deleteShader(GLuint *prog)
{
    if (prog && *prog) {
        glDeleteProgram(*prog);
        *prog = 0;
    }
}

void createBuffer(GLuint *buf, GLenum binding, GLsizei size, const void *data)
{
    glGenBuffers(1, buf);
    glBindBuffer(binding, *buf);
    glBufferData(binding, size, data, GL_STATIC_DRAW);
}

void deleteBuffer(GLuint *buf)
{
    if (buf && *buf) {
        glDeleteBuffers(1, buf);
        *buf = 0;
    }
}

void createTex(GLuint *tex, int w, int h, GLenum format, GLenum filter, GLenum wrap, void *data)
{
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);

// TODO: No mipmaps. Remove?
/*
    if ((min_filter == GL_LINEAR_MIPMAP_LINEAR)
            || (min_filter == GL_LINEAR_MIPMAP_NEAREST)
            || (min_filter == GL_NEAREST_MIPMAP_LINEAR)
            || (min_filter == GL_NEAREST_MIPMAP_NEAREST)) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
*/
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

void deleteTex(GLuint *tex)
{
    if (tex && *tex) {
        glDeleteTextures(1, tex);
        *tex = 0;
    }
}

void createFBTex(GLuint *tex, GLuint *fb, int w, int h, GLenum format, GLenum filter, GLenum wrap)
{
    createTex(tex, w, h, format, filter, wrap, 0);

    glGenFramebuffers(1, fb);
    glBindFramebuffer(GL_FRAMEBUFFER, *fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void deleteFBTex(GLuint *tex, GLuint *fb)
{
    deleteTex(tex);
    if (fb && *fb) {
        glDeleteFramebuffers(1, fb);
        *fb = 0;
    }
}

// TODO: Disabled for now...
#if 0
// Inverse edge length weighted method. As described in Real-time rendering (3rd ed.) p.546.
// Ref: Max, N. L., (1999) 'Weights for computing vertex normals from facet normals'
//   in Journal of grahics tools, vol.4, no.2, pp.1-6.
static GLfloat* meshGenerateNormals(int num_verts, int num_elems, const GLfloat *verts, const GLushort *elems)
{
    GLfloat *norms = (GLfloat*) calloc(3*num_verts, sizeof(GLfloat));
    GLfloat e1[3], e2[3], no[3];

    for (int i = 0; i < num_elems; i += 3) {
        for (int j = 0; j < 3; ++j) {
            int a = 3*elems[i+j];
            int b = 3*elems[i+((j+1)%3)];
            int c = 3*elems[i+((j+2)%3)];

            vec3Sub(e1, &verts[c], &verts[b]);
            vec3Sub(e2, &verts[a], &verts[b]);
            vec3Cross(no, e1, e2);

            double d = vec3Length2(e1) * vec3Length2(e2);
            vec3DivScalar(no, no, d);
            vec3Add(&norms[b], &norms[b], no);
        }
    }

    for (int i = 0; i < 3*num_verts; i += 3) {
        vec3Normalize(&norms[i], &norms[i]);
    }

    return norms;
}
#endif

static int type2Size(GLenum type)
{
    switch (type) {
    case GL_FLOAT: return sizeof(GLfloat);
    case GL_UNSIGNED_BYTE:
    case GL_BYTE: return sizeof(GLbyte);
    case GL_UNSIGNED_SHORT:
    case GL_SHORT: return sizeof(GLshort);
    case GL_UNSIGNED_INT:
    case GL_INT: return sizeof(GLint);
    default: return 0;
    }
}

static int verts2Type(int num_verts)
{
    if (num_verts <= 256) return GL_UNSIGNED_BYTE;
    else if (num_verts <= 65536) return GL_UNSIGNED_SHORT;
    else return GL_UNSIGNED_INT;
}

void createMesh(ES2Mesh *p, int num_verts, int num_varrays, ES2VArray *varrays, int num_elems, const void *elems)
{
    memset(p, 0, sizeof(ES2Mesh));
    p->num_varrays = num_varrays;
    p->varrays = varrays;
    p->num_elems = num_elems;
    p->elem_type = verts2Type(num_verts);

    for (int i = 0; i < num_varrays; i++) {
        GLsizei size = varrays[i].count * type2Size(varrays[i].type) * num_verts;
        const GLushort *enc = (const GLushort*) varrays[i].data;
        GLfloat *norms = 0;
        if (varrays[i].flags & VARRAY_ENCODED_NORMALS) {
            norms = (GLfloat*) malloc(size);
            for (int j = 0; j < num_verts; j++) {
                double a = 1.0*M_PI * enc[2*j  ] / 65535.0;
                double b = 2.0*M_PI * enc[2*j+1] / 65535.0;
                norms[3*j  ] = sin(a) * cos(b);
                norms[3*j+1] = sin(a) * sin(b);
                norms[3*j+2] = cos(a);
            }
        }

        if (norms) {
            createBuffer(&varrays[i]._buf, GL_ARRAY_BUFFER, size, norms);
            free(norms);
        } else {
            createBuffer(&varrays[i]._buf, GL_ARRAY_BUFFER, size, varrays[i].data);
        }
    }

// DEBUG: Code to find mesh AABB.
#if 0
    GLfloat mins[3] = { .0f, .0f, .0f }, maxs[3] = { .0f, .0f, .0f };
    for (int i = 0; i < 3*num_verts; i += 3) {
        for (int j = 0; j < 3; ++j) {
            if (verts[i+j] > maxs[j]) {
                maxs[j] = verts[i+j];
            } else if (verts[i+j] < mins[j]) {
                mins[j] = verts[i+j];
            }
        }
    }
    FCEUI_printf("verts:%d aabb: min:%.5f,%.5f,%.5f max:%.5f,%.5f,%.5f\n", num_verts,
        mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);
#endif

// TODO: Testing normal generation in code... Might save some space.
#if 0
        GLfloat *ns = meshGenerateNormals(num_verts, num_elems, verts, elems);
        createBuffer(&p->norm_buf, GL_ARRAY_BUFFER, 3*sizeof(GLfloat)*num_verts, ns);
        free(ns);
#endif

    if (elems) {
        GLsizei size = type2Size(p->elem_type) * num_elems;
        createBuffer(&p->elem_buf, GL_ELEMENT_ARRAY_BUFFER, size, elems);
    }
}

void deleteMesh(ES2Mesh *p)
{
    for (int i = 0; i < p->num_varrays; i++) {
        deleteBuffer(&p->varrays[i]._buf);
    }
    deleteBuffer(&p->elem_buf);
}

void meshRender(ES2Mesh *p)
{
    for (int i = 0; i < MAX_VARRAYS; i++) {
        if (i < p->num_varrays && p->varrays[i]._buf) {
            glEnableVertexAttribArray(i);
            is_varray_enabled[i] = 1;
            glBindBuffer(GL_ARRAY_BUFFER, p->varrays[i]._buf);
            GLboolean normalized = (p->varrays[i].type == GL_FLOAT) ? GL_FALSE : GL_TRUE;
            glVertexAttribPointer(i, p->varrays[i].count, p->varrays[i].type, normalized, 0, 0);
        } else if (is_varray_enabled[i]) {
            glDisableVertexAttribArray(i);
            is_varray_enabled[i] = 0;
        }
    }

    if (p->elem_buf) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p->elem_buf);
        glDrawElements(GL_TRIANGLES, p->num_elems, p->elem_type, 0);
    } else {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, p->num_elems);
    }
}

// num_elems: Must be 3*<number of triangles>, i.e. index buffer number of elements.
int *createUniqueEdges(int *num_edges, int num_verts, int num_elems, const void *elems)
{
    GLuint *elems_copy = (GLuint*) malloc(sizeof(GLuint) * num_elems);

    if (num_verts <= 256) {
        const GLubyte *e = (const GLubyte*) elems;
        for (GLuint i = 0; i < num_elems; i++) {
            elems_copy[i] = e[i];
        }
    } else if (num_verts <= 65536) {
        const GLushort *e = (const GLushort*) elems;
        for (GLuint i = 0; i < num_elems; i++) {
            elems_copy[i] = e[i];
        }
    } else {
        memcpy(elems_copy, elems, sizeof(GLuint) * num_elems);
    }

    int *edges = (int*) malloc(2*sizeof(int) * num_elems);
    // int *face_edges = (int*) malloc(sizeof(int) * num_elems);
    int n = 0;
    // Find unique edges using O(n^2) process. Small, but not fast.
    for (int i = 0; i < num_elems; i += 3) {
        for (int j = 0; j < 3; ++j) {
            int a0 = elems_copy[i+j];
            int b0 = elems_copy[i+((j+1)%3)];
            int k;
            // Check if we already have the same edge.
            for (k = 0; k < n; k += 2) {
                int a1 = edges[k];
                int b1 = edges[k+1];
                if (a0 == b1 && b0 == a1) {
                    // Duplicate edge with opposite direction.
                    // k = -k;
                    break;
                }
                if (a0 == a1 && b0 == b1) {
                    // Duplicate edge with same direction.
                    break;
                }
            }
            if (k == n) {
                edges[n] = a0;
                edges[n+1] = b0;
                n += 2;
            }
            // Set found edge as face edge.
            // face_edges[i+j] = k / 2;
        }
    }

    free(elems_copy);
    *num_edges = n / 2;
    return edges;
}


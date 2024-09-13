#include <string.h>

#include <stdio.h>
#include <dc/matrix.h>
#include <dc/matrix3d.h>
#include <dc/vec3f.h>

#include "private.h"
#include "../include/gl.h"
#include "../containers/stack.h"

#define DEG2RAD (0.01745329251994329576923690768489)

/* Depth range */
GLfloat DEPTH_RANGE_MULTIPLIER_L = (1 - 0) / 2;
GLfloat DEPTH_RANGE_MULTIPLIER_H = (0 + 1) / 2;

/* Viewport size */
static GLint gl_viewport_x1, gl_viewport_y1, gl_viewport_width, gl_viewport_height;

static Stack MATRIX_STACKS[3]; // modelview, projection, texture
static Matrix4x4 NORMAL_MATRIX __attribute__((aligned(32)));
static Matrix4x4 SCREENVIEW_MATRIX __attribute__((aligned(32)));

static GLenum MATRIX_MODE = GL_MODELVIEW;
static GLubyte MATRIX_IDX = 0;

static const Matrix4x4 IDENTITY __attribute__((aligned(32))) = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

static Matrix4x4 WORK_MATRIX __attribute__((aligned(32))) = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

GLfloat NEAR_PLANE_DISTANCE = 0.0f;

static void _glStoreNearPlane() {
    Matrix4x4* proj = (Matrix4x4*) stack_top(MATRIX_STACKS + (GL_PROJECTION & 0xF));

    GLfloat a = *(*proj + 10);
    GLfloat b = *(*proj + 14);

    NEAR_PLANE_DISTANCE = -b / (1.0f - a);
}

static void print_matrix(Matrix4x4 *m) {
    float *_m = (float*) *m;
    printf("%f %f %f %f\n", _m[0], _m[1], _m[2], _m[3]);
    printf("%f %f %f %f\n", _m[4], _m[5], _m[6], _m[7]);
    printf("%f %f %f %f\n", _m[8], _m[9], _m[10], _m[11]);
    printf("%f %f %f %f\n", _m[12], _m[13], _m[14], _m[15]);
}

static inline void upload_matrix(Matrix4x4* m) {
    printf("GLDC: Uploading matrix %d\n", MATRIX_IDX);
    print_matrix(m);
    mat_load((matrix_t*) m);
}

static inline void multiply_matrix(Matrix4x4* m) {
    printf("GLDC: Multiplying matrix %d\n", MATRIX_IDX);
    print_matrix(m);
    mat_apply((matrix_t*) m);
}

static inline void download_matrix(Matrix4x4* m) {
    printf("GLDC: Downloading matrix %d\n", MATRIX_IDX);
    mat_store((matrix_t*) m);
}

Matrix4x4* _glGetProjectionMatrix() {
    return (Matrix4x4*) stack_top(&MATRIX_STACKS[1]);
}

Matrix4x4* _glGetModelViewMatrix() {
    return (Matrix4x4*) stack_top(&MATRIX_STACKS[0]);
}

void _glInitMatrices() {
    init_stack(&MATRIX_STACKS[0], sizeof(Matrix4x4), 32);
    init_stack(&MATRIX_STACKS[1], sizeof(Matrix4x4), 32);
    init_stack(&MATRIX_STACKS[2], sizeof(Matrix4x4), 32);

    stack_push(&MATRIX_STACKS[0], IDENTITY);
    stack_push(&MATRIX_STACKS[1], IDENTITY);
    stack_push(&MATRIX_STACKS[2], IDENTITY);

    memcpy4(NORMAL_MATRIX, IDENTITY, sizeof(Matrix4x4));
    memcpy4(SCREENVIEW_MATRIX, IDENTITY, sizeof(Matrix4x4));

    glDepthRange(0.0f, 1.0f);
    glViewport(0, 0, vid_mode->width, vid_mode->height);
}

#define swap(a, b) { \
    GLfloat x = (a); \
    a = b; \
    b = x; \
}

static void inverse(GLfloat* m) {
    GLfloat f4 = m[4];
    GLfloat f8 = m[8];
    GLfloat f1 = m[1];
    GLfloat f9 = m[9];
    GLfloat f2 = m[2];
    GLfloat f6 = m[6];
    GLfloat f12 = m[12];
    GLfloat f13 = m[13];
    GLfloat f14 = m[14];

    m[1]  =  f4;
    m[2]  =  f8;
    m[4]  =  f1;
    m[6]  =  f9;
    m[8]  =  f2;
    m[9]  =  f6;
    m[12] = -(f12 * m[0]  +  f13 * m[4]  +  f14 * m[8]);
    m[13] = -(f12 * m[1]  +  f13 * m[5]  +  f14 * m[9]);
    m[14] = -(f12 * m[2]  +  f13 * m[6]  +  f14 * m[10]);
}

static void transpose(GLfloat* m) {
    swap(m[1], m[4]);
    swap(m[2], m[8]);
    swap(m[3], m[12]);
    swap(m[6], m[9]);
    swap(m[7], m[3]);
    swap(m[11], m[14]);
}

static void recalculateNormalMatrix() {
    memcpy4(NORMAL_MATRIX, stack_top(MATRIX_STACKS + (GL_MODELVIEW & 0xF)), sizeof(Matrix4x4));
    inverse((GLfloat*) NORMAL_MATRIX);
    transpose((GLfloat*) NORMAL_MATRIX);
}

void APIENTRY glMatrixMode(GLenum mode) {
    MATRIX_MODE = mode;
    MATRIX_IDX = mode & 0xF;
}

void APIENTRY glPushMatrix() {
    stack_push(MATRIX_STACKS + MATRIX_IDX, stack_top(MATRIX_STACKS + MATRIX_IDX));
}

void APIENTRY glPopMatrix() {
    stack_pop(MATRIX_STACKS + MATRIX_IDX);
    if(MATRIX_MODE == GL_MODELVIEW) {
        recalculateNormalMatrix();
    }
}

void APIENTRY glLoadIdentity() {
    stack_replace(MATRIX_STACKS + MATRIX_IDX, IDENTITY);
}

void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    memcpy4(WORK_MATRIX, IDENTITY, sizeof(Matrix4x4));

    WORK_MATRIX[M12] = x;
    WORK_MATRIX[M13] = y;
    WORK_MATRIX[M14] = z;

    upload_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));
    multiply_matrix(&WORK_MATRIX);
    download_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));

    if(MATRIX_MODE == GL_MODELVIEW) {
        recalculateNormalMatrix();
    }
}


void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z) {
    memcpy4(WORK_MATRIX, IDENTITY, sizeof(Matrix4x4));

    WORK_MATRIX[M0] = x;
    WORK_MATRIX[M5] = y;
    WORK_MATRIX[M10] = z;

    upload_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));
    multiply_matrix(&WORK_MATRIX);
    download_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));

    if(MATRIX_MODE == GL_MODELVIEW) {
        recalculateNormalMatrix();
    }
}

void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat  y, GLfloat z) {
    memcpy4(WORK_MATRIX, IDENTITY, sizeof(Matrix4x4));

    float c, s;

    fsincos(angle, &s, &c);

    float invc = 1.0f - c;
    float xs = x * s;
    float zs = z * s;
    float ys = y * s;
    float xz = x * z;
    float xy = y * x;
    float yz = y * z;

    vec3f_normalize(x, y, z);

    WORK_MATRIX[M0] = (x * x) * invc + c;
    WORK_MATRIX[M1] = xy * invc + zs;
    WORK_MATRIX[M2] = xz * invc - ys;

    WORK_MATRIX[M4] = xy * invc - zs;
    WORK_MATRIX[M5] = (y * y) * invc + c;
    WORK_MATRIX[M6] = yz * invc + xs;

    WORK_MATRIX[M8] = xz * invc + ys;
    WORK_MATRIX[M9] = yz * invc - xs;
    WORK_MATRIX[M10] = (z * z) * invc + c;

    upload_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));
    multiply_matrix(&WORK_MATRIX);
    download_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));

    if(MATRIX_MODE == GL_MODELVIEW) {
        recalculateNormalMatrix();
    }
}

/* Load an arbitrary matrix */
void APIENTRY glLoadMatrixf(const GLfloat *m) {
    Matrix4x4 *TEMP = &WORK_MATRIX;

#if TRANSPOSE
    TEMP[M0] = m[0];
    TEMP[M1] = m[1];
    TEMP[M2] = m[2];
    TEMP[M3] = m[3];

    TEMP[M4] = m[4];
    TEMP[M5] = m[5];
    TEMP[M6] = m[6];
    TEMP[M7] = m[7];

    TEMP[M8] = m[8];
    TEMP[M9] = m[9];
    TEMP[M10] = m[10];
    TEMP[M11] = m[11];

    TEMP[M12] = m[12];
    TEMP[M13] = m[13];
    TEMP[M14] = m[14];
    TEMP[M15] = m[15];
#else
    memcpy4(TEMP, m, sizeof(Matrix4x4));
#endif

    stack_replace(MATRIX_STACKS + MATRIX_IDX, TEMP);

    if(MATRIX_MODE == GL_MODELVIEW) {
        recalculateNormalMatrix();
    }
}

/* Ortho */
void APIENTRY glOrtho(GLfloat left, GLfloat right,
             GLfloat bottom, GLfloat top,
             GLfloat znear, GLfloat zfar) {

    /* Ortho Matrix */
    float *OrthoMatrix = (float*)WORK_MATRIX;

    OrthoMatrix[M0] = 2.0f / (right - left);
    OrthoMatrix[M5] = 2.0f / (top - bottom);
    OrthoMatrix[M10] = -2.0f / (zfar - znear);
    OrthoMatrix[M12] = -(right + left) / (right - left);
    OrthoMatrix[M13] = -(top + bottom) / (top - bottom);
    OrthoMatrix[M14] = -(zfar + znear) / (zfar - znear);

    upload_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));
    multiply_matrix((Matrix4x4*)&OrthoMatrix);
    download_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));
}


/* Set the GL frustum */
void APIENTRY glFrustum(GLfloat left, GLfloat right,
               GLfloat bottom, GLfloat top,
               GLfloat znear, GLfloat zfar) {

    /* Frustum Matrix */
    float *FrustumMatrix = WORK_MATRIX;

    memset(FrustumMatrix, 0, sizeof(Matrix4x4));

    const float near2 = 2.0f * znear;
    const float A = (right + left) / (right - left);
    const float B = (top + bottom) / (top - bottom);
    const float C = -((zfar + znear) / (zfar - znear));
    const float D = -((2.0f * zfar * znear) / (zfar - znear));

    FrustumMatrix[M0] = near2 / (right - left);
    FrustumMatrix[M5] = near2 / (top - bottom);

    FrustumMatrix[M8] = A;
    FrustumMatrix[M9] = B;
    FrustumMatrix[M10] = C;
    FrustumMatrix[M11] = -1.0f;
    FrustumMatrix[M14] = D;

    upload_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));
    multiply_matrix((Matrix4x4*)&FrustumMatrix);
    download_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));

    if(MATRIX_MODE == GL_PROJECTION) {
        _glStoreNearPlane();
    }
}


/* Multiply the current matrix by an arbitrary matrix */
void glMultMatrixf(const GLfloat *m) {
    float *TEMP = WORK_MATRIX;

#if TRANSPOSE
    TEMP[M0] = m[0];
    TEMP[M1] = m[1];
    TEMP[M2] = m[2];
    TEMP[M3] = m[3];

    TEMP[M4] = m[4];
    TEMP[M5] = m[5];
    TEMP[M6] = m[6];
    TEMP[M7] = m[7];

    TEMP[M8] = m[8];
    TEMP[M9] = m[9];
    TEMP[M10] = m[10];
    TEMP[M11] = m[11];

    TEMP[M12] = m[12];
    TEMP[M13] = m[13];
    TEMP[M14] = m[14];
    TEMP[M15] = m[15];
#else
    memcpy4(TEMP, m, sizeof(Matrix4x4));
#endif

    upload_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));
    multiply_matrix((Matrix4x4*) &TEMP);
    download_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));

    if(MATRIX_MODE == GL_MODELVIEW) {
        recalculateNormalMatrix();
    }

    if(MATRIX_MODE == GL_PROJECTION) {
        _glStoreNearPlane();
    }
}

/* Load an arbitrary transposed matrix */
void glLoadTransposeMatrixf(const GLfloat *m) {
    /* We store matrices transpose anyway, so m will be
     * transpose compared to all other matrices */

    float *TEMP = WORK_MATRIX;

    TEMP[M0] = m[0];
    TEMP[M1] = m[4];
    TEMP[M2] = m[8];
    TEMP[M3] = m[12];

    TEMP[M4] = m[1];
    TEMP[M5] = m[5];
    TEMP[M6] = m[9];
    TEMP[M7] = m[13];

    TEMP[M8] = m[3];
    TEMP[M9] = m[6];
    TEMP[M10] = m[10];
    TEMP[M11] = m[14];

    TEMP[M12] = m[4];
    TEMP[M13] = m[7];
    TEMP[M14] = m[11];
    TEMP[M15] = m[15];

    stack_replace(MATRIX_STACKS + MATRIX_IDX, TEMP);

    if(MATRIX_MODE == GL_MODELVIEW) {
        recalculateNormalMatrix();
    }

    if(MATRIX_MODE == GL_PROJECTION) {
        _glStoreNearPlane();
    }
}

/* Multiply the current matrix by an arbitrary transposed matrix */
void glMultTransposeMatrixf(const GLfloat *m) {
    float *TEMP = WORK_MATRIX;

    TEMP[M0] = m[0];
    TEMP[M1] = m[4];
    TEMP[M2] = m[8];
    TEMP[M3] = m[12];

    TEMP[M4] = m[1];
    TEMP[M5] = m[5];
    TEMP[M6] = m[9];
    TEMP[M7] = m[13];

    TEMP[M8] = m[3];
    TEMP[M9] = m[6];
    TEMP[M10] = m[10];
    TEMP[M11] = m[14];

    TEMP[M12] = m[4];
    TEMP[M13] = m[7];
    TEMP[M14] = m[11];
    TEMP[M15] = m[15];

    upload_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));
    multiply_matrix((Matrix4x4*) &TEMP);
    download_matrix(stack_top(MATRIX_STACKS + MATRIX_IDX));

    if(MATRIX_MODE == GL_MODELVIEW) {
        recalculateNormalMatrix();
    }

    if(MATRIX_MODE == GL_PROJECTION) {
        _glStoreNearPlane();
    }
}

/* Set the GL viewport */
void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    gl_viewport_x1 = x;
    gl_viewport_y1 = y;
    gl_viewport_width = width;
    gl_viewport_height = height;

    GLfloat rw = x + width;
    GLfloat lw = x;
    GLfloat tw = y + height;
    GLfloat bw = y;

    GLfloat hw = ((GLfloat) width) / 2.0f;
    GLfloat hh = ((GLfloat) height) / 2.0f;

    SCREENVIEW_MATRIX[M0] = hw;
    SCREENVIEW_MATRIX[M5] = -hh;
    SCREENVIEW_MATRIX[M10] = 1;
    SCREENVIEW_MATRIX[M12] = (rw + lw) / 2.0f;
    SCREENVIEW_MATRIX[M13] = (tw + bw) / 2.0f;
}

GLfloat _glGetNearPlane() {
    return NEAR_PLANE_DISTANCE;
}

/* Set the depth range */
void APIENTRY glDepthRangef(GLclampf n, GLclampf f) {
    if(n < 0.0f) n = 0.0f;
    else if(n > 1.0f) n = 1.0f;

    if(f < 0.0f) f = 0.0f;
    else if(f > 1.0f) f = 1.0f;

    DEPTH_RANGE_MULTIPLIER_L = (f - n) / 2.0f;
    DEPTH_RANGE_MULTIPLIER_H = (n + f) / 2.0f;
}

void APIENTRY glDepthRange(GLclampf n, GLclampf f){
    glDepthRangef(n,f);
}

/* Vector Cross Product - Used by gluLookAt */
static inline void vec3f_cross(const GLfloat* v1, const GLfloat* v2, GLfloat* result) {
    result[0] = v1[1] * v2[2] - v1[2] * v2[1];
    result[1] = v1[2] * v2[0] - v1[0] * v2[2];
    result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

static inline void vec3f_normalize_sh4(float *v){
    float length, ilength;

	ilength = MATH_fsrra(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	length = MATH_Invert(ilength);
	if (length)
	{
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
}

void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx,
               GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy,
               GLfloat upz) {

    float *m = WORK_MATRIX;
   	GLfloat f [3];
	GLfloat u [3];
	GLfloat s [3];

	f[0] = centerx - eyex;
	f[1] = centery - eyey;
	f[2] = centerz - eyez;

	u[0] = upx;
	u[1] = upy;
	u[2] = upz;

    vec3f_normalize_sh4(f);
	vec3f_cross(f, u, s);
    vec3f_normalize_sh4(s);
	vec3f_cross(s, f, u);

	m[0] =  s[0]; m[4] =  s[1]; m[8] =   s[2]; m[12] = 0.0f;
	m[1] =  u[0]; m[5] =  u[1]; m[9] =   u[2]; m[13] = 0.0f;
	m[2] = -f[0]; m[6] = -f[1]; m[10] = -f[2]; m[14] = 0.0f;
    m[3] =   0.0f; m[7] =   0.0f; m[11] =   0.0f; m[15] = 1.0f;

    static Matrix4x4 trn __attribute__((aligned(32))) = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    trn[M12] = -eyex;
    trn[M13] = -eyey;
    trn[M14] = -eyez;

    // Does not modify internal Modelview matrix
    upload_matrix((Matrix4x4*) &m);
    multiply_matrix(&trn);
    multiply_matrix(stack_top(MATRIX_STACKS + (GL_MODELVIEW & 0xF)));
    download_matrix(stack_top(MATRIX_STACKS + (GL_MODELVIEW & 0xF)));
}


void _glApplyRenderMatrix() {
    upload_matrix(&SCREENVIEW_MATRIX);
    multiply_matrix(stack_top(MATRIX_STACKS + (GL_PROJECTION & 0xF)));
    multiply_matrix(stack_top(MATRIX_STACKS + (GL_MODELVIEW & 0xF)));
}

void _glMatrixLoadTexture() {
    upload_matrix(stack_top(MATRIX_STACKS + (GL_TEXTURE & 0xF)));
}

void _glMatrixLoadModelView() {
    upload_matrix(stack_top(MATRIX_STACKS + (GL_MODELVIEW & 0xF)));
}

void _glMatrixLoadNormal() {
    upload_matrix(&NORMAL_MATRIX);
}

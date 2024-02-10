#include <math.h>
#include "private.h"
#include "glext.h"

/* Set the Perspective */
void APIENTRY gluPerspective(GLfloat angle, GLfloat aspect,
                    GLfloat znear, GLfloat zfar) {
    GLfloat xmin, xmax, ymin, ymax;

    ymax = znear * tanf(angle * F_PI / 360.0f);
    ymin = -ymax;
    xmin = ymin * aspect;
    xmax = ymax * aspect;

    glFrustum(xmin, xmax, ymin, ymax, znear, zfar);
}

void APIENTRY gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top) {
    glOrtho(left, right, bottom, top, -1.0f, 1.0f);
}

extern GLuint _determinePVRFormat(GLint internalFormat, GLenum type);
extern GLint _determineStride(GLenum format, GLenum type);
extern GLint _cleanInternalFormat(GLint internalFormat);


GLint APIENTRY gluScaleImage(GLenum format,
		    GLsizei src_width, GLsizei src_height,
		    GLenum src_type, const GLvoid *src,
		    GLsizei dst_width, GLsizei dst_height,
		    GLenum dst_type, GLvoid *dst)
{
	(void) format;
    (void) src_width;
	(void) src_height;
    (void) src_type;
	(void) src;
    (void) dst_width, 
	(void) dst_height;
    (void) dst_type;
	(void) dst;

#if 0
    /* Calculate the format that we need to convert the data to */
    GLuint dst_format = _determinePVRFormat(format, src_type);
    GLuint pvr_format = _determinePVRFormat(format, dst_type);
	
    int i;

	if (src_width == dst_width &&
	    src_height == dst_height &&
	    src_type == dst_type) {
		memcpy(dst, src, dst_width * dst_height * 2 /* shorts? */);
		return 0;
	} else {
        return 0;
    }

	image = malloc(src_width * src_height * sizeof(pix_t));
	if (image == NULL)
		return GLU_OUT_OF_MEMORY;
	for(i = 0; i < src_height; i++)
		(*srcfmt->unpack)(srcfmt,
				  src + i * src_width * srcfmt->size,
				  &image[i * src_width],
				  src_width);

	if (src_width != dst_width)
		image = rescale_horiz(image, src_width, dst_width, src_height);

	if (src_height != dst_height)
		image = rescale_vert(image, dst_width, src_height, dst_height);

	if (image == NULL)
		return GL_OUT_OF_MEMORY;

	for(i = 0; i < dst_height; i++)
		(*dstfmt->pack)(dstfmt,
				&image[i * dst_width],
				dst + i * dst_width * dstfmt->size,
				dst_width);
	free(image);
    #endif
	return GL_INVALID_VALUE;
}

GLint APIENTRY gluBuild2DMipmaps( GLenum target,GLint internalFormat, GLsizei width, GLsizei height,
			 GLenum format,	 GLenum type, const void *data )
{
    #if 0
	GLsizei tw, th;
	const void *src;
	void *dst;
	int level, levels;
	GLint maxtex;

	tw = width;
	th = height;

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtex);

	while(tw > maxtex)
		tw /= 2;
	while(th > maxtex)
		th /= 2;	

	levels = 1 + floor(log2(tw));
	level = 1 + floor(log2(th));
	if (level > levels)
		levels = level;

	src = data;
	dst = NULL;

	for(level = 0; level <= levels; level++) {
		dst = malloc(tw * th * 2 /*shorts so 2 bytes*/);

		gluScaleImage(format,
			      width, height, type, src,
			      tw, th, type, dst);

		glTexImage2D(target, level, internalFormat, 
			     tw, th, 0, format, type, dst);

		if (src != data)
			free((void *)src);
		src = dst;
		width = tw;
		height = th;

		if (tw > 1)
			tw /= 2;
		if (th > 1)
			th /= 2;
	}
	free(dst);
    #endif
    glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
    glGenerateMipmapEXT(target);
	return 0;
}
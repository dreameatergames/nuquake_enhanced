#include "private.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "config.h"
#include "../include/glext.h"
#include "../include/glkos.h"

#define CLAMP_U (1<<1)
#define CLAMP_V (1<<0)

static TextureObject* TEXTURE_UNITS[MAX_TEXTURE_UNITS] = {NULL, NULL};
static NamedArray TEXTURE_OBJECTS;
GLubyte ACTIVE_TEXTURE = 0;

static TexturePalette* SHARED_PALETTES[4] = {NULL, NULL, NULL, NULL};

GLuint _determinePVRFormat(GLint internalFormat, GLenum type);

#define PACK_ARGB8888(a,r,g,b) ( ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF) )


#define _PACK4(v) ((v * 0xF) / 0xFF)
#define PACK_ARGB4444(a,r,g,b) (_PACK4(a) << 12) | (_PACK4(r) << 8) | (_PACK4(g) << 4) | (_PACK4(b))

static GLboolean BANKS_USED[4];  // Each time a 256 colour bank is used, this is set to true
static GLboolean SUBBANKS_USED[4][16]; // 4 counts of the used 16 colour banks within the 256 ones
static GLenum INTERNAL_PALETTE_FORMAT = GL_RGBA4;

static TexturePalette* _initTexturePalette() {
    TexturePalette* palette = (TexturePalette*) malloc(sizeof(TexturePalette));
    assert(palette);

    memset(palette, 0x0, sizeof(TexturePalette));
    palette->bank = -1;
    return palette;
}

static GLshort _glGenPaletteSlot(GLushort size) {
    GLushort i, j;

    assert(size == 16 || size == 256);

    if(size == 16) {
        for(i = 0; i < 4; ++i) {
            for(j = 0; j < 16; ++j) {
                if(!SUBBANKS_USED[i][j]) {
                    BANKS_USED[i] = GL_TRUE;
                    SUBBANKS_USED[i][j] = GL_TRUE;
                    return (i * 16) + j;
                }
            }
        }
    } else {
        for(i = 0; i < 4; ++i) {
            if(!BANKS_USED[i]) {
                BANKS_USED[i] = GL_TRUE;
                for(j = 0; j < 16; ++j) {
                    SUBBANKS_USED[i][j] = GL_TRUE;
                }
                return i;
            }
        }
    }

    fprintf(stderr, "GL ERROR: No palette slots remain\n");
    return -1;
}

static void _glReleasePaletteSlot(GLshort slot, GLushort size) {
    GLushort i;

    assert(size == 16 || size == 256);
    if(size == 16) {
        GLushort bank = slot / 4;
        GLushort subbank = slot % 4;

        SUBBANKS_USED[bank][subbank] = GL_FALSE;
        for(i = 0; i < 16; ++i) {
            if(SUBBANKS_USED[bank][i]) {
                return;
            }
        }

        BANKS_USED[bank] = GL_FALSE;
    } else {
        BANKS_USED[slot] = GL_FALSE;
        for(i = 0; i < 16; ++i) {
            SUBBANKS_USED[slot][i] = GL_FALSE;
        }
    }
}

TexturePalette* _glGetSharedPalette(GLshort bank) {
    assert(bank >= 0 && bank < 4);
    return SHARED_PALETTES[bank];
}

void _glSetInternalPaletteFormat(GLenum val) {
    INTERNAL_PALETTE_FORMAT = val;

    if(INTERNAL_PALETTE_FORMAT == GL_RGBA4) {
        pvr_set_pal_format(PVR_PAL_ARGB4444);
    } else {
        assert(INTERNAL_PALETTE_FORMAT == GL_RGBA8);
        pvr_set_pal_format(PVR_PAL_ARGB8888);
    }
}

void _glApplyColorTable(TexturePalette* src) {
    /*
     * FIXME:
     *
     * - Different palette formats (GL_RGB -> PVR_PAL_RGB565)
     */
    if(!src || !src->data) {
        return;
    }

    GLushort i;
    GLushort offset = src->size * src->bank;
    for(i = 0; i < src->width; ++i) {
        GLubyte* entry = &src->data[i * 4];
        if(INTERNAL_PALETTE_FORMAT == GL_RGBA8) {
            pvr_set_pal_entry(offset + i, PACK_ARGB8888(entry[3], entry[0], entry[1], entry[2]));
        } else {
            pvr_set_pal_entry(offset + i, PACK_ARGB4444(entry[3], entry[0], entry[1], entry[2]));
        }
    }
}

GLubyte _glGetActiveTexture() {
    return ACTIVE_TEXTURE;
}

 GLint _determineStride(GLenum format, GLenum type) {
    switch(type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return (format == GL_RED) ? 1 : (format == GL_RGB) ? 3 : 4;
    case GL_UNSIGNED_SHORT:
        return (format == GL_RED) ? 2 : (format == GL_RGB) ? 6 : 8;
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_5_6_5_TWID_KOS:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV_TWID_KOS:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV_TWID_KOS:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
        return 2;
    }

    return -1;
}

GLubyte* _glGetMipmapLocation(TextureObject* obj, GLuint level) {
    GLuint offset = 0;
    GLuint size = obj->height;
    if(obj->width == size){
    if(obj->isPaletted){

    switch(size >> level){
        case 1024:
        offset = 0x55558;
        break;
        case 512:
        offset = 0x15558;
        break;        
        case 256:
        offset = 0x05558;
        break;
        case 128:
        offset = 0x01558;
        break;
        case 64:
        offset = 0x00558;
        break;
        case 32:
        offset = 0x00158;
        break;
        case 16:
        offset = 0x00058;
        break;
        case 8:
        offset = 0x00018;
        break;
        case 4:
        offset = 0x00008;
        break;
        case 2:
        offset = 0x00004;
        break;
        case 1:
        offset = 0x00003;
        break;
    }
    } else if(obj->isCompressed) {

    switch(size >> level){
        case 1024:
        offset = 0x15556;
        break;
        case 512:
        offset = 0x05556;
        break;        
        case 256:
        offset = 0x01556;
        break;
        case 128:
        offset = 0x00556;
        break;
        case 64:
        offset = 0x00156;
        break;
        case 32:
        offset = 0x00056;
        break;
        case 16:
        offset = 0x00016;
        break;
        case 8:
        offset = 0x00006;
        break;
        case 4:
        offset = 0x00002;
        break;
        case 2:
        offset = 0x00001;
        break;
        case 1:
        offset = 0x00000;
        break;
    }
    }else {

    switch(size >> level){
        case 1024:
        offset = 0xAAAB0;
        break;
        case 512:
        offset = 0x2AAB0;
        break;
        case 256:
        offset = 0x0AAB0;
        break;
        case 128:
        offset = 0x02AB0;
        break;
        case 64:
        offset = 0x00AB0;
        break;
        case 32:
        offset = 0x002B0;
        break;
        case 16:
        offset = 0x000B0;
        break;
        case 8:
        offset = 0x00030;
        break;
        case 4:
        offset = 0x00010;
        break;
        case 2:
        offset = 0x00008;
        break;
        case 1:
        offset = 0x00006;
        break;
    }
    
    }
    }

    return ((GLubyte*) obj->data) + offset;
}

GLuint _glGetMipmapLevelCount(TextureObject* obj) {
    return 1 + floor(log2(MAX(obj->width, obj->height)));
}

static GLuint _glGetMipmapDataSize(TextureObject* obj) {
    GLuint size = 0;

    GLuint i = 0;
    GLuint width = obj->width;
    GLuint height = obj->height;

    for(; i < _glGetMipmapLevelCount(obj)+1; ++i) {
        size += (width * height * obj->dataStride);

        if(width > 1) {
            width /= 2;
        }

        if(height > 1) {
            height /= 2;
        }
    }

    return size;
}

GLubyte _glInitTextures() {
    named_array_init(&TEXTURE_OBJECTS, sizeof(TextureObject), MAX_TEXTURE_COUNT);

    // Reserve zero so that it is never given to anyone as an ID!
    named_array_reserve(&TEXTURE_OBJECTS, 0);

    SHARED_PALETTES[0] = _initTexturePalette();
    SHARED_PALETTES[1] = _initTexturePalette();
    SHARED_PALETTES[2] = _initTexturePalette();
    SHARED_PALETTES[3] = _initTexturePalette();

    memset((void*) BANKS_USED, 0x0, sizeof(BANKS_USED));
    memset((void*) SUBBANKS_USED, 0x0, sizeof(SUBBANKS_USED));
    return 1;
}

TextureObject* _glGetTexture0() {
    return TEXTURE_UNITS[0];
}

TextureObject* _glGetTexture1() {
    return TEXTURE_UNITS[1];
}

TextureObject* _glGetBoundTexture() {
    return TEXTURE_UNITS[ACTIVE_TEXTURE];
}

void APIENTRY glActiveTextureARB(GLenum texture) {
    TRACE();

    if(texture < GL_TEXTURE0_ARB || texture > GL_TEXTURE0_ARB + MAX_TEXTURE_UNITS)
        _glKosThrowError(GL_INVALID_ENUM, "glActiveTextureARB");

    if(_glKosHasError()) {
        _glKosPrintError();
        return;
    }

    ACTIVE_TEXTURE = texture & 0xF;
}

GLboolean APIENTRY glIsTexture(GLuint texture) {
    return (named_array_used(&TEXTURE_OBJECTS, texture)) ? GL_TRUE : GL_FALSE;
}

static void _glInitializeTextureObject(TextureObject* txr, unsigned int id) {
    txr->index = id;
    txr->width = txr->height = 0;
    txr->mipmap = 0;
    txr->uv_clamp = 0;
    txr->env = PVR_TXRENV_MODULATEALPHA;
    txr->data = NULL;
    txr->mipmapCount = 0;
    txr->minFilter = GL_NEAREST;
    txr->magFilter = GL_NEAREST;
    txr->palette = NULL;
    txr->isCompressed = GL_FALSE;
    txr->isPaletted = GL_FALSE;

    /* Always default to the first shared bank */
    txr->shared_bank = 0;
}

void APIENTRY glGenTextures(GLsizei n, GLuint *textures) {
    TRACE();

    while(n--) {
        GLuint id = 0;
        TextureObject* txr = (TextureObject*) named_array_alloc(&TEXTURE_OBJECTS, &id);

        assert(id);  // Generated IDs must never be zero

        _glInitializeTextureObject(txr, id);

        *textures = id;

        textures++;
    }
}

void APIENTRY glDeleteTextures(GLsizei n, GLuint *textures) {
    TRACE();

    while(n--) {
        TextureObject* txr = (TextureObject*) named_array_get(&TEXTURE_OBJECTS, *textures);

        /* Make sure we update framebuffer objects that have this texture attached */
        _glWipeTextureOnFramebuffers(*textures);

        if(txr == TEXTURE_UNITS[ACTIVE_TEXTURE]) {
            TEXTURE_UNITS[ACTIVE_TEXTURE] = NULL;
        }

        if(txr->data) {
            pvr_mem_free(txr->data);
            txr->data = NULL;
        }

        if(txr->palette && txr->palette->data) {
            free(txr->palette->data);
            txr->palette->data = NULL;
        }

        if(txr->palette) {
            free(txr->palette);
            txr->palette = NULL;
        }

        named_array_release(&TEXTURE_OBJECTS, *textures++);
    }
}

void APIENTRY glBindTexture(GLenum  target, GLuint texture) {
    TRACE();

    GLint target_values [] = {GL_TEXTURE_2D, 0};

    if(_glCheckValidEnum(target, target_values, __func__) != 0) {
        return;
    }

    if(texture) {
        /* If this didn't come from glGenTextures, then we should initialize the
         * texture the first time it's bound */
        if(!named_array_used(&TEXTURE_OBJECTS, texture)) {
            TextureObject* txr = named_array_reserve(&TEXTURE_OBJECTS, texture);
            _glInitializeTextureObject(txr, texture);
        }

        TEXTURE_UNITS[ACTIVE_TEXTURE] = (TextureObject*) named_array_get(&TEXTURE_OBJECTS, texture);
    } else {
        TEXTURE_UNITS[ACTIVE_TEXTURE] = NULL;
    }
}

void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param) {
    TRACE();

    GLubyte failures = 0;

    GLint target_values [] = {GL_TEXTURE_ENV, GL_TEXTURE_FILTER_CONTROL_EXT, 0};
    failures += _glCheckValidEnum(target, target_values, __func__);

    TextureObject* active = TEXTURE_UNITS[ACTIVE_TEXTURE];
    
    if(!active) {
        return;
    }

    if(failures) {
        return;
    }
    switch(target){
        case GL_TEXTURE_ENV:
            {
            GLint pname_values [] = {GL_TEXTURE_ENV_MODE, 0};
            GLint param_values [] = {GL_MODULATE, GL_DECAL, GL_REPLACE, 0};
            failures += _glCheckValidEnum(pname, pname_values, __func__);
            failures += _glCheckValidEnum(param, param_values, __func__);
            
            if(failures) {
                return;
            }

            switch(param) {
                case GL_MODULATE:
                    active->env = PVR_TXRENV_MODULATEALPHA;
                break;
                case GL_DECAL:
                    active->env = PVR_TXRENV_DECAL;
                break;
                case GL_REPLACE:
                    active->env = PVR_TXRENV_REPLACE;
                break;
                default:
                    break;
            }
            }
            break;

        case GL_TEXTURE_FILTER_CONTROL_EXT:
            {
            GLint pname_values [] = {GL_TEXTURE_LOD_BIAS_EXT, 0};
            failures += _glCheckValidEnum(pname, pname_values, __func__);
            failures += (param > GL_MAX_TEXTURE_LOD_BIAS_DEFAULT || param < -GL_MAX_TEXTURE_LOD_BIAS_DEFAULT); 
            if(failures) {
                return;
            }
            active->mipmap_bias = (GL_MAX_TEXTURE_LOD_BIAS_DEFAULT+1)+param; // bring to 1-15 inclusive
            }
            break;
        default:
           break;
    }
  
}

void APIENTRY glCompressedTexImage2DARB(GLenum target,
                                     GLint level,
                                     GLenum internalFormat,
                                     GLsizei width,
                                     GLsizei height,
                                     GLint border,
                                     GLsizei imageSize,
                                     const GLvoid *data) {
    TRACE();

    if(target != GL_TEXTURE_2D) {
        _glKosThrowError(GL_INVALID_ENUM, __func__);
    }

    GLint w = width;
    if(w < 8 || (w & -w) != w) {
        /* Width is not a power of two. Must be!*/
        _glKosThrowError(GL_INVALID_VALUE, __func__);
    }

    GLint h = height;
    if(h < 8 || (h & -h) != h) {
        /* Height is not a power of two. Must be!*/
        _glKosThrowError(GL_INVALID_VALUE, __func__);
    }

    if(level || border) {
        /* We don't support setting mipmap levels manually with compressed textures
           maybe one day */
        _glKosThrowError(GL_INVALID_VALUE, __func__);
    }

    GLboolean mipmapped = GL_FALSE;

    switch(internalFormat) {
        case GL_COMPRESSED_ARGB_1555_VQ_KOS:
        case GL_COMPRESSED_ARGB_1555_VQ_TWID_KOS:
        case GL_COMPRESSED_ARGB_4444_VQ_KOS:
        case GL_COMPRESSED_ARGB_4444_VQ_TWID_KOS:
        case GL_COMPRESSED_RGB_565_VQ_KOS:
        case GL_COMPRESSED_RGB_565_VQ_TWID_KOS:
        break;
        case GL_COMPRESSED_ARGB_1555_VQ_MIPMAP_KOS:
        case GL_COMPRESSED_ARGB_1555_VQ_MIPMAP_TWID_KOS:
        case GL_COMPRESSED_ARGB_4444_VQ_MIPMAP_KOS:
        case GL_COMPRESSED_ARGB_4444_VQ_MIPMAP_TWID_KOS:
        case GL_COMPRESSED_RGB_565_VQ_MIPMAP_KOS:
        case GL_COMPRESSED_RGB_565_VQ_MIPMAP_TWID_KOS:
            mipmapped = GL_TRUE;
        break;
        default:
        _glKosThrowError(GL_INVALID_OPERATION, __func__);
    }

    if(TEXTURE_UNITS[ACTIVE_TEXTURE] == NULL) {
        _glKosThrowError(GL_INVALID_OPERATION, __func__);
    }

    /* Guess whether we have mipmaps or not */

    /* `expected` is the uncompressed size */
    GLuint expected = sizeof(GLshort) * width * height;

    /* The ratio is the uncompressed vs compressed data size */
    GLuint ratio = (GLuint) (((GLfloat) expected) / ((GLfloat) imageSize));

    if(ratio < 7 && !mipmapped) {
        /* If the ratio is less than 1:7 then we assume that the reason for that
        is the extra data used for mipmaps. Testing shows that a single VQ compressed
        image is around 1:7 or 1:8. We may need to tweak this if it detects false positives */
        fprintf(stderr, "GL ERROR: Detected multiple mipmap levels being uploaded to %s\n", __func__);
        _glKosThrowError(GL_INVALID_OPERATION, __func__);
    }

    if(_glKosHasError()) {
        _glKosPrintError();
        return;
    }

    TextureObject* active = TEXTURE_UNITS[ACTIVE_TEXTURE];

    /* Set the required mipmap count */
    active->width   = width;
    active->height  = height;
    active->color   = _determinePVRFormat(
        internalFormat,
        internalFormat  /* Doesn't matter (see determinePVRFormat) */
    );
    active->mipmapCount = _glGetMipmapLevelCount(active);
    active->mipmap = (mipmapped) ? ~0 : (1 << level);  /* Set only a single bit if this wasn't mipmapped otherwise set all */
    active->isCompressed = GL_TRUE;

    /* Odds are slim new data is same size as old, so free always */
    if(active->data)
        pvr_mem_free(active->data);

    active->data = pvr_mem_malloc(imageSize);

    if(data)
        sq_cpy(active->data, data, imageSize);
}

GLint _cleanInternalFormat(GLint internalFormat) {
    switch (internalFormat) {
    case GL_COLOR_INDEX4_EXT:
        return GL_COLOR_INDEX4_EXT;
    case GL_COLOR_INDEX8_EXT:
        return GL_COLOR_INDEX8_EXT;
    case GL_ALPHA:
/*    case GL_ALPHA4:
    case GL_ALPHA8:
    case GL_ALPHA12:
    case GL_ALPHA16:*/
       return GL_ALPHA;
    case 1:
    case GL_LUMINANCE:
/*    case GL_LUMINANCE4:
    case GL_LUMINANCE8:
    case GL_LUMINANCE12:
    case GL_LUMINANCE16:*/
       return GL_LUMINANCE;
    case 2:
    case GL_LUMINANCE_ALPHA:
/*    case GL_LUMINANCE4_ALPHA4:
    case GL_LUMINANCE6_ALPHA2:
    case GL_LUMINANCE8_ALPHA8:
    case GL_LUMINANCE12_ALPHA4:
    case GL_LUMINANCE12_ALPHA12:
    case GL_LUMINANCE16_ALPHA16: */
       return GL_LUMINANCE_ALPHA;
/*    case GL_INTENSITY:
    case GL_INTENSITY4:
    case GL_INTENSITY8:
    case GL_INTENSITY12:
    case GL_INTENSITY16:
       return GL_INTENSITY; */
    case 3:
       return GL_RGB;
    case GL_RGB:
    /* case GL_R3_G3_B2: */
    case GL_RGB4:
    case GL_RGB5:
    case GL_RGB8:
    case GL_RGB10:
    case GL_RGB12:
    case GL_RGB16:
       return GL_RGB;
    case 4:
       return GL_RGBA;
    case GL_RGBA:
    case GL_RGBA2:
    case GL_RGBA4:
    case GL_RGB5_A1:
    case GL_RGBA8:
    case GL_RGB10_A2:
    case GL_RGBA12:
    case GL_RGBA16:
        return GL_RGBA;

    /* Support ARB_texture_rg */
    case GL_RED:
/*    case GL_R8:
    case GL_R16:
    case GL_RED:
    case GL_COMPRESSED_RED: */
        return GL_RED;
/*    case GL_RG:
    case GL_RG8:
    case GL_RG16:
    case GL_COMPRESSED_RG:
        return GL_RG;*/
    default:
        return -1;
    }
}

 GLuint _determinePVRFormat(GLint internalFormat, GLenum type) {
    /* Given a cleaned internalFormat, return the Dreamcast format
     * that can hold it
     */
    switch(internalFormat) {
        case GL_ALPHA:
        case GL_LUMINANCE:
        case GL_LUMINANCE_ALPHA:
        case GL_RGBA:
        /* OK so if we have something that requires alpha, we return 4444 unless
         * the type was already 1555 (1-bit alpha) in which case we return that
         */
            if(type == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
                return PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED;
            } else if(type == GL_UNSIGNED_SHORT_1_5_5_5_REV_TWID_KOS) {
                return PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_TWIDDLED;
            } else if(type == GL_UNSIGNED_SHORT_4_4_4_4_REV_TWID_KOS) {
                return PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_TWIDDLED;
            } else {
                return PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_NONTWIDDLED;
            }
        case GL_RED:
        case GL_RGB:
            /* No alpha? Return RGB565 which is the best we can do without using palettes */
            return PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED;
        /* Compressed and twiddled versions */
        case GL_UNSIGNED_SHORT_5_6_5_TWID_KOS:
            return PVR_TXRFMT_RGB565 | PVR_TXRFMT_TWIDDLED;
        case GL_UNSIGNED_SHORT_4_4_4_4_REV_TWID_KOS:
            return PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_TWIDDLED;
        case GL_UNSIGNED_SHORT_1_5_5_5_REV_TWID_KOS:
            return PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_TWIDDLED;
        case GL_COMPRESSED_RGB_565_VQ_KOS:
        case GL_COMPRESSED_RGB_565_VQ_MIPMAP_KOS:
            return PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED | PVR_TXRFMT_VQ_ENABLE;
        case GL_COMPRESSED_RGB_565_VQ_TWID_KOS:
        case GL_COMPRESSED_RGB_565_VQ_MIPMAP_TWID_KOS:
            return PVR_TXRFMT_RGB565 | PVR_TXRFMT_TWIDDLED | PVR_TXRFMT_VQ_ENABLE;
        case GL_COMPRESSED_ARGB_4444_VQ_TWID_KOS:
        case GL_COMPRESSED_ARGB_4444_VQ_MIPMAP_TWID_KOS:
            return PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_TWIDDLED | PVR_TXRFMT_VQ_ENABLE;
        case GL_COMPRESSED_ARGB_4444_VQ_KOS:
        case GL_COMPRESSED_ARGB_4444_VQ_MIPMAP_KOS:
            return PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_NONTWIDDLED | PVR_TXRFMT_VQ_ENABLE;
        case GL_COMPRESSED_ARGB_1555_VQ_KOS:
        case GL_COMPRESSED_ARGB_1555_VQ_MIPMAP_KOS:
            return PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED | PVR_TXRFMT_VQ_ENABLE;
        case GL_COMPRESSED_ARGB_1555_VQ_TWID_KOS:
        case GL_COMPRESSED_ARGB_1555_VQ_MIPMAP_TWID_KOS:
            return PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_TWIDDLED | PVR_TXRFMT_VQ_ENABLE;
        case GL_COLOR_INDEX8_EXT:
            return PVR_TXRFMT_PAL8BPP | PVR_TXRFMT_TWIDDLED;
        case GL_COLOR_INDEX4_EXT:
            return PVR_TXRFMT_PAL4BPP | PVR_TXRFMT_TWIDDLED;
        default:
            return 0;
    }
}


typedef void (*TextureConversionFunc)(const GLubyte*, GLubyte*);

static inline void _rgba8888_to_argb4444(const GLubyte* source, GLubyte* dest) {
    *((GLushort*) dest) = (source[3] & 0xF0) << 8 | (source[0] & 0xF0) << 4 | (source[1] & 0xF0) | (source[2] & 0xF0) >> 4;
}

static inline void _rgba8888_to_rgba8888(const GLubyte* source, GLubyte* dest) {
    /* Noop */
    GLubyte* dst = (GLubyte*) dest;
    dst[0] = source[0];
    dst[1] = source[1];
    dst[2] = source[2];
    dst[3] = source[3];
}

static inline void _rgba8888_to_rgb565(const GLubyte* source, GLubyte* dest) {
    *((GLushort*) dest) = ((source[0] & 0b11111000) << 8) | ((source[1] & 0b11111100) << 3) | (source[2] >> 3);
}

static inline void _rgb888_to_rgba8888(const GLubyte* source, GLubyte* dest) {
    /* Noop */
    GLubyte* dst = (GLubyte*) dest;
    dst[0] = source[0];
    dst[1] = source[1];
    dst[2] = source[2];
    dst[3] = 255;
}

static inline void _rgb888_to_rgb565(const GLubyte* source, GLubyte* dest) {
    *((GLushort*) dest) = ((source[0] & 0b11111000) << 8) | ((source[1] & 0b11111100) << 3) | (source[2] >> 3);
}

static inline void _rgba8888_to_a000(const GLubyte* source, GLubyte* dest) {
    *((GLushort*) dest) = ((source[3] & 0b11111000) << 8);
}

static inline void _r8_to_rgb565(const GLubyte* source, GLubyte* dest) {
    *((GLushort*) dest) = (source[0] & 0b11111000) << 8;
}

static inline void _rgba4444_to_argb4444(const GLubyte* source, GLubyte* dest) {
    GLushort* src = (GLushort*) source;
    *((GLushort*) dest) = ((*src & 0x000F) << 12) | *src >> 4;
}

static inline void _rgba4444_to_rgba8888(const GLubyte* source, GLubyte* dest) {
    GLushort src = *((GLushort*) source);
    GLubyte* dst = (GLubyte*) dest;

    dst[0] = ((src & 0xF000) >> 12) * 2;
    dst[1] = ((src & 0x0F00) >> 8) * 2;
    dst[2] = ((src & 0x00F0) >> 4) * 2;
    dst[3] = ((src & 0x000F)) * 2;
}

static inline void _i8_to_i8(const GLubyte* source, GLubyte* dest) {
    /* For indexes */
    GLubyte* dst = (GLubyte*) dest;
    *dst = *source;
}

static TextureConversionFunc _determineConversion(GLint internalFormat, GLenum format, GLenum type) {
    switch(internalFormat) {
    case GL_ALPHA: {
        if(type == GL_UNSIGNED_BYTE && format == GL_RGBA) {
            return _rgba8888_to_a000;
        } else if(type == GL_BYTE && format == GL_RGBA) {
            return _rgba8888_to_a000;
        }
    } break;
    case GL_RED: {
        if(type == GL_UNSIGNED_BYTE && format == GL_RED) {
            /* Dreamcast doesn't really support GL_RED internally, so store as rgb */
            return _r8_to_rgb565;
        }
    } break;
    case GL_RGB: {
        if(type == GL_UNSIGNED_BYTE && format == GL_RGB) {
            return _rgb888_to_rgb565;
        } else if(type == GL_UNSIGNED_BYTE && format == GL_RGBA) {
            return _rgba8888_to_rgb565;
        } else if(type == GL_BYTE && format == GL_RGB) {
            return _rgb888_to_rgb565;
        } else if(type == GL_UNSIGNED_BYTE && format == GL_RED) {
            return _r8_to_rgb565;
        }
    } break;
    case GL_RGBA: {
        if(type == GL_UNSIGNED_BYTE && format == GL_RGBA) {
            return _rgba8888_to_argb4444;
        } else if (type == GL_BYTE && format == GL_RGBA) {
            return _rgba8888_to_argb4444;
        } else if(type == GL_UNSIGNED_SHORT_4_4_4_4 && format == GL_RGBA) {
            return _rgba4444_to_argb4444;
        }
    } break;
    case GL_RGBA8: {
        if(type == GL_UNSIGNED_BYTE && format == GL_RGBA) {
            return _rgba8888_to_rgba8888;
        } else if (type == GL_BYTE && format == GL_RGBA) {
            return _rgba8888_to_rgba8888;
        } else if(type == GL_UNSIGNED_BYTE && format == GL_RGB) {
            return _rgb888_to_rgba8888;
        } else if (type == GL_BYTE && format == GL_RGB) {
            return _rgb888_to_rgba8888;
        } else if(type == GL_UNSIGNED_SHORT_4_4_4_4 && format == GL_RGBA) {
            return _rgba4444_to_rgba8888;
        }
    } break;
    case GL_COLOR_INDEX8_EXT:
        if(format == GL_COLOR_INDEX) {
            switch(type) {
                case GL_BYTE:
                case GL_UNSIGNED_BYTE:
                    return _i8_to_i8;
                default:
                    break;
            }
        }
    break;
    default:
        fprintf(stderr, "Unsupported conversion: %x -> %x, %x\n", internalFormat, format, type);
        break;
    }
    return 0;
}

static GLboolean _isSupportedFormat(GLenum format) {
    switch(format) {
    case GL_RED:
    case GL_RGB:
    case GL_RGBA:
    case GL_BGRA:
    case GL_COLOR_INDEX:
        return GL_TRUE;
    default:
        return GL_FALSE;
    }
}

GLboolean _glIsMipmapComplete(const TextureObject* obj) {
    if(!obj->mipmap || !obj->mipmapCount) {
        return GL_FALSE;
    }
    
    GLsizei i = 0;
    for(; i < (GLubyte)(obj->mipmapCount-3); ++i) {
        if((obj->mipmap & (1 << i)) == 0) {
            return GL_FALSE;
        }
    }

    return GL_TRUE;
}

#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)| \
                     ((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9) )

#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )

void _glAllocateSpaceForMipmaps(TextureObject* active) {
    if(!active->data){
        return;
    }
    if(active->data && active->mipmap > 1) {
        /* Already done */
        return;
    }

    int size = active->height*active->width*active->dataStride;

    /* We've allocated level 0 before, but now we're allocating
     * a level beyond that, we need to reallocate the data, copy level 0
     * then free the original
    */
    GLubyte* temp = malloc(size);
    memcpy4(temp, active->data, size);
    pvr_mem_free(active->data);
    active->data = pvr_mem_malloc(_glGetMipmapDataSize(active));

    /* If there was existing data, then copy it where it should go */
    memcpy4(_glGetMipmapLocation(active,0), temp, size);
    free(temp);
}

void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalFormat,
                           GLsizei width, GLsizei height, GLint border,
                           GLenum format, GLenum type, const GLvoid *data) {

    TRACE();
    if(!data) {
        /* No data? Do nothing! */
        return;
    }

    if(target != GL_TEXTURE_2D) {
        _glKosThrowError(GL_INVALID_ENUM, "glTexImage2D");
    }

    if(format != GL_COLOR_INDEX) {
        if(!_isSupportedFormat(format)) {
            _glKosThrowError(GL_INVALID_ENUM, "glTexImage2D");
        }

        /* Abuse determineStride to see if type is valid */
        if(_determineStride(GL_RGBA, type) == -1) {
            _glKosThrowError(GL_INVALID_ENUM, "glTexImage2D");
        }

        internalFormat = _cleanInternalFormat(internalFormat);
        if(internalFormat == -1) {
            _glKosThrowError(GL_INVALID_VALUE, "glTexImage2D");
        }
    } else {
        if(internalFormat != GL_COLOR_INDEX8_EXT) {
            _glKosThrowError(GL_INVALID_ENUM, __func__);
        }
    }

    GLuint w = width;
    GLuint h = height;
    if(level == 0){
    if((w < 8 || (w & -w) != w)) {
        /* Width is not a power of two. Must be!*/
        _glKosThrowError(GL_INVALID_VALUE, "glTexImage2D");
    }

    
    if((h < 8 || (h & -h) != h)) {
        /* height is not a power of two. Must be!*/
        _glKosThrowError(GL_INVALID_VALUE, "glTexImage2D");
    }
    }

    if(level < 0) {
        _glKosThrowError(GL_INVALID_VALUE, "glTexImage2D");
    }

    if(border) {
        _glKosThrowError(GL_INVALID_VALUE, "glTexImage2D");
    }

    if(!TEXTURE_UNITS[ACTIVE_TEXTURE]) {
        _glKosThrowError(GL_INVALID_OPERATION, __func__);
    }

    GLboolean isPaletted = (internalFormat == GL_COLOR_INDEX8_EXT) ? GL_TRUE : GL_FALSE;

    if(_glKosHasError()) {
        _glKosPrintError();
        return;
    }

    /* Calculate the format that we need to convert the data to */
    GLuint pvr_format = _determinePVRFormat(internalFormat, type);

    TextureObject* active = TEXTURE_UNITS[ACTIVE_TEXTURE];

    assert(active);

    if(active->data && (level == 0) ) {
        /* pre-existing texture - check if changed */
        if(active->width != width ||
           active->height != height ||
           active->color != pvr_format) {
            /* changed - free old texture memory */
            pvr_mem_free(active->data);
            active->data = NULL;
            active->mipmap = 0;
            active->mipmapCount = 0;
            active->mipmap_bias = GL_KOS_INTERNAL_DEFAULT_MIPMAP_LOD_BIAS; // in the scale of -8 - 8 moved to 1-15, -4 = 4 
            active->dataStride = 0;
        }
    }

    /* All colour formats are represented as shorts internally. Paletted textures
     * are represented by byte indexes (which look up into a color table)
     */
    GLint destStride = isPaletted ? 1 : 2;
    GLuint bytes = (width * height * destStride);

    if(!active->data && (level == 0)) {
        assert(active);
        assert(width);
        assert(height);
        assert(destStride);

        /* need texture memory */
        active->width   = width;
        active->height  = height;
        active->color   = pvr_format;
        /* Set the required mipmap count */
        active->mipmapCount = _glGetMipmapLevelCount(active);
        active->mipmap_bias = GL_KOS_INTERNAL_DEFAULT_MIPMAP_LOD_BIAS; // in the scale of -8 - 8 moved to 1-15, -4 = 4 

        active->dataStride = destStride;

        active->data = pvr_mem_malloc(destStride*width*height);
        assert(active->data);

        active->isCompressed = GL_FALSE;
        active->isPaletted = isPaletted;
    } else if(!active->data && (level != 0)){
        printf("Spooky!\n");
        assert(0 && "GLdc broken, sorry! @neo");
        return;
    }

    /* Check if we need to re-alloc and move texture data */
    if(level > 0){
       _glAllocateSpaceForMipmaps(active);
    }

    /* Mark this level as set in the mipmap bitmask */
    active->mipmap |= (1 << level);

    /* Let's assume we need to convert */
    GLboolean needsConversion = GL_TRUE;
    GLboolean needsTwiddling = GL_FALSE;

    /*
     * These are the only formats where the source format passed in matches the pvr format.
     * Note the REV formats + GL_BGRA will reverse to ARGB which is what the PVR supports
     */
    if(format == GL_COLOR_INDEX) {
        /* Don't convert color indexes */
        needsConversion = GL_FALSE;
        needsTwiddling = type == GL_UNSIGNED_BYTE;
    } else if(format == GL_BGRA && type == GL_UNSIGNED_SHORT_4_4_4_4_REV && internalFormat == GL_RGBA) {
        needsConversion = GL_FALSE;
    } else if(format == GL_BGRA && type == GL_UNSIGNED_SHORT_1_5_5_5_REV && internalFormat == GL_RGBA) {
        needsConversion = GL_FALSE;
    } else if(format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5 && internalFormat == GL_RGB) {
        needsConversion = GL_FALSE;
    } else if(format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5_TWID_KOS && internalFormat == GL_RGB) {
        needsConversion = GL_FALSE;
    } else if(format == GL_BGRA && type == GL_UNSIGNED_SHORT_1_5_5_5_REV_TWID_KOS && internalFormat == GL_RGBA) {
        needsConversion = GL_FALSE;
    } else if(format == GL_BGRA && type == GL_UNSIGNED_SHORT_4_4_4_4_REV_TWID_KOS && internalFormat == GL_RGBA) {
        needsConversion = GL_FALSE;
    }

    GLubyte* targetData = NULL;

    if(active->mipmap == 1 && level==0){
        targetData = active->data;
    } else {
        targetData = _glGetMipmapLocation(active, level);
    }
    
    assert(targetData);

    if(!needsConversion) {
        assert(targetData);
        assert(data);
        assert(bytes);

        if(needsTwiddling) {
            assert(type == GL_UNSIGNED_BYTE);  // Anything else needs this loop adjusting
            GLuint x, y, min, mask;

            GLubyte *pixels   = (GLubyte*) data;
            GLushort *vtex    = (GLushort*) targetData;

            min = MIN(w, h);
            mask = min - 1;

            if(height == 1 || width == 1){
                targetData[0] = ((GLubyte*)data)[0];
                targetData[1] = ((GLubyte*)data)[0];
                targetData[2] = ((GLubyte*)data)[0];
                targetData[3] = ((GLubyte*)data)[0];
            } else {
            for(y = 0; y < h; y += 2) {
                for(x = 0; x < w; x++) {
                    vtex[TWIDOUT((y & mask) / 2, x & mask) + (x / min + y / min)*min * min / 2] = pixels[y * w + x] | (pixels[(y + 1) * w + x] << 8);
                }
            }
            }
        } else {
            /* No conversion? Just copy the data, and the pvr_format is correct */
            FASTCPY(targetData, data, bytes);
        }

        return;
    } else {
        TextureConversionFunc convert = _determineConversion(
            internalFormat,
            format,
            type
        );

        if(!convert) {
            _glKosThrowError(GL_INVALID_OPERATION, __func__);
            return;
        }

        GLubyte* dest = (GLubyte*) targetData;
        const GLubyte* source = data;

        assert(dest);
        assert(source);

        GLint stride = _determineStride(format, type);
        assert(stride > -1);

        if(stride == -1) {
            _glKosThrowError(GL_INVALID_OPERATION, __func__);
            return;
        }

        /* Perform the conversion */
        GLuint i;
        for(i = 0; i < bytes; i += destStride) {
            convert(source, dest);

            dest += destStride;
            source += stride;
        }
    }
}

void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param) {
    TRACE();

    TextureObject* active = _glGetBoundTexture();

    if(!active) {
        return;
    }

    if(target == GL_TEXTURE_2D) {
        switch(pname) {
            case GL_TEXTURE_MAG_FILTER:
                switch(param) {
                    case GL_NEAREST:
                    case GL_LINEAR:
                    break;
                    default: {
                        _glKosThrowError(GL_INVALID_VALUE, __func__);
                        _glKosPrintError();
                        return;
                    }
                }
                active->magFilter = param;
            break;
            case GL_TEXTURE_MIN_FILTER:
                switch(param) {
                    case GL_NEAREST:
                    case GL_LINEAR:
                    case GL_NEAREST_MIPMAP_LINEAR:
                    case GL_NEAREST_MIPMAP_NEAREST:
                    case GL_LINEAR_MIPMAP_LINEAR:
                    case GL_LINEAR_MIPMAP_NEAREST:
                    break;
                    default: {
                        _glKosThrowError(GL_INVALID_VALUE, __func__);
                        _glKosPrintError();
                        return;
                    }
                }
                active->minFilter = param;
            break;
            case GL_TEXTURE_WRAP_S:
                switch(param) {
                    case GL_CLAMP:
                        active->uv_clamp |= CLAMP_U;
                        break;

                    case GL_REPEAT:
                        active->uv_clamp &= ~CLAMP_U;
                        break;
                }

                break;

            case GL_TEXTURE_WRAP_T:
                switch(param) {
                    case GL_CLAMP:
                        active->uv_clamp |= CLAMP_V;
                        break;

                    case GL_REPEAT:
                        active->uv_clamp &= ~CLAMP_V;
                        break;
                }

                break;
            case GL_SHARED_TEXTURE_BANK_KOS:
                active->shared_bank = param;
                break;
            default:
                break;
        }
    }
}

void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLint param) {
    glTexParameteri(target, pname, (GLint) param);
}

GLAPI void APIENTRY glColorTableEXT(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *data) {
    GLint validTargets[] = {
        GL_TEXTURE_2D,
        GL_SHARED_TEXTURE_PALETTE_EXT,
        GL_SHARED_TEXTURE_PALETTE_0_KOS,
        GL_SHARED_TEXTURE_PALETTE_1_KOS,
        GL_SHARED_TEXTURE_PALETTE_2_KOS,
        GL_SHARED_TEXTURE_PALETTE_3_KOS,
        0
    };

    GLint validInternalFormats[] = {GL_RGB8, GL_RGBA8, 0};
    GLint validFormats[] = {GL_RGB, GL_RGBA, 0};
    GLint validTypes[] = {GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT, 0};

    if(_glCheckValidEnum(target, validTargets, __func__) != 0) {
        return;
    }

    if(_glCheckValidEnum(internalFormat, validInternalFormats, __func__) != 0) {
        return;
    }

    if(_glCheckValidEnum(format, validFormats, __func__) != 0) {
        return;
    }

    if(_glCheckValidEnum(type, validTypes, __func__) != 0) {
        return;
    }

    /* Only allow up to 256 colours in a palette */
    if(width > 256 || width == 0) {
        _glKosThrowError(GL_INVALID_VALUE, __func__);
        _glKosPrintError();
        return;
    }

    GLint sourceStride = _determineStride(format, type);

    assert(sourceStride > -1);

    TextureConversionFunc convert = _determineConversion(
        GL_RGBA8,  /* We always store palettes in this format */
        format,
        type
    );

    if(!convert) {
        _glKosThrowError(GL_INVALID_OPERATION, __func__);
        _glKosPrintError();
        return;
    }

    TexturePalette* palette = NULL;

    /* Custom extension - allow uploading to one of 4 custom palettes */
    if(target == GL_SHARED_TEXTURE_PALETTE_EXT || target == GL_SHARED_TEXTURE_PALETTE_0_KOS) {
        palette = SHARED_PALETTES[0];
    } else if(target == GL_SHARED_TEXTURE_PALETTE_1_KOS) {
        palette = SHARED_PALETTES[1];
    } else if(target == GL_SHARED_TEXTURE_PALETTE_2_KOS) {
        palette = SHARED_PALETTES[2];
    } else if(target == GL_SHARED_TEXTURE_PALETTE_3_KOS) {
        palette = SHARED_PALETTES[3];
    } else {
        TextureObject* active = _glGetBoundTexture();
        if(!active->palette) {
            active->palette = _initTexturePalette();
        }

        palette = active->palette;
    }

    assert(palette);

    if(target) {
        free(palette->data);
        palette->data = NULL;
    }

    if(palette->bank > -1) {
        _glReleasePaletteSlot(palette->bank, palette->size);
        palette->bank = -1;
    }

    palette->data = (GLubyte*) malloc(width * 4);
    palette->format = format;
    palette->width = width;
    palette->size = (width > 16) ? 256 : 16;
    assert(palette->size == 16 || palette->size == 256);

    palette->bank = _glGenPaletteSlot(palette->size);

    if(palette->bank < 0) {
        /* We ran out of slots! */
        _glKosThrowError(GL_INVALID_OPERATION, __func__);
        _glKosPrintError();

        free(palette->data);
        palette->format = palette->width = palette->size = 0;
        return;
    }

    GLubyte* src = (GLubyte*) data;
    GLubyte* dst = (GLubyte*) palette->data;

    assert(src);
    assert(dst);

    /* Transform and copy the source palette to the texture */
    GLushort i = 0;
    for(; i < width; ++i) {
        convert(src, dst);

        src += sourceStride;
        dst += 4;
    }

    _glApplyColorTable(palette);
}

GLAPI void APIENTRY glColorSubTableEXT(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data) {
    (void) target;
    (void) start;
    (void) count;
    (void) format;
    (void) type;
    (void) data;
    _glKosThrowError(GL_INVALID_OPERATION, __func__);
    _glKosPrintError();
}

GLAPI void APIENTRY glGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid *data) {
    (void) target;
    (void) format;
    (void) type;
    (void) data;
    _glKosThrowError(GL_INVALID_OPERATION, __func__);
    _glKosPrintError();
}

GLAPI void APIENTRY glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params) {
    (void) target;
    (void) pname;
    (void) params;
    _glKosThrowError(GL_INVALID_OPERATION, __func__);
    _glKosPrintError();
}

GLAPI void APIENTRY glGetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat *params) {
    (void) target;
    (void) pname;
    (void) params;
    _glKosThrowError(GL_INVALID_OPERATION, __func__);
    _glKosPrintError();
}

GLAPI void APIENTRY glTexSubImage2D(
    GLenum target, GLint level, GLint xoffset, GLint yoffset,
    GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    (void) target;
    (void) level;
    (void) xoffset;
    (void) yoffset;
    (void) width;    
    (void) height;      
    (void) format;      
    (void) type;      
    (void) pixels;      
    assert(0 && "Not Implemented");
}

GLAPI void APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    (void) target;
    (void) level;
    (void) xoffset;
    (void) yoffset;
    (void) x;    
    (void) y;    
    (void) width;    
    (void) height;      
    assert(0 && "Not Implemented");
}

GLAPI void APIENTRY glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
    (void) target;
    (void) level;
    (void) xoffset;
    (void) x;
    (void) y;
    (void) width; 
    assert(0 && "Not Implemented");
}

GLAPI void APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    (void) target;
    (void) level;
    (void) internalformat;
    (void) x;    
    (void) y;    
    (void) width;    
    (void) height;    
    (void) border;    
    assert(0 && "Not Implemented");
}

GLAPI void APIENTRY glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {
    (void) target;
    (void) level;
    (void) internalformat;
    (void) x;    
    (void) y;    
    (void) width;    
    (void) border;    
    assert(0 && "Not Implemented");
}

GLAPI void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
    (void) x;    
    (void) y;    
    (void) width;    
    (void) height;    
    (void) format;    
    (void) type;    
    (void) pixels;    
    assert(0 && "Not Implemented");
}

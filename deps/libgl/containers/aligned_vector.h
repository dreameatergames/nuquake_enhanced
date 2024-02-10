#ifndef ALIGNED_VECTOR_H
#define ALIGNED_VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../GL/cygprofile.h"

typedef struct {
    unsigned int size;
    unsigned int capacity;
    unsigned char* data;
    unsigned int element_size;
} AlignedVector;

#define ALIGNED_VECTOR_CHUNK_SIZE 256u

void aligned_vector_init(AlignedVector* vector, unsigned int element_size);
void aligned_vector_reserve(AlignedVector* vector, unsigned int element_count);
void* aligned_vector_push_back(AlignedVector* vector, const void* objs, unsigned int count);
void* aligned_vector_resize(AlignedVector* vector, const unsigned int element_count);
INLINE_ALWAYS void* aligned_vector_at(const AlignedVector* vector, const unsigned int index) {
    #if 0
    if(index >= vector->size){
        char msg[60];
        sprintf(msg, "Vector OOB: %d %d wanted %d\n", vector->capacity, vector->size, index);
        //aligned_vector_resize(vector, index);
        assert_msg(index < vector->size, msg);
    }
    assert(index < vector->size); /* Check here */
    #endif
    return &vector->data[index * vector->element_size];
}
void* aligned_vector_extend(AlignedVector* vector, const unsigned int additional_count);
INLINE_ALWAYS void aligned_vector_clear(AlignedVector* vector){
    vector->size = 0;
}
void aligned_vector_shrink_to_fit(AlignedVector* vector);
void aligned_vector_cleanup(AlignedVector* vector);
INLINE_ALWAYS void* aligned_vector_back(AlignedVector* vector){
    return aligned_vector_at(vector, vector->size - 1);
}

#ifdef __cplusplus
}
#endif

#endif // ALIGNED_VECTOR_H

#ifndef SORT_H
#define SORT_H

#include "box.h" // I do not know why it can not put in the extern c

typedef struct{
    int x1, y1, x2, y2;
    float prob;
} person_det;

typedef struct{
    int id;
    int x1, y1, x2, y2;
} person_sort_det;

typedef struct{
    int id;
    int x, y;
} person_sort_compare_det;

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "utils.h"
#include "blas.h"
#include "cuda.h"
#include "image.h"

// in image.c
image get_label(image **characters, char *string, int size);

void draw_detections_with_sort_id(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes);
void sort_init();
int sort_update(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes);
void sort_cleanUp();

#ifdef __cplusplus
}
#endif

#endif // SORT_H
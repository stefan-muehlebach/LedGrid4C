#define _GNU_SOURCE

#include "PiPack2.h"
 #include <wiringPi.h>
 #include <wiringPiSPI.h>
// #include <softPwm.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>

/*
 * LedStrip --
 */

#define LEDSTRIP_MAXLENGTH     100
#define PIPACK_SPI_CHANNEL       0
#define PIPACK_SPI_SPEED   4000000

struct LedStrip {
    int fd;
    int size;
    unsigned char *array, *output, *map;
};

LedStrip LedStrip_Init (int size, char *colorMapFile) {
    LedStrip ls;
    FILE *fd;
    int a, b;

    assert ((size > 0) && (size <= LEDSTRIP_MAXLENGTH));

    ls = malloc (sizeof (*ls));
    ls->fd = wiringPiSPISetup(PIPACK_SPI_CHANNEL, PIPACK_SPI_SPEED);
    if (ls->fd < 0) {
        fprintf(stderr, "Can't open the SPI bus: %s\n", strerror(errno));
        exit(1);
    }
//    ls->fd = open ("/dev/spidev0.0", O_WRONLY | O_DSYNC);
    ls->size = size;
    ls->array = calloc (size, 3 * sizeof (unsigned char));
    ls->output = calloc (size, 3 * sizeof (unsigned char));
    ls->map = calloc (256, sizeof (unsigned char));

    fd = fopen (colorMapFile, "r");
    while (fscanf (fd, "%d %d", &a, &b) == 2) {
        ls->map[a] = b;
    }
    fclose (fd);

    return ls;
}

void LedStrip_Free (LedStrip ls) {
    assert (ls != NULL);

    free (ls->array);
    free (ls->output);
    free (ls);
}

void LedStrip_Show (LedStrip ls) {
    int i;

    assert (ls != NULL);

    for (i=0; i<3*ls->size; i++) {
        ls->output[i] = ls->map[ls->array[i]];
    }
    write (ls->fd, ls->output, 3 * ls->size);
    fsync (ls->fd);
}

void LedStrip_SetColor (LedStrip ls, int pixel,
        unsigned char red, unsigned char green, unsigned char blue) {
    assert (ls != NULL);
    assert (pixel < ls->size);

    ls->array[3 * pixel + 0] = red;
    ls->array[3 * pixel + 1] = green;
    ls->array[3 * pixel + 2] = blue;
}

void LedStrip_SetColorValue (LedStrip ls, int pixel,
        enum LedStrip_ColorIndexEnum colorIndex, unsigned char value) {
    ls->array[3 * pixel + colorIndex] = value;
}

void LedStrip_SetColorInt (LedStrip ls, int pixel, unsigned int value) {
    LedStrip_SetColorValue (ls, pixel, RED, (value>>16)&0xFF);
    LedStrip_SetColorValue (ls, pixel, GREEN, (value>>8)&0xFF);
    LedStrip_SetColorValue (ls, pixel, BLUE, value&0xFF);
}

void LedStrip_SetRed (LedStrip ls, int pixel, unsigned char red) {
    ls->array[3 * pixel + 0] = red;
}

void LedStrip_SetGreen (LedStrip ls, int pixel, unsigned char green) {
    ls->array[3 * pixel + 1] = green;
}

void LedStrip_SetBlue (LedStrip ls, int pixel, unsigned char blue) {
    ls->array[3 * pixel + 2] = blue;
}

unsigned char LedStrip_GetColorValue (LedStrip ls, int pixel,
        enum LedStrip_ColorIndexEnum colorIndex) {
    return ls->array[3 * pixel + colorIndex];
}

unsigned int LedStrip_GetColorInt (LedStrip ls, int pixel) {
    return ls->array[3 * pixel];
}

unsigned char LedStrip_GetRed (LedStrip ls, int pixel) {
    return ls->array[3 * pixel + 0];
}

unsigned char LedStrip_GetGreen (LedStrip ls, int pixel) {
    return ls->array[3 * pixel + 1];
}

unsigned char LedStrip_GetBlue (LedStrip ls, int pixel) {
    return ls->array[3 * pixel + 2];
}

void LedStrip_Clear (LedStrip ls) {
    int pixel, k;

    assert (ls != NULL);

    for (pixel=0; pixel<ls->size; pixel++) {
        LedStrip_SetColorInt (ls, pixel, 0);
    }
}

/*
 * LedGrid --
 */

struct LedGrid {
    int sizeX, sizeY;
    unsigned char **field;
    LedStrip ls;
};

LedGrid LedGrid_Init (int sizeX, int sizeY, char *colorMapFile) {
    LedGrid lg;
    int y;

    lg = malloc (sizeof (*lg));
    lg->sizeX = sizeX;
    lg->sizeY = sizeY;

    lg->field = calloc (lg->sizeY, sizeof (unsigned char **));
    for (y=0; y<sizeY; y++) {
        lg->field[y] = calloc (lg->sizeX, 3 * sizeof (unsigned char));
    }
    lg->ls = LedStrip_Init (LEDSTRIP_MAXLENGTH, colorMapFile);
     
    return lg;
}

void LedGrid_Free (LedGrid lg) {
    int y;

    assert (lg != NULL);

    for (y=0; y<lg->sizeY; y++) {
        free (lg->field[y]);
    }
    free (lg->field);
    LedStrip_Free (lg->ls);

    free (lg);
}

void LedGrid_Show (LedGrid lg) {
    int x, y, p, k;

    assert (lg != NULL);

    k = 0;
    for (y=0; y<lg->sizeY; y++) {
        if (y%2 == 0) {
            for (x=0; x<lg->sizeX; x++) {
                for (p=0; p<3; p++) {
                    lg->ls->array[k++] = lg->field[y][3*x+p];
                }
            }
        } else {
            for (x=lg->sizeX-1; x>=0; x--) {
                for (p=0; p<3; p++) {
                    lg->ls->array[k++] = lg->field[y][3*x+p];
                }
            }
        }
    }
    LedStrip_Show (lg->ls);
}

void LedGrid_SetColor (LedGrid lg, int x, int y, unsigned char red,
        unsigned char green, unsigned char blue) {
    LedGrid_SetColorValue (lg, x, y, RED, red);
    LedGrid_SetColorValue (lg, x, y, GREEN, green);
    LedGrid_SetColorValue (lg, x, y, BLUE, blue);
}

void LedGrid_SetColorValue (LedGrid lg, int x, int y,
        enum LedStrip_ColorIndexEnum colorIndex, unsigned char value) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));
    assert ((colorIndex >= RED) && (colorIndex <= BLUE));
    assert ((value >= 0) && (value < 256));

    lg->field[y][3*x+colorIndex] = value;
}

void LedGrid_SetColorInt (LedGrid lg, int x, int y, unsigned int value) {
    LedGrid_SetColorValue (lg, x, y, RED, (value>>16)&0xFF);
    LedGrid_SetColorValue (lg, x, y, GREEN, (value>>8)&0xFF);
    LedGrid_SetColorValue (lg, x, y, BLUE, value&0xFF);
}

void LedGrid_SetRed (LedGrid lg, int x, int y, unsigned char value) {
    LedGrid_SetColorValue (lg, x, y, RED, value);
}

void LedGrid_SetGreen (LedGrid lg, int x, int y, unsigned char value) {
    LedGrid_SetColorValue (lg, x, y, GREEN, value);
}

void LedGrid_SetBlue (LedGrid lg, int x, int y, unsigned char value) {
    LedGrid_SetColorValue (lg, x, y, BLUE, value);
}

unsigned char LedGrid_GetColorValue (LedGrid lg, int x, int y,
        enum LedStrip_ColorIndexEnum colorIndex) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));
    assert ((colorIndex >= RED) && (colorIndex <= BLUE));

    return lg->field[y][3 * x + colorIndex];
}

unsigned int LedGrid_GetColorInt (LedGrid lg, int x, int y) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));

    return lg->field[y][3 * x];
}

unsigned char LedGrid_GetRed (LedGrid lg, int x, int y) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));

    return lg->field[y][3 * x + RED];
}

unsigned char LedGrid_GetGreen (LedGrid lg, int x, int y) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));

    return lg->field[y][3 * x + GREEN];
}

unsigned char LedGrid_GetBlue (LedGrid lg, int x, int y) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));

    return lg->field[y][3 * x + BLUE];
}

void LedGrid_Clear (LedGrid lg) {
    int curIndex;
    int x, y, k;

    assert (lg != NULL);

    for (y=0; y<lg->sizeY; y++) {
        for (x=0; x<lg->sizeX; x++) {
            for (k=0; k<3; k++) {
                lg->field[y][3*x+k] = 0;
            }
        }
    }
}


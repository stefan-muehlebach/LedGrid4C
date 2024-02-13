#define _GNU_SOURCE

#include "LedGrid.h"
#include <wiringPi.h>
#include <softPwm.h>
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

#define COORD2PIXEL(lg,col,row) \
    (row%2)?((row+1)*lg->nCols-1-col):(row*lg->nCols+col)

struct LedGrid {
    int nCols, nRows, nPixels;
    unsigned char *strip, *out;
    unsigned char *gamma;
    int fd;
    Palette p;
};

struct Palette {
    unsigned char *pal;
};

/*-----------------------------------------------------------------------------
 *
 * LedGrid --
 *
 */
LedGrid LedGrid_Init (int nCols, int nRows) {
    LedGrid lg;
    int i;

    lg = malloc (sizeof (*lg));
    lg->nCols = nCols;
    lg->nRows = nRows;
    lg->nPixels = nCols * nRows;

    lg->strip = calloc (lg->nPixels, 3 * sizeof (unsigned char));
    lg->out   = calloc (lg->nPixels, 3 * sizeof (unsigned char));

    lg->gamma = calloc (256, sizeof (unsigned char));
    LedGrid_SetGamma (lg, 1.0);

    lg->fd = open ("/dev/spidev0.0", O_WRONLY | O_DSYNC);

    return lg;
}

void LedGrid_Free (LedGrid lg) {
    assert (lg != NULL);

    close (lg->fd);
    free (lg->strip);
    free (lg->out);
    free (lg->gamma);
    free (lg);
}

void LedGrid_Show (LedGrid lg) {
    int i;

    assert (lg != NULL);

    for (i=0; i<3*lg->nPixels; i+=3) {
        lg->out[i+0] = lg->gamma[lg->strip[i+0]];
        lg->out[i+1] = lg->gamma[lg->strip[i+1]];
        lg->out[i+2] = lg->gamma[lg->strip[i+2]];
/*
        lg->out[i+0] = lg->strip[i+0]/1.1;
        lg->out[i+1] = lg->strip[i+1]/1.1;
        lg->out[i+2] = lg->strip[i+2]/1.3;
        lg->out[i+0] = lg->gamma[lg->out[i+0]];
        lg->out[i+1] = lg->gamma[lg->out[i+1]];
        lg->out[i+2] = lg->gamma[lg->out[i+2]];
*/
    }
    write (lg->fd, lg->out, 3*lg->nPixels);
    fsync (lg->fd);
}

void LedGrid_SetGamma (LedGrid lg, float gamma) {
    int i;

    assert (lg != NULL);
    assert ((gamma >= 1.0) && (gamma <= 3.0));

    for (i=0; i<256; i++) {
        lg->gamma[i] = (int) (pow ((float) i / 255.0, gamma) * 255.0);
    }
}

void LedGrid_SetPalette (LedGrid lg, Palette p) {
    assert (lg != NULL);
    assert (p != NULL);

    lg->p = p;
}

void LedGrid_SetColor (LedGrid lg, int col, int row,
        unsigned char red, unsigned char green, unsigned char blue) {
    int pixel;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);
    lg->strip[3 * pixel + RED]   = red;
    lg->strip[3 * pixel + GREEN] = green;
    lg->strip[3 * pixel + BLUE]  = blue;
}

void LedGrid_SetColorValue (LedGrid lg, int col, int row,
        enum LedGrid_ColorIndexEnum colorIndex, unsigned char value) {
    int pixel;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);
    lg->strip[3 * pixel + colorIndex] = value;
}

void LedGrid_SetColorInt (LedGrid lg, int col, int row,
        unsigned int value) {
    int pixel;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);
    lg->strip[3 * pixel + RED]   = (value >> 16) & 0xFF;
    lg->strip[3 * pixel + GREEN] = (value >> 8) & 0xFF;
    lg->strip[3 * pixel + BLUE]  = value & 0xFF;
}

void LedGrid_SetColorPal (LedGrid lg, int col, int row,
        unsigned char palPos) {
    int pixel;

    assert (lg != NULL);
    assert (lg->p != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);
    lg->strip[3 * pixel + RED]   = lg->p->pal[3 * palPos + RED];
    lg->strip[3 * pixel + GREEN] = lg->p->pal[3 * palPos + GREEN];
    lg->strip[3 * pixel + BLUE]  = lg->p->pal[3 * palPos + BLUE];
}

void LedGrid_SetRed (LedGrid lg, int col, int row,
        unsigned char value) {
    int pixel;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);
    lg->strip[3 * pixel + RED] = value;
}

void LedGrid_SetGreen (LedGrid lg, int col, int row,
        unsigned char value) {
    int pixel;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);
    lg->strip[3 * pixel + GREEN] = value;
}

void LedGrid_SetBlue (LedGrid lg, int col, int row,
        unsigned char value) {
    int pixel;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);
    lg->strip[3 * pixel + BLUE] = value;
}

unsigned char LedGrid_GetColorValue (LedGrid lg, int col, int row,
        enum LedGrid_ColorIndexEnum colorIndex) {
    int pixel;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);

    return lg->strip[3 * pixel + colorIndex];
}

unsigned int LedGrid_GetColorInt (LedGrid lg, int col, int row) {
    int pixel;
    unsigned int value;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);

    value = lg->strip[3 * pixel + RED];
    value <<= 8;
    value += lg->strip[3 * pixel + GREEN];
    value <<= 8;
    value += lg->strip[3 * pixel + BLUE];

    return value;
}

unsigned char LedGrid_GetRed (LedGrid lg, int col, int row) {
    int pixel;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);

    return lg->strip[3 * pixel + RED];
}

unsigned char LedGrid_GetGreen (LedGrid lg, int col, int row) {
    int pixel;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);

    return lg->strip[3 * pixel + GREEN];
}

unsigned char LedGrid_GetBlue (LedGrid lg, int col, int row) {
    int pixel;

    assert (lg != NULL);
    assert (col < lg->nCols);
    assert (row < lg->nRows);

    pixel = COORD2PIXEL(lg,col,row);

    return lg->strip[3 * pixel + BLUE];
}

void LedGrid_Clear (LedGrid lg) {
    int i;

    assert (lg != NULL);

    for (i=0; i<3*lg->nPixels; i++) {
        lg->strip[i] = 0;
    }
}

/*-----------------------------------------------------------------------------
 *
 * Palette --
 *
 */
Palette Palette_Init () {
    Palette p;

    p = malloc (sizeof (*p));
    p->pal = calloc (256, 3 * sizeof (unsigned char));

    return p;
}

void Palette_Free (Palette p) {
    assert (p != NULL);

    free (p->pal);
    free (p);
}

void Palette_SetColor (Palette p, int colorPos,
        unsigned char red, unsigned char green, unsigned char blue) {
    assert (p != NULL);
    assert ((colorPos >= 0) && (colorPos < 256));

    p->pal[3 * colorPos + RED]   = red;
    p->pal[3 * colorPos + GREEN] = green;
    p->pal[3 * colorPos + BLUE]  = blue;
}

void Palette_SetColorValue (Palette p, int colorPos,
        enum LedGrid_ColorIndexEnum colorIndex, unsigned char value) {
    assert (p != NULL);
    assert ((colorPos >= 0) && (colorPos < 256));

    p->pal[3 * colorPos + colorIndex] = value;
}

void Palette_SetColorInt (Palette p, int colorPos,
        unsigned int value) {
    assert (p != NULL);
    assert ((colorPos >= 0) && (colorPos < 256));

    p->pal[3 * colorPos + RED]   = (value >> 16) & 0xFF;
    p->pal[3 * colorPos + GREEN] = (value >> 8) & 0xFF;
    p->pal[3 * colorPos + BLUE]  = value & 0xFF;
}

void Palette_Interpolate (Palette p, int colorPosFrom, int colorPosTo) {
    int numSteps, step;
    unsigned char red1, green1, blue1;
    unsigned char red2, green2, blue2;
    unsigned char red,  green,  blue;
    double dRed, dGreen, dBlue;

    assert (p != NULL);
    assert (colorPosFrom >= 0);
    assert (colorPosTo < 256);
    assert (colorPosFrom < colorPosTo);

    numSteps = colorPosTo - colorPosFrom;

    red1   = p->pal[3 * colorPosFrom + RED];
    green1 = p->pal[3 * colorPosFrom + GREEN];
    blue1  = p->pal[3 * colorPosFrom + BLUE];

    red2   = p->pal[3 * colorPosTo + RED];
    green2 = p->pal[3 * colorPosTo + GREEN];
    blue2  = p->pal[3 * colorPosTo + BLUE];

    dRed   = ((double)red2-red1)/(double)numSteps;
    dGreen = ((double)green2-green1)/(double)numSteps;
    dBlue  = ((double)blue2-blue1)/(double)numSteps;

    for (step=1; step<numSteps; step++) {
        red   = red1   + step * dRed;
        green = green1 + step * dGreen;
        blue  = blue1  + step * dBlue;

        Palette_SetColor (p, colorPosFrom+step, red, green, blue);
    }
}


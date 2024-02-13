#define _GNU_SOURCE
#define LEDGRID_V2

#include "PiPack.h"
#include <wiringPi.h>
#include <wiringPiSPI.h>
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
#include <errno.h>

/*
 * Semaphore --
 */

struct Semaphore {
    int              count;
    pthread_mutex_t *mutex;
    pthread_cond_t  *cond;
};

Semaphore Semaphore_Init (int count) {
    Semaphore s;

    s = malloc (sizeof *s);
    s->count = count;
    s->mutex = malloc (sizeof *s->mutex);
    s->cond  = malloc (sizeof *s->cond);
    pthread_mutex_init (s->mutex, NULL);
    pthread_cond_init (s->cond, NULL);

    return s;
}

void Semaphore_P (Semaphore s) {
    assert (s != NULL);

    pthread_mutex_lock (s->mutex);
    while (s->count == 0) {
        pthread_cond_wait (s->cond, s->mutex);
    }
    s->count--;
    pthread_mutex_unlock (s->mutex);
}

void Semaphore_V (Semaphore s) {
    assert (s != NULL);

    pthread_mutex_lock (s->mutex);
    s->count++;
    if (s->count > 0) {
        pthread_cond_signal (s->cond);
    }
    pthread_mutex_unlock (s->mutex);
}

/*
 * Button --
 */

struct Button {
    int pin;
    void (*riseHandler)(void *arg);
    void *riseArg;
    unsigned int riseTime;
    void (*fallHandler)(void *arg);
    void *fallArg;
    unsigned int fallTime;
};

void Button_Handler (void *arg) {
    Button btn;
    int level;

    assert (arg != NULL);

    btn = (Button) arg;
    level = Button_GetState (btn);
    if (level == HIGH) {
        btn->riseTime = millis ();
        if (btn->riseHandler != NULL) {
            (btn->riseHandler) (btn->riseArg);
        }
    }
    if (level == LOW) {
        btn->fallTime = millis ();
        if (btn->fallHandler != NULL) {
            (btn->fallHandler) (btn->fallArg);
        }
    }
}

Button Button_Init (int pin) {
    Button btn;

    assert ((pin >= 0) && (pin <= 16));

    btn = malloc (sizeof *btn);
    btn->pin = pin;
    pinMode (btn->pin, INPUT);
    btn->riseHandler = NULL;
    btn->riseArg     = NULL;
    btn->fallHandler = NULL;
    btn->fallArg     = NULL;

    wiringPiISR (btn->pin, INT_EDGE_BOTH, Button_Handler, btn);

    return btn;
}

int Button_GetState (Button btn) {
    assert (btn != NULL);

    return digitalRead (btn->pin);
}

void Button_SetEdgeHandler (Button btn,
        enum Button_EdgeTypeEnum edgeType,
        void (*handler)(void *arg), void *arg) {

    assert (btn != NULL);
    assert (handler != NULL);

    switch (edgeType) {
        case RISING:
            btn->riseHandler = handler;
            btn->riseArg     = arg;
            break;
        case FALLING:
            btn->fallHandler = handler;
            btn->fallArg     = arg;
            break;
        case BOTH:
            btn->riseHandler = handler;
            btn->riseArg     = arg;
            btn->fallHandler = handler;
            btn->fallArg     = arg;
            break;
    }
}

/*
 * Led --
 */

struct Led {
    int pin;
    int mode;
    int value;
};

Led Led_Init (int pin, enum Led_ModeEnum mode) {
    Led led;

    assert ((pin >= 0) && (pin <= 16));

    led = malloc (sizeof *led);
    led->pin   = pin;
    led->mode  = mode;
    led->value = 0;

    if ((mode == OUT) || ((mode == PWM) && (pin == 1))) {
        pinMode (pin, mode);
    } else {
        softPwmCreate (pin, 0, 100);
    }

    return led;
}

void Led_On (Led led) {
    assert (led != NULL);

    switch (led->mode) {
        case OUT:
            Led_Set (led, 1);
            break;
        case PWM:
            Led_Set (led, 100);
            break;
    }
}

void Led_Off (Led led) {
    assert (led != NULL);

    Led_Set (led, 0);
}

void Led_Set (Led led, int value) {
    assert (led != NULL);
    assert (value >= 0);

    led->value = value;
    switch (led->mode) {
        case OUT:
            digitalWrite (led->pin, led->value);
            break;
        case PWM:
            if (led->pin == 1) {
                pwmWrite (led->pin, (1024 * led->value) / 100);
            } else {
                softPwmWrite (led->pin, led->value);
            }
            break;
    }
}

int Led_Get (Led led) {
    assert (led != NULL);

    return led->value;
}

/*
 * LedCount --
 */

struct LedCount {
    Led led[3];
    int value;
};

LedCount LedCount_Init (int led01, int led02, int led04) {
    LedCount lcn;

    lcn = malloc (sizeof *lcn);
    lcn->led[0] = Led_Init (led01, OUTPUT);
    lcn->led[1] = Led_Init (led02, OUTPUT);
    lcn->led[2] = Led_Init (led04, OUTPUT);
    lcn->value = 0;

    return lcn;
}

void LedCount_Set (LedCount lcn, int value) {
    int i;

    lcn->value = value;

    for (i=0; i<3; i++) {
        if (lcn->value & (0x01 << i)) {
            Led_On (lcn->led[i]);
        } else {
            Led_Off (lcn->led[i]);
        }
    }
}

int LedCount_Get (LedCount lcn) {
    return lcn->value;
}

/*
 * LedFader --
 */

struct LedFader {
    Led led;
    Button button;
    int oldValue;
    int fadeDir;
    int isFading;
    int fadeThreadID;
};

void *LedFader_FadeThread (void *arg) {
    LedFader lfd = (LedFader) arg;
    int i;

    delay (1000);
    if (Button_GetState (lfd->button) == LOW) {
        return NULL;
    }
    lfd->isFading = 1;
    i = Led_Get (lfd->led);
    while ((Button_GetState (lfd->button) == HIGH)
            && (i>=0 && i<=100)) {
        Led_Set (lfd->led, i);
        i = i + lfd->fadeDir;
        delay (50);
    }
    lfd->fadeDir = -lfd->fadeDir;

    return NULL;
}

void LedFader_RiseHandler (void *arg) {
    LedFader lfd = (LedFader) arg;

    if (lfd->fadeThreadID != 0) {
        return;
    }
    lfd->fadeThreadID = piThreadCreate (LedFader_FadeThread, arg);
}

void LedFader_FallHandler (void *arg) {
    LedFader lfd = (LedFader) arg;

    if (lfd->isFading) {
        lfd->isFading = 0;
        lfd->fadeThreadID = 0;
        return;
    }

    lfd->oldValue = Led_Get (lfd->led);
    if (lfd->oldValue > 0) {
        Led_Off (lfd->led);
        if (lfd->oldValue < 512) {
            lfd->fadeDir = +1;
        } else {
            lfd->fadeDir = -1;
        }
    } else {
        Led_On (lfd->led);
        lfd->fadeDir = -1;
    }
}

LedFader LedFader_Init (int ledPin, int buttonPin) {
    LedFader lfd;

    lfd = malloc (sizeof *lfd);
    lfd->led = Led_Init (ledPin, PWM);
    lfd->button = Button_Init (buttonPin);
    lfd->fadeDir = +1;
    lfd->isFading = 0;
    lfd->fadeThreadID = 0;

    Button_SetEdgeHandler (lfd->button, RISING,
            LedFader_RiseHandler, lfd);
    Button_SetEdgeHandler (lfd->button, FALLING,
            LedFader_FallHandler, lfd);

    return lfd;
}

/*
 * LedStrip --
 */

#define LEDSTRIP_MAXLENGTH     100
#define PIPACK_SPI_CHANNEL       0
#define PIPACK_SPI_SPEED   4000000

struct LedStrip {
    int fd;
    int size;
    unsigned char *array, *output, *gamma;
};

LedStrip LedStrip_Init (int size, float gammaValue) {
    LedStrip ls;
    int i;

    assert ((size > 0) && (size <= LEDSTRIP_MAXLENGTH));

    ls = malloc (sizeof (*ls));
    ls->fd = wiringPiSPISetup(PIPACK_SPI_CHANNEL, PIPACK_SPI_SPEED);
    if (ls->fd < 0) {
        fprintf(stderr, "Can't open the SPI bus: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
//    ls->fd = open ("/dev/spidev0.0", O_WRONLY | O_DSYNC);
    ls->size = size;
    ls->array = calloc (size, 3 * sizeof (unsigned char));
    ls->output = calloc (size, 3 * sizeof (unsigned char));
    ls->gamma = calloc (256, sizeof (unsigned char));
    LedStrip_SetGamma (ls, gammaValue);

    return ls;
}

void LedStrip_Free (LedStrip ls) {
    assert (ls != NULL);

    close (ls->fd);
    free (ls->array);
    free (ls->output);
    free (ls->gamma);
    free (ls);
}

void LedStrip_Show (LedStrip ls) {
    int i;

    assert (ls != NULL);

    for (i=0; i<3*ls->size; i++) {
        ls->output[i] = ls->gamma[ls->array[i]];
    }
    if (wiringPiSPIDataRW(PIPACK_SPI_CHANNEL, ls->output, 3 * ls->size) < 0) {
        fprintf(stderr, "SPI failure: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
//    write (ls->fd, ls->output, 3 * ls->size);
//    fsync (ls->fd);
}

void LedStrip_SetGamma (LedStrip ls, float gammaValue) {
    int i;

    assert (ls != NULL);

    for (i=0; i<256; i++) {
        ls->gamma[i] = (unsigned char) (255.0 * pow ((float)i/255.0, gammaValue) + 0.5);
    }
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

unsigned char LedStrip_GetRed (LedStrip ls, int pixel) {
    return ls->array[3 * pixel + 0];
}

unsigned char LedStrip_GetGreen (LedStrip ls, int pixel) {
    return ls->array[3 * pixel + 1];
}

unsigned char LedStrip_GetBlue (LedStrip ls, int pixel) {
    return ls->array[3 * pixel + 2];
}

void LedStrip_SetValue (LedStrip ls, int pixel, unsigned char value) {
    LedStrip_SetColor (ls, pixel, value, value, value);
}

unsigned char LedStrip_GetValue (LedStrip ls, int pixel) {
    return ls->array[3 * pixel];
}

void LedStrip_Cycle (LedStrip ls) {
    int i;
    unsigned char red, green, blue;

    red   = ls->array[3 * (ls->size-1) + 0];
    green = ls->array[3 * (ls->size-1) + 1];
    blue  = ls->array[3 * (ls->size-1) + 2];
    for (i=ls->size-1; i>0; i--) {
        ls->array[3 * i + 0] = ls->array[3 * (i-1) + 0];
        ls->array[3 * i + 1] = ls->array[3 * (i-1) + 1];
        ls->array[3 * i + 2] = ls->array[3 * (i-1) + 2];
    }
    ls->array[0] = red;
    ls->array[1] = green;
    ls->array[2] = blue;
}

void LedStrip_PushColor (LedStrip ls,
        unsigned char red, unsigned char green, unsigned char blue) {
    int i;

    for (i=ls->size-1; i>0; i--) {
        ls->array[3 * i + 0] = ls->array[3 * (i-1) + 0];
        ls->array[3 * i + 1] = ls->array[3 * (i-1) + 1];
        ls->array[3 * i + 2] = ls->array[3 * (i-1) + 2];
    }
    LedStrip_SetColor (ls, 0, red, green, blue);
}

void LedStrip_PushValue (LedStrip ls, unsigned char value) {
    LedStrip_PushColor (ls, value, value, value);
}

void LedStrip_AllOn (LedStrip ls) {
    int i;

    for (i=0; i<ls->size; i++) {
        LedStrip_SetValue (ls, i, 255);
    }
    LedStrip_Show (ls);
}

void LedStrip_AllOff (LedStrip ls) {
    int i;

    for (i=0; i<ls->size; i++) {
        LedStrip_SetValue (ls, i, 0);
    }
    LedStrip_Show (ls);
}

void LedStrip_Fade (LedStrip ls) {
    int i;

    for (i=0; i<3*ls->size; i++) {
        ls->array[i] /= 2;
    }
}

/*
 * LedGrid --
 */

//
// V1.0
//

struct LedGrid {
    int sizeX, sizeY, size;
    int numImages, curImage, fadeStep;
    unsigned char ***field;
    int startByte;
#ifdef LEDGRID_V1
    int fd;
    unsigned char *array;
#endif
#ifdef LEDGRID_V2
    LedStrip ls;
#endif
    Semaphore sem;
};

LedGrid LedGrid_Init (int sizeX, int sizeY, float gammaValue) {
    LedGrid lg;
    int i;

    lg = malloc (sizeof (*lg));
    lg->sizeX = sizeX;
    lg->sizeY = sizeY;
    lg->numImages = 1;
    lg->curImage  = 0;
    lg->fadeStep  = 0;

    lg->field    = calloc (lg->numImages, sizeof (unsigned char **));
    lg->field[lg->curImage] = calloc (lg->sizeY, sizeof (unsigned char *));
    for (i=0; i<sizeY; i++) {
        lg->field[lg->curImage][i] = calloc (lg->sizeX, 3 * sizeof (unsigned char));
    }
    lg->startByte = 3 * (LEDSTRIP_MAXLENGTH - (lg->sizeX * lg->sizeY));

#ifdef LEDGRID_V1
    lg->fd = open ("/dev/spidev0.0", O_WRONLY | O_DSYNC);
    lg->array = calloc (LEDSTRIP_MAXLENGTH, 3 * sizeof (unsigned char));
#endif
#ifdef LEDGRID_V2
    lg->ls = LedStrip_Init (LEDSTRIP_MAXLENGTH, gammaValue);
#endif
    lg->sem = Semaphore_Init (1);
     
    return lg;
}

void LedGrid_Free (LedGrid lg) {
    int i, j;

    assert (lg != NULL);

    for (i=0; i<lg->numImages; i++) {
        for (j=0; j<lg->sizeY; j++) {
            free (lg->field[i][j]);
        }
        free (lg->field[i]);
    }
    free (lg->field);
#ifdef LEDGRID_V1
    free (lg->array);
#endif
#ifdef LEDGRID_V2
    LedStrip_Free (lg->ls);
#endif

    free (lg);
}

void LedGrid_Show (LedGrid lg) {
    int i, j, k, l;
    int v1, v2, v;

    assert (lg != NULL);

    Semaphore_P (lg->sem);
    k = lg->startByte;
    for (i=0; i<lg->sizeY; i++) {
        if (i%2 == 0) {
            for (l=0; l<3*lg->sizeX; l++) {
                if (lg->fadeStep == 0) {
                    v  = lg->field[lg->curImage][i][l];
                } else {
                    v1 = lg->field[lg->curImage][i][l];
                    v2 = lg->field[(lg->curImage+1)%lg->numImages][i][l];
                    v = v1 + lg->fadeStep * (v2-v1) / 100;
                }
#ifdef LEDGRID_V1
                lg->array[k++] = v;
#endif
#ifdef LEDGRID_V2
                lg->ls->array[k++] = v;
#endif
            }
        } else {
            for (j=lg->sizeX-1; j>=0; j--) {
                for (l=0; l<3; l++) {
		    if (lg->fadeStep == 0) {
			v  = lg->field[lg->curImage][i][3*j+l];
		    } else {
			v1 = lg->field[lg->curImage][i][3*j+l];
			v2 = lg->field[(lg->curImage+1)%lg->numImages][i][3*j+l];
			v = v1 + lg->fadeStep * (v2-v1) / 100;
		    }
#ifdef LEDGRID_V1
                    lg->array[k++] = v;
#endif
#ifdef LEDGRID_V2
                    lg->ls->array[k++] = v;
#endif
                }
            }
        }
    }
#ifdef LEDGRID_V1
    write (lg->fd, lg->array, 3 * LEDSTRIP_MAXLENGTH);
    fsync (lg->fd);
#endif
#ifdef LEDGRID_V2
    LedStrip_Show (lg->ls);
#endif
    Semaphore_V (lg->sem);
}

void LedGrid_SetGamma (LedGrid lg, float gammaValue) {
    assert (lg != NULL);

    LedStrip_SetGamma (lg->ls, gammaValue);
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

    lg->field[lg->curImage][y][3 * x + colorIndex] = value;
}

void LedGrid_SetValue (LedGrid lg, int x, int y, unsigned char value) {
    LedGrid_SetColor (lg, x, y, value, value, value);
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

void LedGrid_SetColorInt (LedGrid lg, int x, int y, unsigned int value) {
    LedGrid_SetColorValue (lg, x, y, RED, (value>>16)&0xFF);
    LedGrid_SetColorValue (lg, x, y, GREEN, (value>>8)&0xFF);
    LedGrid_SetColorValue (lg, x, y, BLUE, value&0xFF);
}

void LedGrid_SetAllColor (LedGrid lg, unsigned char red,
        unsigned char green, unsigned char blue) {
    int x, y;

    assert (lg != NULL);
    assert ((red >= 0) && (red < 256));
    assert ((green >= 0) && (green < 256));
    assert ((blue >= 0) && (blue < 256));

    for (y=0; y<lg->sizeY; y++) {
        for (x=0; x<lg->sizeX; x++) {
            lg->field[lg->curImage][y][3 * x + RED]   = red;
            lg->field[lg->curImage][y][3 * x + GREEN] = green;
            lg->field[lg->curImage][y][3 * x + BLUE]  = blue;
        }
    }
}

void LedGrid_SetAllColorValue (LedGrid lg,
        enum LedStrip_ColorIndexEnum colorIndex, unsigned char value) {
    int x, y;

    assert (lg != NULL);
    assert ((colorIndex >= RED) && (colorIndex <= BLUE));
    assert ((value >= 0) && (value < 256));

    for (y=0; y<lg->sizeY; y++) {
        for (x=0; x<lg->sizeX; x++) {
            lg->field[lg->curImage][y][3 * x + colorIndex] = value;
        }
    }
}

unsigned char LedGrid_GetValue (LedGrid lg, int x, int y) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));

    return lg->field[lg->curImage][y][3 * x];
}

unsigned char LedGrid_GetColorValue (LedGrid lg, int x, int y,
        enum LedStrip_ColorIndexEnum colorIndex) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));
    assert ((colorIndex >= RED) && (colorIndex <= BLUE));

    return lg->field[lg->curImage][y][3 * x + colorIndex];
}

unsigned char LedGrid_GetRed (LedGrid lg, int x, int y) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));

    return lg->field[lg->curImage][y][3 * x + RED];
}

unsigned char LedGrid_GetGreen (LedGrid lg, int x, int y) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));

    return lg->field[lg->curImage][y][3 * x + GREEN];
}

unsigned char LedGrid_GetBlue (LedGrid lg, int x, int y) {
    assert (lg != NULL);
    assert ((x >= 0) && (y >= 0));

    return lg->field[lg->curImage][y][3 * x + BLUE];
}

void LedGrid_AllOn (LedGrid lg) {
    int i, j;

    assert (lg != NULL);

    for (i=0; i<lg->sizeY; i++) {
        for (j=0; j<lg->sizeX; j++) {
            LedGrid_SetValue (lg, j, i, 1.0);
        }
    }
    LedGrid_Show (lg);
}

void LedGrid_AllOff (LedGrid lg) {
    int i, j;

    assert (lg != NULL);

    for (i=0; i<lg->sizeY; i++) {
        for (j=0; j<lg->sizeX; j++) {
            LedGrid_SetValue (lg, j, i, 0.0);
        }
    }
    LedGrid_Show (lg);
}

int LedGrid_NewImage (LedGrid lg) {
    int i, imgIndex;

    assert (lg != NULL);

    imgIndex = lg->numImages;
    lg->numImages += 1;
    lg->field = realloc (lg->field, lg->numImages * sizeof (unsigned char **));
    lg->field[imgIndex] = calloc (lg->sizeY, sizeof (unsigned char *));
    for (i=0; i<lg->sizeY; i++) {
        lg->field[imgIndex][i] = calloc (lg->sizeX, 3 * sizeof (unsigned char));
    }

    return imgIndex;
}

int LedGrid_LoadImage (LedGrid lg, char *fileName, int imgIndex) {
    FILE *fd;
    int sizeX, sizeY;
    int x, y, red, green, blue;

    assert (lg != NULL);
    assert (fileName != NULL);

    if (imgIndex < 0) {
        imgIndex = LedGrid_NewImage (lg);
    } else if (imgIndex >= lg->numImages) {
        return -1;
    }

    fd = fopen (fileName, "r");
    fscanf (fd, "%d %d", &sizeX, &sizeY);
    if ((sizeX != lg->sizeX) || (sizeY != lg->sizeY)) {
        return -1;
    }
    for (y=0; y<lg->sizeY; y++) {
        for (x=0; x<lg->sizeX; x++) {
            fscanf (fd, "%2x%2x%2x", &red, &green, &blue);
            lg->field[imgIndex][y][3 * x + RED] = red;
            lg->field[imgIndex][y][3 * x + GREEN] = green;
            lg->field[imgIndex][y][3 * x + BLUE] = blue;
        }
    }
    fclose (fd);

    return imgIndex;
}

int LedGrid_SaveImage (LedGrid lg, char *fileName, int imgIndex) {
    FILE *fd;
    int x, y, red, green, blue;

    assert (lg != NULL);
    assert (fileName != NULL);

    if (imgIndex < 0) {
        imgIndex = lg->curImage;
    } else if (imgIndex >= lg->numImages) {
        return -1;
    }

    fd = fopen (fileName, "w");
    fprintf (fd, "%d %d\n\n", lg->sizeX, lg->sizeY);
    for (y=0; y<lg->sizeY; y++) {
        for (x=0; x<lg->sizeX; x++) {
            red = LedGrid_GetRed (lg, x, y);
            green = LedGrid_GetGreen (lg, x, y);
            blue = LedGrid_GetBlue (lg, x, y);
            fprintf (fd, "%02x%02x%02x ", red, green, blue);
        }
        fprintf (fd, "\n");
    }
    fprintf (fd, "\n");
    fclose (fd);

    return imgIndex;
}

void LedGrid_SetImage (LedGrid lg, int imgIndex, int fadeStep) {
    assert (lg != NULL);

    if (imgIndex >= lg->numImages) {
        return;
    }
    Semaphore_P (lg->sem);
    lg->curImage = imgIndex;
    lg->fadeStep = fadeStep;
    Semaphore_V (lg->sem);
}

int LedGrid_GetImageCount (LedGrid lg) {
    assert (lg != NULL);

    return lg->numImages;
}

int LedGrid_GetCurImage (LedGrid lg) {
    assert (lg != NULL);

    return lg->curImage;
}

void LedGrid_FadeImage (LedGrid lg) {
    int curIndex, newIndex;
    int x, y, i, j, k;
    int value;

    assert (lg != NULL);

    curIndex = lg->curImage;
    newIndex = curIndex + 1;
    if (newIndex == lg->numImages) {
        LedGrid_NewImage (lg);
    }

    for (y=0; y<lg->sizeY; y++) {
        for (x=0; x<lg->sizeX; x++) {
            for (k=0; k<3; k++) {
                value = 0;
                for (i=y-1; i<=y+1; i++) {
                    for (j=x-1; j<=x+1; j++) {
                        if ((i<0) || (i>=lg->sizeY) || (j<0) \
                                || (j>=lg->sizeX) || ((i==y) && (j==x))) {
                            continue;
                        }
                        value += lg->field[curIndex][i][3*j+k];
                    }
                }
                value = value / 8;
                value += lg->field[curIndex][y][3*x+k];
                value = value / 2;
                lg->field[newIndex][y][3*x+k] = value;
            }
        }
    }
}

/*
void LedGrid_InterpolateImage (LedGrid lg) {
    int curIndex;

    curIndex = lg->curImage;
}
*/

void LedGrid_Clear (LedGrid lg) {
    int curIndex;
    int x, y, k;

    assert (lg != NULL);

    curIndex = lg->curImage;
    for (y=0; y<lg->sizeY; y++) {
        for (x=0; x<lg->sizeX; x++) {
            for (k=0; k<3; k++) {
                lg->field[curIndex][y][3*x+k] = 0;
            }
        }
    }
}

void LedGrid_Shift (LedGrid lg, enum LedGrid_ShiftDirectionEnum dir,
        int rotate) {
    int i;
    int x, y, k;
    unsigned char t[3];

    assert (lg != NULL);

    i = lg->curImage;
    switch (dir) {
        case SHIFT_UP:
            for (x=0; x<lg->sizeX; x++) {
                for (k=0; k<3; k++) {
                    t[k] = lg->field[i][0][3*x+k];
                    for (y=0; y<lg->sizeY-1; y++) {
                        lg->field[i][y][3*x+k] = lg->field[i][y+1][3*x+k];
                    }
                    if (rotate) {
                        lg->field[i][y][3*x+k] = t[k];
                    } else {
                        lg->field[i][y][3*x+k] = 0;
                    }
                }
            }
            break;

        case SHIFT_DOWN:
            for (x=0; x<lg->sizeX; x++) {
                for (k=0; k<3; k++) {
                    t[k] = lg->field[i][lg->sizeY-1][3*x+k];
                    for (y=lg->sizeY-1; y>0; y--) {
                        lg->field[i][y][3*x+k] = lg->field[i][y-1][3*x+k];
                    }
                    if (rotate) {
                        lg->field[i][y][3*x+k] = t[k];
                    } else {
                        lg->field[i][y][3*x+k] = 0;
                    }
                }
            }
            break;

        case SHIFT_LEFT:
            for (y=0; y<lg->sizeY; y++) {
                for (k=0; k<3; k++) {
                    t[k] = lg->field[i][y][3*0+k];
                    for (x=0; x<lg->sizeX-1; x++) {
                        lg->field[i][y][3*x+k] = lg->field[i][y][3*(x+1)+k];
                    }
                    if (rotate) {
                        lg->field[i][y][3*x+k] = t[k];
                    } else {
                        lg->field[i][y][3*x+k] = 0;
                    }
                }
            }
            break;

        case SHIFT_RIGHT:
            for (y=0; y<lg->sizeY; y++) {
                for (k=0; k<3; k++) {
                    t[k] = lg->field[i][y][3*(lg->sizeX-1)+k];
                    for (x=lg->sizeX-1; x>0; x--) {
                        lg->field[i][y][3*x+k] = lg->field[i][y][3*(x-1)+k];
                    }
                    if (rotate) {
                        lg->field[i][y][3*x+k] = t[k];
                    } else {
                        lg->field[i][y][3*x+k] = 0;
                    }
                }
            }
            break;

    }
}

/*
void LedGrid_Shift (LedGrid lg, enum LedGrid_ShiftDirectionEnum dir,
        int count, int rotate) {
    int i;
    int x, y, k;

    assert (lg != NULL);

    i = lg->curImage;
    switch (dir) {
        case SHIFT_UP:
            for (x=0; x<lg->sizeX; x++) {
                for (y=0; y<lg->sizeY-count; y++) {
                    for (k=0; k<3; k++) {
                        lg->field[i][y][3*x+k] = lg->field[i][y+count][3*x+k];
                    }
                }
                for ( ; y<lg->sizeY; y++) {
                    for (k=0; k<3; k++) {
                        lg->field[i][y][3*x+k] = 0;
                    }
                }
            }
            break;
        case SHIFT_DOWN:
            for (x=0; x<lg->sizeX; x++) {
                for (y=lg->sizeY-1; y>count; y--) {
                    for (k=0; k<3; k++) {
                        lg->field[i][y][3*x+k] = lg->field[i][y-count][3*x+k];
                    }
                }
                for ( ; y>=0; y--) {
                    for (k=0; k<3; k++) {
                        lg->field[i][y][3*x+k] = 0;
                    }
                }
            }
            break;
        case SHIFT_LEFT:
            for (y=0; y<lg->sizeY; y++) {
                for (x=0; x<lg->sizeX-count; x++) {
                    for (k=0; k<3; k++) {
                        lg->field[i][y][3*x+k] = lg->field[i][y][3*(x+count)+k];
                    }
                }
                for ( ; x<lg->sizeX; x++) {
                    for (k=0; k<3; k++) {
                        lg->field[i][y][3*x+k] = 0;
                    }
                }
            }
            break;
        case SHIFT_RIGHT:
            for (y=0; y<lg->sizeY; y++) {
                for (x=lg->sizeX-1; x>count; x--) {
                    for (k=0; k<3; k++) {
                        lg->field[i][y][3*x+k] = lg->field[i][y][3*(x-count)+k];
                    }
                }
                for ( ; x>=0; x--) {
                    for (k=0; k<3; k++) {
                        lg->field[i][y][3*x+k] = 0;
                    }
                }
            }
            break;
    }
}
*/

/*
 * ColorGrid --
 */

typedef struct ColorFuncType {
    ColorFunc *func;
    char *name;
} ColorFuncType;

typedef struct ColorGrid {
    LedGrid lg;
    unsigned char **matrix;
    // int **matrix;
    int size;
    int numFadeSteps;
    int fadeStep[3], fadeIncr[3];
    double maxValue[3], expValue[3];
    int colorFunc[3];
    int numColorFuncs;
    ColorFuncType *colorFuncArray;
    Semaphore sem;
} *ColorGrid;

ColorGrid ColorGrid_Init (int size, int numFadeSteps, float gammaValue) {
    ColorGrid cg;
    int i, j;

    assert ((size > 0) && (numFadeSteps > 0));

    cg = malloc (sizeof (* cg));
    cg->lg = LedGrid_Init (size, size, gammaValue);
    cg->matrix = calloc (3, sizeof (int *));
    cg->size = size;
    cg->numFadeSteps = numFadeSteps;
    for (i=0; i<3; i++) {
        cg->matrix[i] = calloc (2*(size-1)*numFadeSteps, sizeof (int));
        for (j=0; j<2*(size-1)*numFadeSteps; j++) {
            cg->matrix[i][j] = 0;
        }
        cg->fadeStep[i] = 0;
        cg->fadeIncr[i] = 0;
    }
    cg->numColorFuncs = 0;
    cg->sem = Semaphore_Init (1);

    return cg;
}

void ColorGrid_Free (ColorGrid cg) {
    int i;

    assert (cg != NULL);

    for (i=0; i<3; i++) {
        free (cg->matrix[i]);
    }
    LedGrid_Free (cg->lg);
    free (cg);
}

void ColorGrid_Recalc (ColorGrid cg, int color, \
        double max, double exp) {
    int i, j;
    double x;

    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));
    assert ((max > 0.0) && (exp >= 1.0));

    cg->maxValue[color] = max;
    cg->expValue[color] = exp;

    int f (double x, double max, double e) {
        double p = 256.0 / pow (e, 8.0);
        double v;

        x = x * (cg->size-1.0);
        v = p * floor (pow (2.0, (x-1.0)));
        return (v>255) ? 255 : v;
    }

    for (i=0, j=0; i<(cg->size-1)*cg->numFadeSteps; i++, j++) {
        x = (double) (j) / (double) ((cg->size-1) * cg->numFadeSteps);
        cg->matrix[color][i] = f (x, max, exp);
    }

    for (j=(cg->size-1)*cg->numFadeSteps; j>0; i++, j--) {
        x = (double) (j) / (double) ((cg->size-1) * cg->numFadeSteps);
        cg->matrix[color][i] = f (x, max, exp);
    }
}

void ColorGrid_SetGamma (ColorGrid cg, float gammaValue) {
    assert (cg != NULL);

    LedGrid_SetGamma (cg->lg, gammaValue);
}

void ColorGrid_WriteColorFile (ColorGrid cg, char *fileName) {
    FILE *fd;
    int i, j;

    assert (cg != NULL);
    assert (fileName != NULL);

    fd = fopen (fileName, "w");
    if (fd == NULL) {
        fprintf (stderr, "ERROR: couldn't open file '%s'!\n", fileName);
        exit (1);
    }
    fprintf (fd, "%d %d\n", cg->size, cg->numFadeSteps);
    for (i=0; i<3; i++) {
        fprintf (fd, "\n");
        for (j=0; j<2*(cg->size-1)*cg->numFadeSteps; j++) {
            fprintf (fd, "%3d ", cg->matrix[i][j]);
            if ((j+1) % cg->numFadeSteps == 0) {
                fprintf (fd, "\n");
            }
        }
        fprintf (fd, "\n");
    }
    fclose (fd);
}

unsigned char ColorGrid_GetColor (ColorGrid cg, int color, int step) {
    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));
    assert (step >= 0);

    return cg->matrix[color][step%(2*(cg->size-1)*cg->numFadeSteps)];
}

void ColorGrid_Fade (ColorGrid cg, int color) {
    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));

    cg->fadeStep[color] += cg->fadeIncr[color];
    if (cg->fadeStep[color] < 0) {
        cg->fadeStep[color] += 2 * (cg->size-1) * cg->numFadeSteps;
    }
    if (cg->fadeStep[color] >= (2 * (cg->size-1) * cg->numFadeSteps)) {
        cg->fadeStep[color] -= 2 * (cg->size-1) * cg->numFadeSteps;
    }
}

void ColorGrid_SetFadeIncr (ColorGrid cg, int color, int incr) {
    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));

    cg->fadeIncr[color] = incr;
}

int  ColorGrid_GetFadeIncr (ColorGrid cg, int color) {
    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));

    return cg->fadeIncr[color];
}

void ColorGrid_IncrFadeIncr (ColorGrid cg, int color, int incrIncr) {
    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));

    cg->fadeIncr[color] += incrIncr;
}

void ColorGrid_SetFadeStep (ColorGrid cg, int color, int step) {
    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));
    assert (step >= 0);

    cg->fadeStep[color] = step;
    if (cg->fadeStep[color] >= (2 * (cg->size-1) * cg->numFadeSteps)) {
        cg->fadeStep[color] -= 2 * (cg->size-1) * cg->numFadeSteps;
    }
}

int  ColorGrid_GetFadeStep (ColorGrid cg, int color) {
    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));

    return cg->fadeStep[color];
}

void ColorGrid_IncrFadeStep (ColorGrid cg, int color, int stepIncr) {
    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));
    // assert (stepIncr >= 0);

    cg->fadeStep[color] += stepIncr;
    if (cg->fadeStep[color] < 0) {
        cg->fadeStep[color] += 2 * (cg->size-1) * cg->numFadeSteps;
    } else if (cg->fadeStep[color] >= (2 * (cg->size-1) * cg->numFadeSteps)) {
        cg->fadeStep[color] -= 2 * (cg->size-1) * cg->numFadeSteps;
    }
}

void ColorGrid_SetColors (ColorGrid cg) {
    int x, y;

    assert (cg != NULL);

    for (y=0; y<cg->size; y++) {
        for (x=0; x<cg->size; x++) {
            LedGrid_SetColor (cg->lg, x, y, \
                    cg->colorFuncArray[cg->colorFunc[0]].func (cg, 0, \
                        x, y, cg->fadeStep[0]), \
                    cg->colorFuncArray[cg->colorFunc[1]].func (cg, 1, \
                        x, y, cg->fadeStep[1]), \
                    cg->colorFuncArray[cg->colorFunc[2]].func (cg, 2, \
                        x, y, cg->fadeStep[2]));
        }
    }
}

void ColorGrid_Show (ColorGrid cg) {
    LedGrid_Show (cg->lg);
}

void ColorGrid_SetColorFunc (ColorGrid cg, int color, int funcIndex) {
    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));
    assert ((funcIndex >= 0) && (funcIndex < cg->numColorFuncs));

    cg->colorFunc[color] = funcIndex;
}

int ColorGrid_GetColorFunc (ColorGrid cg, int color) {
    assert (cg != NULL);
    assert ((color >= 0) && (color <= 2));

    return cg->colorFunc[color];
}

char *ColorGrid_GetColorFuncName (ColorGrid cg, int funcIndex) {
    assert (cg != NULL);
    assert ((funcIndex >= 0) && (funcIndex < cg->numColorFuncs));

    return cg->colorFuncArray[funcIndex].name;
}

void ColorGrid_AddColorFunc (ColorGrid cg, ColorFunc func, char *name) {
    int colorFuncIndex;

    assert (cg != NULL);
    assert ((func != NULL) && (name != NULL));

    colorFuncIndex = cg->numColorFuncs;
    cg->numColorFuncs++;
    cg->colorFuncArray = realloc (cg->colorFuncArray, \
            cg->numColorFuncs * sizeof (ColorFuncType));
    cg->colorFuncArray[colorFuncIndex].func = func;
    cg->colorFuncArray[colorFuncIndex].name = strdup (name);
}

int ColorGrid_GetNumColorFuncs (ColorGrid cg) {
    assert (cg != NULL);

    return cg->numColorFuncs;
}

int ColorGrid_NewImage (ColorGrid cg) {
    assert (cg != NULL);

    return LedGrid_NewImage (cg->lg);
}

void ColorGrid_SetImage (ColorGrid cg, int imageIndex, int fadeStep) {
    assert (cg != NULL);

    LedGrid_SetImage (cg->lg, imageIndex, fadeStep);
}

/*
 * LedRun --
 */

typedef struct LedRun {
    LedStrip ls;
    int colorIndex;
    int pos;
    int incr;
    int step;
    int divisor;
} *LedRun;

LedRun LedRun_Init (LedStrip ls, enum LedStrip_ColorIndexEnum colorIndex,
        int pos, int incr, int divisor) {
    LedRun rl;

    rl = malloc (sizeof (*rl));
    rl->ls = ls;
    rl->colorIndex = colorIndex;
    rl->pos = pos;
    rl->incr = incr;
    rl->step = 0;
    rl->divisor = divisor;

    return rl;
}

void LedRun_Free (LedRun rl) {
    free (rl);
}

void LedRun_Next (LedRun rl) {
    rl->step = (rl->step + 1) % rl->divisor;
    if (rl->step != 0) {
        return;
    }
    if (((rl->incr > 0) && (rl->pos+rl->incr > rl->ls->size-1))
            || ((rl->incr < 0) && (rl->pos+rl->incr < 0))) {
        rl->incr = -rl->incr;
    }
    rl->pos += rl->incr;
}

void LedRun_Set (LedRun rl) {
    LedStrip_SetColorValue (rl->ls, rl->pos, rl->colorIndex, 255);
}

/*
 * PhotoSensor --
 */

struct PhotoSensor {
    int pin;
    int min, max, val;
    int photoThreadID;
    //pthread_mutex_t mutex;
};

void *PhotoSensor_Thread (void *arg) {
    PhotoSensor sen;
    int value;

    sen = (PhotoSensor) arg;
    while (1) {
        value = 0; 
        pinMode (sen->pin, OUTPUT);
        digitalWrite (sen->pin, LOW);
        delay (500);
        pinMode (sen->pin, INPUT);
        while (digitalRead (sen->pin) == LOW) {
            value++;
        }
        //if (pthread_mutex_lock (&(sen->mutex))) {
        //    perror ("PhotoSensor_Thread (lock)");
        //}
        sen->val = value;
        if (value > sen->max) {
            sen->max = value;
        } else if (value < sen->min) {
            sen->min = value;
        }
        //if (pthread_mutex_unlock (&(sen->mutex))) {
        //    perror ("PhotoSensor_Thread (unlock)");
        //}
    }
}

PhotoSensor PhotoSensor_Init (int pin) {
    PhotoSensor sen;

    sen = malloc (sizeof *sen);
    sen->pin = pin;
    sen->min = 1000000;
    sen->max = 0;
    sen->val = 0;
    //pthread_mutex_init (&(sen->mutex), NULL);
    sen->photoThreadID = piThreadCreate (PhotoSensor_Thread, sen);

    return sen;
}

/*
void PhotoSensor_Calibrate (PhotoSensor sen, int duration) {
    int val;
    unsigned int t1;

    t1  = millis ();
    while ((millis () - t1) < 1000 * duration) {
        val = PhotoSensor_GetValue (sen->pin);
        if (val > sen->max) {
            sen->max = val;
        } else if (val < sen->min) {
            sen->min = val;
        }
    }
    printf ("INFO: min: %d; max: %d\n", sen->min, sen->max);
}
*/

int PhotoSensor_GetValue (PhotoSensor sen) {
    int value;

    //if (pthread_mutex_lock (&(sen->mutex))) {
    //    perror ("PhotoSensor_GetValue (lock)");
    //}
    value = sen->val;
    //if (pthread_mutex_unlock (&(sen->mutex))) {
    //    perror ("PhotoSensor_GetValue (unlock)");
    //}

    return value;
}

int PhotoSensor_GetLight (PhotoSensor sen) {
    int value;

    //if (pthread_mutex_lock (&(sen->mutex))) {
    //    perror ("PhotoSensor_GetLight (lock)");
    //}
    value = 100 * (sen->max - sen->val) / (sen->max - sen->min);
    //if (pthread_mutex_unlock (&(sen->mutex))) {
    //    perror ("PhotoSensor_GetLight (unlock)");
    //}

    return value;
}

void PhotoSensor_GetInfo (PhotoSensor sen) {
    int min, max, val;

    //if (pthread_mutex_lock (&(sen->mutex))) {
    //    perror ("PhotoSensor_GetInfo (lock)");
    //}
    min = sen->min;
    max = sen->max;
    val = sen->val;
    //if (pthread_mutex_unlock (&(sen->mutex))) {
    //    perror ("PhotoSensor_GetInfo (unlock)");
    //}
    printf ("INFO: (min, max, val): (%d, %d, %d)\n", min, max, val);
}

/*
 * Camera --
 */

struct Camera {
    int  width;
    int  height;
    int  exp;
    int  fx;
    int  imageSequence;
    int  imageNumber;
    char *path;
    char cmd[256];
};

const char *Camera_Exposure[] = {
    "auto", 
    "night",
    "nightpreview",
    "backlight",
    "spotlight",
    "sports",
    "snow",
    "beach",
    "verylong",
    "fixedfps",
    "antishake",
    "fireworks"
};

const char *Camera_Effect[] = {
    "none",
    "negative",
    "solarise",
    "sketch",
    "denoise",
    "emboss",
    "oilpaint",
    "hatch",
    "gpen",
    "pastel",
    "watercolour",
    "film",
    "blur",
    "saturation",
    "colourswap",
    "washedout",
    "posterise",
    "colourpoint",
    "colourbalance",
    "cartoon"
};

void Camera_SetCmd (Camera cam) {
    char fileName[256];

    if (cam->imageSequence) {
        sprintf (fileName, "%s/image%02d.jpg", cam->path, cam->imageNumber);
    } else {
        sprintf (fileName, "%s", cam->path);
    }
        
    sprintf (cam->cmd, "raspistill \
            --width %d --height %d \
            --output %s \
            --nopreview \
            --timeout 500 \
            --exposure %s \
            --imxfx %s",
            cam->width, cam->height,
            fileName,
            Camera_Exposure[cam->exp],
            Camera_Effect[cam->fx]);
}

Camera Camera_Init (int width, int height, char *path) {
    Camera cam;
    struct stat statBuf;

    cam = malloc (sizeof *cam);
    cam->width = width;
    cam->height = height;
    cam->exp = 0;
    cam->fx = 0;

    stat (path, &statBuf);
    if (S_ISDIR(statBuf.st_mode)) {
        cam->imageSequence = 1;
    } else {
        cam->imageSequence = 0;
    }
    cam->imageNumber = 0;

    cam->path = malloc (strlen (path) + 1);
    strcpy (cam->path, path);

    return cam;
}

extern void Camera_SetExposure (Camera cam, enum Camera_ExposureEnum exp) {
    cam->exp = exp;
}

extern void Camera_SetEffect (Camera cam, enum Camera_EffectEnum fx) {
    cam->fx = fx;
}

void Camera_Shoot (Camera cam) {
    Camera_SetCmd (cam);
    system (cam->cmd);
    if (cam->imageSequence) {
        cam->imageNumber++;
    }
}


/*-----------------------------------------------------------------------------
 *
 * ledgrid11.c
 *
 *     Die wohl ausgereifteste Applikation fuer die Animation einer gesamten
 *     LED-Matrix.
 *
 *     Unter Verwendung von 'ColorGrid' aus dem PiPack.
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <math.h>
#include <ncurses.h>
#include <libgen.h>
#include <pthread.h>
#include <assert.h>
#include <getopt.h>
#include <wiringPi.h>
#include "PiPack.h"

#define SENSOR_PIN 11
#undef  HAS_PHOTO_SENSOR

#define SIZE 10
#define LINE "----------------------------------------------------\n"

void debug (char *msg) {
    fprintf (stderr, "DEBUG: %s\n", msg);
}

int   fadeSteps;

//=========================================================================
// 
// Color functions
//
//-------------------------------------------------------------------------
//
// Aus (retourniert immer 0)
//
unsigned char colorFunc00 (ColorGrid cg, \
	int color, int x, int y, int step) {
    return 0;
}
//
// Fade der ganzen Flaeche
//
unsigned char colorFunc99 (ColorGrid cg, \
	int color, int x, int y, int step) {
    return ColorGrid_GetColor (cg, color, step);
}
//
// X-Achse
//
unsigned char colorFunc10 (ColorGrid cg, \
	int color, int x, int y, int step) {
    return ColorGrid_GetColor (cg, color, x*fadeSteps+step);
}
//
// Y-Achse
//
unsigned char colorFunc15 (ColorGrid cg, \
	int color, int x, int y, int step) {
    return ColorGrid_GetColor (cg, color, y*fadeSteps+step);
}
//
// Diagonal (links unten - rechts oben)
//
unsigned char colorFunc20 (ColorGrid cg, \
        int color, int x, int y, int step) {
    return ColorGrid_GetColor (cg, color, (x+y)*fadeSteps+step);
}
//
// Diagonal (links oben - rechts unten)
//
unsigned char colorFunc25 (ColorGrid cg, \
        int color, int x, int y, int step) {
    return ColorGrid_GetColor (cg, color, (SIZE+x-1-y)*fadeSteps+step);
}
//
// Karo
//
unsigned char colorFunc30 (ColorGrid cg, \
        int color, int x, int y, int step) {
    x = (x<SIZE/2) ? x : SIZE-x-1;
    y = (y<SIZE/2) ? y : SIZE-y-1;
    return ColorGrid_GetColor (cg, color, (x+y)*fadeSteps+step);
}
//
// Quadrat
//
int min (int a, int b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

unsigned char colorFunc35 (ColorGrid cg, \
        int color, int x, int y, int step) {
    x = (x<SIZE/2) ? x : SIZE-x-1;
    y = (y<SIZE/2) ? y : SIZE-y-1;
    return ColorGrid_GetColor (cg, color, 2*min(x,y)*fadeSteps+step);
}
//
// Kreis
//
unsigned char colorFunc40 (ColorGrid cg, \
        int color, int x, int y, int step) {
    int v;
    x = (x<SIZE/2) ? (SIZE/2-x-1) : x-SIZE/2;
    y = (y<SIZE/2) ? (SIZE/2-y-1) : y-SIZE/2;
    v = round (sqrt (x*x + y*y));
    return ColorGrid_GetColor (cg, color, 2*v*fadeSteps+step);
}

//
// Linie auf Y-Achse
//
unsigned char colorFunc50 (ColorGrid cg, \
        int color, int x, int y, int step) {
    int v;
    v = (x*fadeSteps+step)%(2*(SIZE-1)*SIZE);
    if ((v < 10) || ((v >= 90) && (v < 100))) {
        return 255;
    } else {
        return 0;
    }
}

//
// Linie auf X-Achse
//
unsigned char colorFunc51 (ColorGrid cg, \
        int color, int x, int y, int step) {
    int v;
    v = (y*fadeSteps+step)%(2*(SIZE-1)*SIZE);
    if ((v < 10) || ((v >= 90) && (v < 100))) {
        return 255;
    } else {
        return 0;
    }
}

//-----------------------------------------------------------------------------
//
// (main)
//
int main (int argc, char *argv[]) {
    const int DefaultSize      = 10;
    const int DefaultFadeSteps = 10;
    const int DefaultDelayTime = 50;

    float gammaValue    = 1.0;
    float expRedValue   = 1.0;
    float expGreenValue = 1.0;
    float expBlueValue  = 1.0;

    enum ModeEnum { MAIN_MODE, CMD_MODE, COLOR_MODE, FUNC_MODE, VALUE_MODE, \
            RANDOM_MODE, SAVE_MODE, SPEED_MODE, STEP_MODE };
    int mode;
    pthread_mutex_t animRunMutex;
#ifdef HAS_PHOTO_SENSOR
    pthread_mutex_t photoRunMutex;
#endif
    int running, animationRunning;
#ifdef HAS_PHOTO_SENSOR
    int photoRunning;
#endif
    int colorIndex, funcIndex;
    int delayTime;
    ColorGrid cg;

    // Animation thread. Animates the colors of the LED grid.
    //
    void *AnimationThreadFunc (void *arg) {
        ColorGrid cg;

        cg = (ColorGrid) arg;
        while (1) {
            if (! animationRunning) {
                pthread_mutex_lock (&animRunMutex);
            }
            ColorGrid_Fade (cg, 0);
            ColorGrid_Fade (cg, 1);
            ColorGrid_Fade (cg, 2);
            ColorGrid_SetColors (cg);
            ColorGrid_Show (cg);
            delay (delayTime);
        }
        return NULL;
    }

    // Random thread. Cycles through the different color functions and
    // change every 5 seconds the functions for all three colors.
    //
    void *RandomThreadFunc (void *arg) {
        ColorGrid cg;
        int fade, incr, shift;

        cg = (ColorGrid) arg;
        if (animationRunning) {
            animationRunning = 0;
            ColorGrid_SetImage (cg, 0, 100);
        }
        while (1) {
            clear ();
            printw ("----------------------------------------------\n");
            for (colorIndex=0; colorIndex<3; colorIndex++) {
                funcIndex = random () % ColorGrid_GetNumColorFuncs (cg);
                incr      = random () % 5 - 2;
                shift     = random () % 2;
                switch (colorIndex) {
                    case 0:
                        printw ("red   ");
                        break;
                    case 1:
                        printw ("green ");
                        break;
                    case 2:
                        printw ("blue  ");
                        break;
                }
                printw ("(%+2d, %d): %s\n", incr, shift, \
                        ColorGrid_GetColorFuncName (cg, funcIndex));
                ColorGrid_SetColorFunc (cg, colorIndex, funcIndex);
                ColorGrid_SetFadeIncr (cg, colorIndex, incr);
                ColorGrid_IncrFadeStep (cg, colorIndex, 90 * shift);
            }
            printw ("----------------------------------------------\n");
            printw ("[x]   Exit\n");
            printw ("----------------------------------------------\n");
            printw ("\nSelect command: \n");
            printw ("\nFade in...\n");
            refresh ();
            animationRunning = 1;
            pthread_mutex_unlock (&animRunMutex);
            for (fade=100; fade>=0; fade--) {
                ColorGrid_SetImage (cg, 0, fade);
                // ColorGrid_SetColors (cg);
                // ColorGrid_Show (cg);
                delay (40);
            }
            printw ("Let it run...\n");
            refresh ();
            delay (6000);
            printw ("Fade out...\n");
            refresh ();
            for (fade=0; fade<=100; fade++) {
                ColorGrid_SetImage (cg, 0, fade);
                // ColorGrid_SetColors (cg);
                // ColorGrid_Show (cg);
                delay (40);
            }
            animationRunning = 0;
            delay (500);
        }
        return NULL;
    }

#ifdef HAS_PHOTO_SENSOR
    // Photo thread. Adjusts the light according to the photo resistor.
    //
    void *PhotoThreadFunc (void *arg) {
        ColorGrid cg;
        PhotoSensor sen;
        int i, v1, v2;

        cg = (ColorGrid) arg;
        sen = PhotoSensor_Init (SENSOR_PIN);
        delay (1000);
        while (1) {
            if (! photoRunning) {
                pthread_mutex_lock (&photoRunMutex);
            }
            v1 = PhotoSensor_GetValue (sen);
            delay (1000);
            v2 = PhotoSensor_GetValue (sen);
            if ((v2 > 50000) && (v1 < 50000)) {
                for (i=0; i<3; i++) {
                    ColorGrid_Recalc (cg, i, 255, 2.0);
                }
            } else if ((v2 < 50000) && (v1 > 50000)) {
                for (i=0; i<3; i++) {
                    ColorGrid_Recalc (cg, i, 255, 1.8);
                }
            }
            v1 = v2;
        }
    }
#endif

    int i;
    pthread_t animationThread, randomThread;
#ifdef HAS_PHOTO_SENSOR
    pthread_t photoThread;
#endif

    // Signal handler for stopping all threads and turn LED's off.
    //
    void signalHandler (int arg) {
        int i;

        endwin ();
        pthread_cancel (animationThread);
        pthread_cancel (randomThread);

        for (i=0; i<3; i++) {
            ColorGrid_SetColorFunc (cg, i, 0);
        }
        ColorGrid_SetColors (cg);
        ColorGrid_Show (cg);
        delay (delayTime);
        ColorGrid_Free (cg);
        exit (0);
    }

    int opt;
    int optionIndex;
    static struct option longOptions[] = {
        {"delay",    required_argument, 0, 'd' },
        {"expRed",   required_argument, 0, 'R' },
        {"expGreen", required_argument, 0, 'G' },
        {"expBlue",  required_argument, 0, 'B' },
        {"file",     required_argument, 0, 'f' },
        {"gamma",    required_argument, 0, 'g' },
        {"help",     no_argument,       0, 'h' },
        {"steps",    required_argument, 0, 's' },
        {"target",   required_argument, 0, 't' },
        {0,          0,                 0, 0   }
    };

    char targetDir[PATH_MAX];
    char fileName[PATH_MAX];

    void usage () {
        fprintf (stderr, "usage: %s <options>\n", basename (argv[0]));
        fprintf (stderr, "  -h        --help\n");
        fprintf (stderr, "  -R <n>    --expRed=<n>\n");
        fprintf (stderr, "  -G <n>    --expGreen=<n>\n");
        fprintf (stderr, "  -B <n>    --expBlue=<n>\n");
        fprintf (stderr, "  -d <n>    --delay=<n>\n");
        fprintf (stderr, "  -f <file> --file=<file>\n");
        fprintf (stderr, "  -g <n>    --gamma=<n>\n");
        fprintf (stderr, "  -s <n>    --steps=<n>\n");
        fprintf (stderr, "  -t <dir>  --target=<dir>\n");
    }

    // ------------------------------------------------------------------------
    //
    // Start of program
    //
#ifdef HAS_PHOTO_SENSOR
    wiringPiSetup ();
#endif

    srandom (getpid () * getppid ());

    // Check command line arguments.
    //
    delayTime = DefaultDelayTime;
    fadeSteps = DefaultFadeSteps;
    strcpy (targetDir, "images");

    while ((opt = getopt_long (argc, argv, "d:f:g:hs:t:R:G:B:", longOptions, \
            &optionIndex)) != -1) {
        switch (opt) {
            case 'd':
                delayTime = atoi (optarg);
                break;
            case 'R':
                expRedValue = atof (optarg);
                break;
            case 'G':
                expGreenValue = atof (optarg);
                break;
            case 'B':
                expBlueValue = atof (optarg);
                break;
            case 'f':
                strcpy (fileName, optarg);
                break;
            case 'g':
                gammaValue = atof (optarg);
                break;
            case 'h':
                usage ();
                exit (0);
                break;
            case 's':
                fadeSteps = atoi (optarg);
                break;
            case 't':
                strcpy (targetDir, optarg);
                break;
            default:
                usage ();
                exit (1);
                break;
        }
    }

    if (optind < argc) {
        usage ();
        exit (1);
    }

    // Establish signal handler for SIGINT (Ctrl-C).
    //
    signal (SIGINT, signalHandler);

    // Initialize 'ncurses'.
    //
    initscr ();
    raw ();
    keypad (stdscr, TRUE);
    noecho ();

    cg = ColorGrid_Init (DefaultSize, fadeSteps, gammaValue);

    ColorGrid_NewImage (cg);

    ColorGrid_AddColorFunc (cg, colorFunc00, "Off");
    ColorGrid_AddColorFunc (cg, colorFunc10, "Fade along x axis");
    ColorGrid_AddColorFunc (cg, colorFunc15, "Fade along y axis");
    ColorGrid_AddColorFunc (cg, colorFunc20, "Diagonal (links unten - rechts oben)");
    ColorGrid_AddColorFunc (cg, colorFunc25, "Diagonal (links oben - rechts unten)");
    ColorGrid_AddColorFunc (cg, colorFunc30, "Karo");
    ColorGrid_AddColorFunc (cg, colorFunc35, "Quadrat");
    ColorGrid_AddColorFunc (cg, colorFunc40, "Kreis");
    ColorGrid_AddColorFunc (cg, colorFunc99, "Fade der ganzen Flaeche");

    ColorGrid_AddColorFunc (cg, colorFunc50, "Line on y axis");
    ColorGrid_AddColorFunc (cg, colorFunc51, "Line on x axis");

    ColorGrid_SetColorFunc (cg, 0, 0);
    ColorGrid_SetColorFunc (cg, 1, 0);
    ColorGrid_SetColorFunc (cg, 2, 0);

    ColorGrid_Recalc (cg, 0, 255, expRedValue);
    ColorGrid_Recalc (cg, 1, 255, expGreenValue);
    ColorGrid_Recalc (cg, 2, 255, expBlueValue);

    running          = 1;
    animationRunning = 0;
#ifdef HAS_PHOTO_SENSOR
    photoRunning     = 0;
#endif
    colorIndex       = 0;
    funcIndex        = 0;

    pthread_mutex_init (&animRunMutex, NULL);
    pthread_mutex_lock (&animRunMutex);
    pthread_create (&animationThread, NULL, &AnimationThreadFunc, cg);

#ifdef HAS_PHOTO_SENSOR
    pthread_mutex_init (&photoRunMutex, NULL);
    pthread_mutex_lock (&photoRunMutex);
    pthread_create (&photoThread, NULL, &PhotoThreadFunc, cg);
#endif

    mode = MAIN_MODE;

    // Enter main loop.
    //
    while (running) {
        int ch, k;
        double exp;

        switch (mode) {

            case MAIN_MODE:
                printw (LINE);
                for (i=0; i<3; i++) {
                    switch (i) {
                        case 0:
                            printw ("Red  : ");
                            break;
                        case 1:
                            printw ("Green: ");
                            break;
                        case 2:
                            printw ("Blue : ");
                            break;
                    }
                    printw ("%3d %+2d", ColorGrid_GetFadeStep (cg, i), \
                            ColorGrid_GetFadeIncr (cg, i));
                    funcIndex = ColorGrid_GetColorFunc (cg, i);
                    printw (" (%s)\n", \
                            ColorGrid_GetColorFuncName (cg, funcIndex));
                }
                printw (LINE);
                printw ("[R]    Modify function for red\n");
                printw ("[G]    Modify function for green\n");
                printw ("[B]    Modify function for blue\n");
                printw (LINE);
                printw ("[y/u]  Modify increase value for red (-1/+1)\n");
                printw ("[h/j]  Modify increase value for green (-1/+1)\n");
                printw ("[n/m]  Modify increase value for blue (-1/+1)\n");
                printw (LINE);
                printw ("[Y/U]  Decrease/Increase value for red\n");
                printw ("[H/J]  Decrease/Increase value for green\n");
                printw ("[N/M]  Decrease/Increase value for blue\n");
                printw (LINE);
                printw ("[C]    Set all values to zero\n");
                printw ("[s]    Increase one step\n");
                printw ("[a]    Stop/Start animation thread ");
                if (animationRunning) {
                    printw ("(on)\n");
                } else {
                    printw ("(off)\n");
                }
                printw ("[q/w]  Decrease/Increase gamma value (%f)\n", gammaValue);
                printw ("[o/p]  Decrease/Increase exponent value (%f)\n", expRedValue);
                printw ("[W]    Write color values to file\n");
                printw (LINE);
                printw ("[z]    Random mode\n");
                printw (LINE);
                printw ("[x]    Exit\n");
                printw (LINE);
                printw ("\nSelect command: ");

                refresh ();

                ch = wgetch (stdscr);
                switch (ch) {
                    case 'R':
                        colorIndex = 0;
                        mode = FUNC_MODE;
                        break;
                    case 'G':
                        colorIndex = 1;
                        mode = FUNC_MODE;
                        break;
                    case 'B':
                        colorIndex = 2;
                        mode = FUNC_MODE;
                        break;

                    case 'y':
                        ColorGrid_IncrFadeIncr (cg, 0, -1);
                        break;
                    case 'u':
                        ColorGrid_IncrFadeIncr (cg, 0, +1);
                        break;

                    case 'h':
                        ColorGrid_IncrFadeIncr (cg, 1, -1);
                        break;
                    case 'j':
                        ColorGrid_IncrFadeIncr (cg, 1, +1);
                        break;

                    case 'n':
                        ColorGrid_IncrFadeIncr (cg, 2, -1);
                        break;
                    case 'm':
                        ColorGrid_IncrFadeIncr (cg, 2, +1);
                        break;

                    case 'Y':
                        ColorGrid_IncrFadeStep (cg, 0, -1);
                        break;
                    case 'U':
                        ColorGrid_IncrFadeStep (cg, 0, +1);
                        break;

                    case 'H':
                        ColorGrid_IncrFadeStep (cg, 1, -1);
                        break;
                    case 'J':
                        ColorGrid_IncrFadeStep (cg, 1, +1);
                        break;

                    case 'N':
                        ColorGrid_IncrFadeStep (cg, 2, -1);
                        break;
                    case 'M':
                        ColorGrid_IncrFadeStep (cg, 2, +1);
                        break;

                    case 'C':
                        for (i=0; i<3; i++) {
                            ColorGrid_SetFadeIncr (cg, i, 0);
                            ColorGrid_SetFadeStep (cg, i, 0);
                        }
                        break;
                    case 's':
                        for (i=0; i<3; i++) {
                            ColorGrid_Fade (cg, i);
                        }
                        break;
                    case 'a':
                        if (! animationRunning) {
                            pthread_mutex_unlock (&animRunMutex);
                        }
                        animationRunning = !animationRunning;
                        break;
                    case 'q':
                        if (gammaValue > 1.0) {
                            gammaValue -= 0.1;
                            ColorGrid_SetGamma (cg, gammaValue);
                        }
                        break;
                    case 'w':
                        if (gammaValue < 2.5) {
                            gammaValue += 0.1;
                            ColorGrid_SetGamma (cg, gammaValue);
                        }
                        break;
                    case 'o':
                        if (expRedValue > 1.0) {
                            expRedValue -= 0.1;
                            ColorGrid_Recalc (cg, 0, 255, expRedValue);
                            ColorGrid_Recalc (cg, 1, 255, expRedValue);
                            ColorGrid_Recalc (cg, 2, 255, expRedValue);
                        }
                        break;
                    case 'p':
                        if (expRedValue < 2.5) {
                            expRedValue += 0.1;
                            ColorGrid_Recalc (cg, 0, 255, expRedValue);
                            ColorGrid_Recalc (cg, 1, 255, expRedValue);
                            ColorGrid_Recalc (cg, 2, 255, expRedValue);
                        }
                        break;
                    case 'W':
                        ColorGrid_WriteColorFile (cg, "colorValues.txt");
                        break;
                    case 'z':
                        mode = RANDOM_MODE;
                        break;
                    case 'x':
                        running = 0;
                        break;
                }
                ColorGrid_SetColors (cg);
                ColorGrid_Show (cg);
                clear ();
                break;

            case CMD_MODE:
                printw ("-----------------------------------------\n");
                printw ("[c]   Color mode (set color and function)\n");
                printw ("[f]   Fade step and increase mode\n");
                printw ("[v]   Value mode (set new color values)\n");
                printw ("-----------------------------------------\n");
                printw ("[a]   Stop/Start animation thread ");
                if (animationRunning) {
                    printw ("(on)\n");
                } else {
                    printw ("(off)\n");
                }
#ifdef HAS_PHOTO_SENSOR
                printw ("[p]   Stop/Start photo thread ");
                if (photoRunning) {
                    printw ("(on)\n");
                } else {
                    printw ("(off)\n");
                }
#endif
                printw ("-----------------------------------------\n");
                printw ("[z]   Random mode\n");
                printw ("-----------------------------------------\n");
                printw ("[w]   Write color values to file\n");
                printw ("-----------------------------------------\n");
                printw ("[x]   Exit\n");
                printw ("-----------------------------------------\n");
                printw ("\nSelect command: ");
                refresh ();
                ch = wgetch (stdscr);
                switch (ch) {
                    case 'c':
                        mode = COLOR_MODE;
                        break;
                    case 'f':
                        mode = STEP_MODE;
                        break;
                    case 'v':
                        mode = VALUE_MODE;
                        break;
                    case 'a':
                        if (! animationRunning) {
                            pthread_mutex_unlock (&animRunMutex);
                        }
                        animationRunning = !animationRunning;
                        break;
#ifdef HAS_PHOTO_SENSOR
                    case 'p':
                        if (! photoRunning) {
                            pthread_mutex_unlock (&photoRunMutex);
                        }
                        photoRunning = !photoRunning;
                        break;
#endif
                    case 'z':
                        mode = RANDOM_MODE;
                        break;
                    case 'w':
                        ColorGrid_WriteColorFile (cg, "colorValues.txt");
                        break;
                    case 'x':
                        running = 0;
                        break;
                }
                clear ();
                break;

            case COLOR_MODE:
                printw ("----------------------------------------------\n");
                for (i=0; i<3; i++) {
                    switch (i) {
                        case 0:
                            printw ("[r]ed   : ");
                            break;
                        case 1:
                            printw ("[g]reen : ");
                            break;
                        case 2:
                            printw ("[b]lue  : ");
                            break;
                    }
                    funcIndex = ColorGrid_GetColorFunc (cg, i);
                    printw ("[%c] %s\n", (funcIndex<26) ? 'a'+funcIndex \
                            : 'A'+(funcIndex-26), \
                            ColorGrid_GetColorFuncName (cg, funcIndex));
                }
                printw ("----------------------------------------------\n");
                printw ("\nSelect color [r,g,b] [x = Exit, s = Save]: ");
                refresh ();
                ch = wgetch (stdscr);
                switch (ch) {
                    case 'r':
                        colorIndex = 0;
                        mode = FUNC_MODE;
                        break;
                    case 'g':
                        colorIndex = 1;
                        mode = FUNC_MODE;
                        break;
                    case 'b':
                        colorIndex = 2;
                        mode = FUNC_MODE;
                        break;
                    case 's':
                        mode = SAVE_MODE;
                        break;
                    case 'x':
                        mode = CMD_MODE;
                        clear ();
                        break;
                }
                break;

            case FUNC_MODE:
                printw (LINE);
                for (k=0; k<ColorGrid_GetNumColorFuncs (cg); k++) {
                    printw ("[%c] %s\n", (k<26) ? k+'a' : (k-26)+'A', \
                            ColorGrid_GetColorFuncName (cg, k));
                }
                printw (LINE);
                printw ("\nSelect function [a-%c]: ", \
                        (k<26) ? (k-1)+'a' : (k-27)+'A');
                refresh ();
                ch = wgetch (stdscr);
                if ((ch >= 'a') && (ch <= 'z')) {
                    k = ch - 'a';
                } else if ((ch >= 'A') && (ch <= 'Z')) {
                    k = 26 + (ch - 'A');
                } else {
                    k = -1;
                }
                if (k >= 0 && k < ColorGrid_GetNumColorFuncs (cg)) {
                    funcIndex = k;
                    mode = MAIN_MODE;
                    ColorGrid_SetColorFunc (cg, colorIndex, funcIndex);
                    ColorGrid_SetColors (cg);
                    ColorGrid_Show (cg);
                }
                clear ();
                break;

            case VALUE_MODE:
                noraw ();
                echo ();
                printw (LINE);
                printw ("\nEnter new exponent: ");
                refresh ();
                wscanw (stdscr, "%lf", &exp);
                for (i=0; i<3; i++) {
                    ColorGrid_Recalc (cg, i, 255, exp);
                }
                ColorGrid_SetColors (cg);
                ColorGrid_Show (cg);
                raw ();
                noecho ();
                clear ();
                mode = MAIN_MODE;
                break;

            case RANDOM_MODE:
                pthread_create (&randomThread, NULL, &RandomThreadFunc, cg);
                refresh ();
                ch = wgetch (stdscr);
                if (ch == 'x') {
                    pthread_cancel (randomThread);
                    mode = MAIN_MODE;
                }
                clear ();
                break;
        }
    }

    signalHandler (0);

    return 0;
}


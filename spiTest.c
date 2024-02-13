/*
 * spiSpeed.c:
 *	Code to measure the SPI speed/latency.
 *	Copyright (c) 2014 Gordon Henderson
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://github.com/WiringPi/WiringPi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with wiringPi.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <getopt.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define	TRUE	(1==1)
#define	FALSE	(!TRUE)

#define NUM_LEDS      100
#define NUM_COLORS    3
#define	SPI_CHAN      0
#define	BUFFER_SIZE   NUM_LEDS*3

int main (int argc, char *argv[]) {
    const int DefWidth          = 10;
    const int DefHeight         = 10;
    const int DefDelayMs        = 1000;
    const int DefMinSpeed  = 4000000,
	      DefMaxSpeed  = 4000000,
	      DefSpeedIncr =  500000;
    const unsigned int DefColor = 0xff0000;

    int opt;
    int optionIndex;

    static int myFd ;

    int width = DefWidth,
	height = DefHeight,
	size, bufferSize;

    int delayMs = DefDelayMs;
    
    int speed,
	minSpeed  = DefMinSpeed,
	maxSpeed  = DefMaxSpeed,
	speedIncr = DefSpeedIncr;

    unsigned int color = DefColor;
    unsigned char redValue, greenValue, blueValue;

    unsigned int start, end, duration;
    int spiFail ;
    unsigned char *offData, *colorData, *pixelBuffer;
    int i, j;
  
    double timePerTransaction, perfectTimePerTransaction, dataSpeed ;

    static struct option longOptions[] = {
	{ "color",  required_argument, 0, 'c' },
        { "delay",  required_argument, 0, 'd' },
        { "height", required_argument, 0, 'H' },
        { "help",   no_argument,       0, 'h' },
        { "speed",  required_argument, 0, 's' },
        { "width",  required_argument, 0, 'W' },
        { 0,        0,                 0,  0  }
    };
    
    void usage() {
        fprintf(stderr, "usage: %s <options>\n", basename(argv[0]));
        fprintf(stderr, "  -h                  --help\n");
        fprintf(stderr, "  -s <min>:<max>:<i>  --speed=<min>:<max>:<i>\n");
        fprintf(stderr, "  -d <n>              --delay=<n>\n");
        fprintf(stderr, "  -c <rgb>            --color=<rgb>\n");
        fprintf(stderr, "  -W <n>              --width=<n>\n");
        fprintf(stderr, "  -H <n>              --height=<n>\n");
    }
    
    void spiSetup (int speed) {
        if ((myFd = wiringPiSPISetup(SPI_CHAN, speed)) < 0) {
            fprintf(stderr, "Can't open the SPI bus: %s\n", strerror(errno)) ;
            exit(EXIT_FAILURE) ;
        }
    }

    wiringPiSetup();

    while ((opt = getopt_long(argc, argv, "c:d:H:hs:W:", longOptions,
            &optionIndex)) != -1) {
	switch (opt) {
	case 'c':
	    color = atoi(optarg);
	    break;
	case 'd':
	    delayMs = atoi(optarg);
	    break;
	case 'H':
	    height = atoi(optarg);
	    break;
	case 'h':
	    usage();
	    exit(0);
	    break;
	case 's':
	    if (sscanf(optarg, "%d:%d:%d", &minSpeed, &maxSpeed,
		    &speedIncr) != 3) {
		fprintf(stderr, "wrong speed argument: '%s'\n", optarg);
		exit(1);
	    }
	    break;
	case 'W':
	    width = atoi(optarg);
	    break;
	default:
	    usage();
	    exit(1);
	    break;
	}
    }

    if (optind < argc) {
	usage();
	exit(1);
    }

    size = width * height;
    bufferSize = 3 * size;
    redValue =   (color & 0x00ff0000) >> 16;
    greenValue = (color & 0x0000ff00) >> 8;
    blueValue =  (color & 0x000000ff);

    //fprintf(stderr, "speed %d:%d:%d\n", minSpeed, maxSpeed, speedIncr);

    if ((offData = malloc(bufferSize)) == NULL) {
        fprintf(stderr, "Unable to allocate buffer: %s\n", strerror(errno)) ;
        exit(EXIT_FAILURE) ;
    }
    for (i = 0; i < bufferSize; i++) {
	offData[i] = 0x00;
    }
    if ((colorData = malloc(bufferSize)) == NULL) {
        fprintf(stderr, "Unable to allocate buffer: %s\n", strerror(errno)) ;
        exit(EXIT_FAILURE) ;
    }
    for (i = 0; i < size; i++) {
	colorData[3*i+0] = redValue;
	colorData[3*i+1] = greenValue;
	colorData[3*i+2] = blueValue;
    }
    if ((pixelBuffer = malloc(bufferSize)) == NULL) {
        fprintf(stderr, "Unable to allocate buffer: %s\n", strerror(errno)) ;
        exit(EXIT_FAILURE) ;
    }

    for (speed = minSpeed; speed <= maxSpeed; speed += speedIncr) {
        spiSetup(speed);
        printf("Testing at %d Hz\n", speed);
        for (i = 0; i < 256; i++) {
	    for (j = 0; j < size; j++) {
		colorData[3*j] = i;
	    }
	    if (wiringPiSPIDataWrite(SPI_CHAN, colorData, bufferSize) == -1) {
	        fprintf(stderr, "SPI failure: %s\n", strerror(errno)) ;
	        exit(1);
            }
	    delay(5);
	}
	delay(delayMs);
        close(myFd);
    }
    return 0;
}


/*
        printf ("Colors:\n");
    spiFail = FALSE ;
    for (colorIndex = 0; colorIndex < NUM_COLORS; colorIndex++) {
        color = colorList[colorIndex];
        printf ("  0x%x\n", color);
        for (i = 0; i < BUFFER_SIZE; i++) {
            pixelData[i] = colorList[colorIndex];
        }
        start = millis();
	if (wiringPiSPIDataRW (SPI_CHAN, pixelData, BUFFER_SIZE) == -1) {
	    printf ("SPI failure: %s\n", strerror (errno)) ;
	    spiFail = TRUE ;
	    break ;
	}
        end = millis ();
        duration = end - start;
        printf (">>> took %d ms\n", duration);
        delay(1000);
    }
    close (myFd) ;
  }

  return 0 ;
}
*/

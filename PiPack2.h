#ifndef PIPACK_INCLUDED
#define PIPACK_INCLUDED

/*-----------------------------------------------------------------------------
 *
 * LedStrip --
 *
 */
typedef struct LedStrip *LedStrip;

enum LedStrip_ColorIndexEnum {
    RED,
    GREEN,
    BLUE
};

extern LedStrip LedStrip_Init (int size, char *colorMapFile);
extern void     LedStrip_Free (LedStrip ls);
extern void     LedStrip_Show (LedStrip ls);

extern void     LedStrip_SetColor (LedStrip ls, int pixel,
        unsigned char red, unsigned char green, unsigned char blue);
extern void     LedStrip_SetColorValue (LedStrip ls, int pixel,
        enum LedStrip_ColorIndexEnum colorIndex, unsigned char value);
extern void     LedStrip_SetColorInt (LedStrip ls, int pixel,
        unsigned int value);
extern void     LedStrip_SetRed (LedStrip ls, int pixel,
        unsigned char red);
extern void     LedStrip_SetGreen (LedStrip ls, int pixel,
        unsigned char green);
extern void     LedStrip_SetBlue (LedStrip ls, int pixel,
        unsigned char blue);

extern unsigned char LedStrip_GetColorValue (LedStrip ls, int pixel,
        enum LedStrip_ColorIndexEnum colorIndex);
extern unsigned int  LedStrip_GetColorInt (LedStrip ls, int pixel);
extern unsigned char LedStrip_GetRed (LedStrip ls, int pixel);
extern unsigned char LedStrip_GetGreen (LedStrip ls, int pixel);
extern unsigned char LedStrip_GetBlue (LedStrip ls, int pixel);

extern void     LedStrip_Clear (LedStrip ls);

/*-----------------------------------------------------------------------------
 *
 * LedGrid --
 *
 *     Umfassenste Implementation einer LED-Matrix von N x M LED's.
 *
 */
typedef struct LedGrid *LedGrid;

extern LedGrid LedGrid_Init (int sizeX, int sizeY, char *colorMapFile);
extern void    LedGrid_Free (LedGrid lg);
extern void    LedGrid_Show (LedGrid lg);

extern void    LedGrid_SetColor (LedGrid lg, int x, int y,
        unsigned char red, unsigned char green, unsigned char blue);
extern void    LedGrid_SetColorValue (LedGrid lg, int x, int y,
        enum LedStrip_ColorIndexEnum colorIndex, unsigned char value);
extern void    LedGrid_SetColorInt (LedGrid lg, int x, int y, unsigned int value);
extern void    LedGrid_SetRed (LedGrid lg, int x, int y, unsigned char value);
extern void    LedGrid_SetGreen (LedGrid lg, int x, int y, unsigned char value);
extern void    LedGrid_SetBlue (LedGrid lg, int x, int y, unsigned char value);

extern unsigned char LedGrid_GetColorValue (LedGrid lg, int x, int y,
        enum LedStrip_ColorIndexEnum colorIndex);
extern unsigned int  LedGrid_GetColorInt (LedGrid lg, int x, int y);
extern unsigned char LedGrid_GetRed (LedGrid lg, int x, int y);
extern unsigned char LedGrid_GetGreen (LedGrid lg, int x, int y);
extern unsigned char LedGrid_GetBlue (LedGrid lg, int x, int y);

extern void    LedGrid_Clear (LedGrid lg);

#endif /* PIPACK_INCLUDED */


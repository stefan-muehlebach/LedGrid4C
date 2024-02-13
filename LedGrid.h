#ifndef LEDGRID_INCLUDED
#define LEDGRID_INCLUDED

typedef struct LedGrid *LedGrid;
typedef struct Palette *Palette;

/*-----------------------------------------------------------------------------
 *
 * LedGrid --
 *
 */
enum LedGrid_ColorIndexEnum {
    RED, GREEN, BLUE
};

/*
 * Create, delete and modify
 */
extern LedGrid LedGrid_Init (int nCols, int nRows);
extern void    LedGrid_Free (LedGrid lg);
extern void    LedGrid_SetGamma (LedGrid lg, float gamma);
extern void    LedGrid_SetPalette (LedGrid lg, Palette p);

/*
 * Showing and clearing
 */
extern void    LedGrid_Show (LedGrid lg);
extern void    LedGrid_Clear (LedGrid lg);

/*
 * Setting functions
 */
extern void    LedGrid_SetColor (LedGrid lg, int col, int row,
        unsigned char red, unsigned char green, unsigned char blue);
extern void    LedGrid_SetColorValue (LedGrid lg, int col, int row,
        enum LedGrid_ColorIndexEnum colorIndex, unsigned char value);
extern void    LedGrid_SetColorInt (LedGrid lg, int col, int row,
        unsigned int value);
extern void    LedGrid_SetColorPal (LedGrid lg, int col, int row,
        unsigned char palPos);

extern void    LedGrid_SetRed (LedGrid lg, int col, int row,
        unsigned char value);
extern void    LedGrid_SetGreen (LedGrid lg, int col, int row,
        unsigned char value);
extern void    LedGrid_SetBlue (LedGrid lg, int col, int row,
        unsigned char value);

/*
 * Getting functions
 */
extern unsigned char LedGrid_GetColorValue (LedGrid lg, int col, int row,
        enum LedGrid_ColorIndexEnum colorIndex);
extern unsigned int  LedGrid_GetColorInt (LedGrid lg, int col, int row);

extern unsigned char LedGrid_GetRed (LedGrid lg, int col, int row);
extern unsigned char LedGrid_GetGreen (LedGrid lg, int col, int row);
extern unsigned char LedGrid_GetBlue (LedGrid lg, int col, int row);

/*-----------------------------------------------------------------------------
 *
 * Palette --
 *
 */
extern Palette Palette_Init ();
extern void Palette_Free (Palette p);

extern void Palette_SetColor (Palette p, int colorPos,
        unsigned char red, unsigned char green, unsigned char blue);
extern void Palette_SetColorValue (Palette p, int colorPos,
        enum LedGrid_ColorIndexEnum colorIndex, unsigned char value);
extern void Palette_SetColorInt (Palette p, int colorPos,
        unsigned int value);

extern void Palette_Interpolate (Palette p, int colorPosFrom, int colorPosTo);

#endif /* LEDGRID_INCLUDED */


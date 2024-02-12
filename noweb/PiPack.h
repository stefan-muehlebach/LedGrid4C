#line 32 "PiPack.nw"
#ifndef PIPACK_INCLUDED
#define PIPACK_INCLUDED

#line 44 "PiPack.nw"
typedef struct LedStrip *LedStrip;

enum LedStrip_ColorIndexEnum {
    RED,
    GREEN,
    BLUE
};

extern LedStrip      LedStrip_Init (int size, float gammaValue);
extern void          LedStrip_Free (LedStrip ls);
extern void          LedStrip_Show (LedStrip ls);

extern void          LedStrip_SetColor (LedStrip ls, int pixel,
        unsigned char red, unsigned char green, unsigned char blue);
extern void          LedStrip_SetColorValue (LedStrip ls, int pixel,
        enum LedStrip_ColorIndexEnum colorIndex, unsigned char value);
extern void          LedStrip_SetValue (LedStrip ls, int pixel,
        unsigned char value);
extern void          LedStrip_SetRed (LedStrip ls, int pixel,
        unsigned char red);
extern void          LedStrip_SetGreen (LedStrip ls, int pixel,
        unsigned char green);
extern void          LedStrip_SetBlue (LedStrip ls, int pixel,
        unsigned char blue);
extern void          LedStrip_SetGamma (LedStrip ls, float gammaValue);

extern unsigned char LedStrip_GetColorValue (LedStrip ls, int pixel,
        enum LedStrip_ColorIndexEnum colorIndex);
extern unsigned char LedStrip_GetValue (LedStrip ls, int pixel);
extern unsigned char LedStrip_GetRed (LedStrip ls, int pixel);
extern unsigned char LedStrip_GetGreen (LedStrip ls, int pixel);
extern unsigned char LedStrip_GetBlue (LedStrip ls, int pixel);

extern void          LedStrip_PushColor (LedStrip ls,
        unsigned char red, unsigned char green, unsigned char blue);
extern void          LedStrip_PushColorValue (LedStrip ls,
        enum LedStrip_ColorIndexEnum colorIndex, unsigned char value);
extern void          LedStrip_PushValue (LedStrip ls, unsigned char value);

extern void          LedStrip_AllOn (LedStrip ls);
extern void          LedStrip_AllOff (LedStrip ls);

extern void          LedStrip_Fade (LedStrip ls);
extern void          LedStrip_Cycle (LedStrip ls);

#line 93 "PiPack.nw"
typedef struct LedGrid *LedGrid;

enum LedGrid_ShiftDirectionEnum {
    SHIFT_UP, SHIFT_DOWN, SHIFT_LEFT, SHIFT_RIGHT
};

extern LedGrid LedGrid_Init (int sizeX, int sizeY, float gammaValue);
extern void    LedGrid_Free (LedGrid lg);
extern void    LedGrid_Show (LedGrid lg);

extern void    LedGrid_SetColor (LedGrid lg, int x, int y,
        unsigned char red, unsigned char green, unsigned char blue);
extern void    LedGrid_SetColorValue (LedGrid lg, int x, int y,
        enum LedStrip_ColorIndexEnum colorIndex, unsigned char value);
extern void    LedGrid_SetValue (LedGrid lg, int x, int y, unsigned char value);
extern void    LedGrid_SetRed (LedGrid lg, int x, int y, unsigned char value);
extern void    LedGrid_SetGreen (LedGrid lg, int x, int y, unsigned char value);
extern void    LedGrid_SetBlue (LedGrid lg, int x, int y, unsigned char value);
extern void    LedGrid_SetColorInt (LedGrid lg, int x, int y, unsigned int value);
extern void    LedGrid_SetGamma (LedGrid lg, float gammaValue);

extern void    LedGrid_SetAllColor (LedGrid lg, unsigned char red,
        unsigned char green, unsigned char blue);
extern void    LedGrid_SetAllColorValue (LedGrid lg,
        enum LedStrip_ColorIndexEnum colorIndex, unsigned char value);

extern unsigned char LedGrid_GetColorValue (LedGrid lg, int x, int y,
        enum LedStrip_ColorIndexEnum colorIndex);
extern unsigned char LedGrid_GetValue (LedGrid lg, int x, int y);
extern unsigned char LedGrid_GetRed (LedGrid lg, int x, int y);
extern unsigned char LedGrid_GetGreen (LedGrid lg, int x, int y);
extern unsigned char LedGrid_GetBlue (LedGrid lg, int x, int y);

extern void          LedGrid_AllOn  (LedGrid lg);
extern void          LedGrid_AllOff (LedGrid lg);

extern int           LedGrid_NewImage (LedGrid lg);
extern int           LedGrid_LoadImage (LedGrid lg, char *fileName,
                             int imgIndex);
extern int           LedGrid_SaveImage (LedGrid lg, char *fileName,
                             int imgIndex);
extern void          LedGrid_SetImage (LedGrid lg, int imageIndex,
                             int fadeStep);
extern int           LedGrid_GetImageCount (LedGrid lg);
extern int           LedGrid_GetCurImage (LedGrid lg);

extern void          LedGrid_FadeImage (LedGrid lg);
extern void          LedGrid_InterpolateImage (LedGrid lg);

extern void          LedGrid_Clear (LedGrid lg);

extern void          LedGrid_Shift (LedGrid lg,
                             enum LedGrid_ShiftDirectionEnum direction,
                             int rotate);


#line 38 "PiPack.nw"
#endif /* PIPACK_INCLUDED */


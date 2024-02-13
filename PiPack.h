#ifndef PIPACK_INCLUDED
#define PIPACK_INCLUDED

/*-----------------------------------------------------------------------------
 *
 * Semaphore --
 *
 */
typedef struct Semaphore *Semaphore;

extern Semaphore Semaphore_Init (int count);
extern void      Semaphore_P    (Semaphore s);
extern void      Semaphore_V    (Semaphore s);

/*-----------------------------------------------------------------------------
 *
 * Button --
 *
 */
typedef struct Button *Button;

enum Button_EdgeTypeEnum {
    FALLING = 1,
    RISING,
    BOTH
};

extern Button Button_Init           (int pin);
extern int    Button_GetState       (Button btn);
extern void   Button_SetEdgeHandler (Button btn,
        enum Button_EdgeTypeEnum edgeType,
        void (*handler)(void *arg), void *arg);

/*-----------------------------------------------------------------------------
 *
 * Led --
 *
 */
typedef struct Led *Led;

enum Led_ModeEnum {
    OUT = 1,
    PWM
};

extern Led  Led_Init (int pin, enum Led_ModeEnum mode);
extern void Led_On   (Led led);
extern void Led_Off  (Led led);
extern void Led_Set  (Led led, int value);
extern int  Led_Get  (Led led);

/*-----------------------------------------------------------------------------
 *
 * LedCount --
 *
 */
typedef struct LedCount *LedCount;

extern LedCount LedCount_Init (int led01, int led02, int led04);
extern void     LedCount_Set  (LedCount lcn, int value);
extern int      LedCount_Get  (LedCount lcn);

/*-----------------------------------------------------------------------------
 *
 * LedFader --
 *
 */
typedef struct LedFader *LedFader;

extern LedFader LedFader_Init (int ledPin, int buttonPin);

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

/*-----------------------------------------------------------------------------
 *
 * LedGrid --
 *
 *     Umfassenste Implementation einer LED-Matrix von N x M LED's.
 *
 */
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

/*-----------------------------------------------------------------------------
 *
 * ColorGrid --
 *
 *     Implementation mit animierten Farbwechseln.
 *
 */
typedef struct ColorGrid *ColorGrid;
typedef unsigned char (ColorFunc) (ColorGrid cg, \
        int color, int x, int y, int step);

extern ColorGrid ColorGrid_Init (int size, int numFadeSteps, float gammaValue);
extern void ColorGrid_Free (ColorGrid cg);
extern void ColorGrid_SetGamma (ColorGrid cg, float gammaValue);

extern void ColorGrid_Recalc (ColorGrid cg, int color, double max, double exp);
extern void ColorGrid_WriteColorFile (ColorGrid cg, char *fileName);
extern unsigned char ColorGrid_GetColor (ColorGrid cg, int color, int step);

extern void ColorGrid_SetFadeIncr (ColorGrid cg, int color, int incr);
extern int  ColorGrid_GetFadeIncr (ColorGrid cg, int color);
extern void ColorGrid_IncrFadeIncr (ColorGrid cg, int color, int incrIncr);

extern void ColorGrid_SetFadeStep (ColorGrid cg, int color, int step);
extern int  ColorGrid_GetFadeStep (ColorGrid cg, int color);
extern void ColorGrid_IncrFadeStep (ColorGrid cg, int color, int stepIncr);

extern void ColorGrid_Fade (ColorGrid cg, int color);

extern void ColorGrid_SetColors (ColorGrid cg);
extern void ColorGrid_Show (ColorGrid cg);
extern void ColorGrid_SetColorFunc (ColorGrid cg, int color, int funcIndex);
extern int  ColorGrid_GetColorFunc (ColorGrid cg, int color);
extern char *ColorGrid_GetColorFuncName (ColorGrid cg, int funcIndex);
extern void ColorGrid_AddColorFunc (ColorGrid cg, ColorFunc func, char *name);
extern int  ColorGrid_GetNumColorFuncs (ColorGrid cg);

extern int  ColorGrid_NewImage (ColorGrid cg);
extern void ColorGrid_SetImage (ColorGrid cg, int imageIndex, int fadeStep);

/*-----------------------------------------------------------------------------
 *
 * LedRun --
 *
 */
typedef struct LedRun *LedRun;

extern LedRun LedRun_Init (LedStrip ls, enum LedStrip_ColorIndexEnum colorIndex,
        int position, int increment, int divisor);
extern void   LedRun_Free (LedRun lr);

extern void   LedRun_Next (LedRun lr);
extern void   LedRun_Set  (LedRun lr);

/*-----------------------------------------------------------------------------
 *
 * PhotoSensor --
 *
 */
typedef struct PhotoSensor *PhotoSensor;

extern PhotoSensor PhotoSensor_Init (int pin);;
extern int         PhotoSensor_GetValue  (PhotoSensor sen);
extern int         PhotoSensor_GetLight  (PhotoSensor sen);

/*-----------------------------------------------------------------------------
 *
 * Camera --
 *
 */
typedef struct Camera *Camera;

enum Camera_ExposureEnum {
    AUTO = 0,
    NIGHT,
    NIGHTPREVIEW,
    BACKLIGHT,
    SPOTLIGHT,
    SPORTS,
    SNOW,
    BEACH,
    VERYLONG,
    FIXEDFPS,
    ANTISHAKE,
    FIREWORKS
};

enum Camera_EffectEnum {
    NONE,
    NEGATIVE,
    SOLARISE,
    SKETCH,
    DENOISE,
    EMBOSS,
    OILPAINT,
    HATCH,
    GPEN,
    PASTEL,
    WATERCOLOUR,
    FILM,
    BLUR,
    SATURATION,
    COLOURSWAP,
    WASHEDOUT,
    POSTERISE,
    COLOURPOINT,
    COLOURBALANCE,
    CARTOON
};

extern Camera Camera_Init  (int width, int height, char *path);
extern void   Camera_SetExposure (Camera cam, enum Camera_ExposureEnum exp);
extern void   Camera_SetEffect (Camera cam, enum Camera_EffectEnum fx);
extern void   Camera_Shoot (Camera cam);

#endif /* PIPACK_INCLUDED */


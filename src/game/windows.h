//
// windows.h
//

#if 0	//TWO_MEG_VERSION
#define GAME_VIEW_WIDTH		(512)
#define GAME_VIEW_HEIGHT	(384)
#else
#define GAME_VIEW_WIDTH		(640)
#define GAME_VIEW_HEIGHT	(480)
#endif

extern void	InitWindowStuff(void);
extern void	DumpGWorld(GWorldPtr, WindowPtr);
extern void	DumpGWorld2(GWorldPtr, WindowPtr, Rect *);
extern void	DumpGWorld3(GWorldPtr, WindowPtr, Rect *, Rect *);
extern void	DumpGWorldToGWorld(GWorldPtr, GWorldPtr, Rect *, Rect *);
extern void	DoLockPixels(GWorldPtr);
extern void	DoUnlockPixels(GWorldPtr);
extern void	UpdateAnimWindow(void);
extern	pascal void DoBold (WindowPtr dlogPtr, short item);
extern	pascal void DoOutline (WindowPtr dlogPtr, short item);
extern	void Home(void);
extern	void DoCR(void);
extern	void InitThermometerWindow(Str255 s);
extern	void FillThermometerWindow(long percent);
extern	void KillThermometerWindow(void);
extern	void MakeFadeEvent(Boolean	fadeIn);

extern	void CleanupDisplay(void);
extern	void GammaFadeOut(void);
extern	void GammaFadeIn(void);
extern	void GammaOn(void);

extern	void GameScreenToBlack(void);
extern	void CleanScreenBorder(void);

extern	void GetWindowDrawInfo(WindowPtr w, u_short **pixelPtr, u_long *rowBytes);

void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect);
void DoLockPixels(GWorldPtr world);
void DoUnlockPixels(GWorldPtr world);


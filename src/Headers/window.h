//
// windows.h
//

#define GAME_VIEW_WIDTH		(640)
#define GAME_VIEW_HEIGHT	(480)

extern void	DumpGWorldToGWorld(GWorldPtr, GWorldPtr, Rect *, Rect *);
extern	void MakeFadeEvent(Boolean	fadeIn);
void Enter2D(void);
void Exit2D(void);
int GetNumDisplays(void);
void MoveToPreferredDisplay(void);
void SetFullscreenMode(bool enforceDisplayPref);

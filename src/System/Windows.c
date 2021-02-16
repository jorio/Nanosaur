/****************************/
/*        WINDOWS           */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include	"globals.h"
#include	"windows_nano.h"
#include	"misc.h"
#include "objects.h"
#include "file.h"
#include "input.h"

static void MoveFadeEvent(ObjNode *theNode);

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float	gFramesPerSecondFrac,gAdditionalClipping;
extern  WindowPtr				gCoverWindow;
extern	PrefsType	gGamePrefs;
extern	SDL_Window*				gSDLWindow;

/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

void MakeFadeEvent(Boolean	fadeIn)
{
#if ALLOW_FADE
ObjNode	*newObj;

	gNewObjectDefinition.genre = EVENT_GENRE;
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall = MoveFadeEvent;
	newObj = MakeNewObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;

	Render_SetWindowGamma(fadeIn ? 0 : 100);

	newObj->Flag[0] = fadeIn;
	if (fadeIn)
		newObj->SpecialF[0] = 0;
	else
		newObj->SpecialF[0] = 100;
#endif
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadeEvent(ObjNode *theNode)
{
	float percent;


			/* SEE IF FADE IN */

	if (theNode->Flag[0])
	{
		percent = theNode->SpecialF[0] += 130*gFramesPerSecondFrac;
		if (percent >= 100)													// see if @ 100%
		{
			percent = 100;
			DeleteObject(theNode);
		}
	}
	
			/* FADE OUT */
	else
	{
		percent = theNode->SpecialF[0] -= 130*gFramesPerSecondFrac;
		if (percent <= 0)													// see if @ 0%
		{
			percent = 0;
			DeleteObject(theNode);
		}
	}


	Render_SetWindowGamma(percent);
}





/*********************** DUMP GWORLD TO GWORLD **********************/
//
//    copies RECT to RECT
//

void DumpGWorldToGWorld(GWorldPtr thisWorld, GWorldPtr destWorld,Rect *srcRect,Rect *destRect)
{
PixMapHandle pm,pm2;
GDHandle	oldGD;
GWorldPtr	oldGW;

	GetGWorld (&oldGW,&oldGD);
	SetGWorld(destWorld,nil);
	ForeColor(blackColor);
	BackColor(whiteColor);

	SetGWorld(thisWorld,nil);
	ForeColor(blackColor);
	BackColor(whiteColor);

	pm = GetGWorldPixMap(thisWorld);
	GAME_ASSERT_MESSAGE(pm && *pm, "Source PixMap Handle or Ptr = Null?!");

	pm2 = GetGWorldPixMap(destWorld);
	GAME_ASSERT_MESSAGE(pm2 && *pm2, "Dest PixMap Handle or Ptr = Null?!");

	CopyBits(*pm, *pm2, srcRect, destRect, srcCopy, 0);

	SetGWorld(oldGW,oldGD);								// restore gworld
}



/*********************** SET FULLSCREEN MODE **********************/

void SetFullscreenMode(void)
{
	SDL_SetWindowFullscreen(
			gSDLWindow,
			gGamePrefs.fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP: 0);

	// Ensure the clipping pane gets resized properly after switching in or out of fullscreen mode
	int width, height;
	SDL_GetWindowSize(gSDLWindow, &width, &height);
	QD3D_OnWindowResized(width, height);

	SDL_ShowCursor(gGamePrefs.fullscreen? 0: 1);
}

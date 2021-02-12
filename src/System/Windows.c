/****************************/
/*        WINDOWS           */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include	<SDL.h>
#include	<stdlib.h>
#include	"globals.h"
#include	"windows_nano.h"
#include	"misc.h"
#include "objects.h"
#include "file.h"
#include "input.h"

#define	ALLOW_FADE		1

static void MoveFadeEvent(ObjNode *theNode);

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode	*gCurrentNode;
extern	float	gFramesPerSecondFrac,gAdditionalClipping;
extern  WindowPtr				gCoverWindow;
extern	PrefsType	gGamePrefs;

Boolean					gGammaIsOn = true;

/**************** GAMMA FADE IN *************************/

void GammaFadeIn(void)
{
	if (!gGamePrefs.allowGammaFade)
		return;

#if ALLOW_FADE	

#if 0
	if (gDisplayContext)
		if (!gGammaIsOn)
			DSpContext_FadeGammaIn(MONITORS_TO_FADE,nil);
#else
	for (int i = 0; i <= 100; i++) {
		SetWindowGamma(i);
		SDL_Delay(16);
	}
#endif
		
	gGammaIsOn = true;
#endif		
}

/**************** GAMMA FADE OUT *************************/

void GammaFadeOut(void)
{
	if (!gGamePrefs.allowGammaFade)
		return;

Rect	r;

#if ALLOW_FADE	
#if 0
	if (gDisplayContext)
		if (gGammaIsOn)
			DSpContext_FadeGammaOut(MONITORS_TO_FADE,nil);
#else
	for (int i = 100; i >= 1; i--) {
		SetWindowGamma(i);
		SDL_Delay(16);
	}
#endif
#endif	

	r = gCoverWindow->portRect;									// clear it
	SetPort(gCoverWindow);
	BackColor(blackColor);
	EraseRect(&r);
	
	gGammaIsOn = false;
}

/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

void MakeFadeEvent(Boolean	fadeIn)
{
	
ObjNode	*newObj;

	if (!gGamePrefs.allowGammaFade)
		return;

#if ALLOW_FADE			

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

	SetWindowGamma(fadeIn ? 0 : 100);
	
	newObj->Flag[0] = fadeIn;
	if (fadeIn)
		newObj->SpecialF[0] = 0;
	else
		newObj->SpecialF[0] = 100;
		
	gGammaIsOn = fadeIn;
#endif		
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadeEvent(ObjNode *theNode)
{
long	percent;
		
			/* SEE IF FADE IN */
			
	if (theNode->Flag[0])
	{
		percent = theNode->SpecialF[0] += 130*gFramesPerSecondFrac;
		if (percent >= 100)													// see if @ 100%
		{
			percent = 100;
			DeleteObject(theNode);
		}
		SetWindowGamma(percent);
#if 0 // ALLOW_FADE			
		if (gDisplayContext)
			DSpContext_FadeGamma(gDisplayContext,percent,nil);
#endif
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
		SetWindowGamma(percent);
#if 0 // ALLOW_FADE		
		if (gDisplayContext)
			DSpContext_FadeGamma(gDisplayContext,theNode->SpecialF[0],nil);	
#endif
	}
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


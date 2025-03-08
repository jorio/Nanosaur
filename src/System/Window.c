/****************************/
/*        WINDOWS           */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

static void MoveFadeEvent(ObjNode *theNode);

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


/************************** ENTER 2D *************************/

void Enter2D(void)
{
	// Linux: work around game window sent to background after showing a dialog box
	// Windows: work around alert box appearing behind game
#if !__APPLE__
	if (gSDLWindow)
	{
		SDL_SetWindowFullscreen(gSDLWindow, false);
		SDL_HideWindow(gSDLWindow);
		SDL_PumpEvents();
	}
#endif
}


/************************** EXIT 2D *************************/

void Exit2D(void)
{
#if !__APPLE__
	if (gSDLWindow)
	{
		SDL_PumpEvents();
		SDL_ShowWindow(gSDLWindow);
		SetFullscreenMode(false);
	}
#endif
}

/******************** GET DEFAULT WINDOW SIZE *******************/

void GetDefaultWindowSize(SDL_DisplayID display, int* width, int* height)
{
	const float aspectRatio = 4.0 / 3.0f;
	const float screenCoverage = .8f;

	SDL_Rect displayBounds = { .x = 0, .y = 0, .w = 640, .h = 480 };
	SDL_GetDisplayUsableBounds(display, &displayBounds);

	if (displayBounds.w > displayBounds.h)
	{
		*width = displayBounds.h * screenCoverage * aspectRatio;
		*height = displayBounds.h * screenCoverage;
	}
	else
	{
		*width = displayBounds.w * screenCoverage;
		*height = displayBounds.w * screenCoverage / aspectRatio;
	}
}

/********************** GET NUM DISPLAYS **********************/

int GetNumDisplays(void)
{
	int numDisplays = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&numDisplays);
	SDL_free(displays);
	return numDisplays;
}

/******************** MOVE WINDOW TO PREFERRED DISPLAY *******************/
//
// This works best in windowed mode.
// Turn off fullscreen before calling this!
//

void MoveToPreferredDisplay(void)
{
	if (gGamePrefs.displayNumMinus1 >= GetNumDisplays())
	{
		gGamePrefs.displayNumMinus1 = 0;
	}

	SDL_DisplayID display = gGamePrefs.displayNumMinus1 + 1;

	int w = 640;
	int h = 480;
	GetDefaultWindowSize(display, &w, &h);
	SDL_SetWindowSize(gSDLWindow, w, h);
	SDL_SyncWindow(gSDLWindow);

	int centered = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
	SDL_SetWindowPosition(gSDLWindow, centered, centered);
	SDL_SyncWindow(gSDLWindow);
}

/*********************** SET FULLSCREEN MODE **********************/

void SetFullscreenMode(bool enforceDisplayPref)
{
	if (!gGamePrefs.fullscreen)
	{
		SDL_SetWindowFullscreen(gSDLWindow, 0);
		SDL_SyncWindow(gSDLWindow);

		if (enforceDisplayPref)
		{
			MoveToPreferredDisplay();
		}
	}
	else
	{
		if (enforceDisplayPref)
		{
			SDL_DisplayID currentDisplay = SDL_GetDisplayForWindow(gSDLWindow);
			SDL_DisplayID desiredDisplay = gGamePrefs.displayNumMinus1 + 1;

			if (currentDisplay != desiredDisplay)
			{
				// We must switch back to windowed mode for the preferred monitor to take effect
				SDL_SetWindowFullscreen(gSDLWindow, false);
				SDL_SyncWindow(gSDLWindow);
				MoveToPreferredDisplay();
			}
		}

		// Enter fullscreen mode
		SDL_SetWindowFullscreen(gSDLWindow, true);
		SDL_SyncWindow(gSDLWindow);
	}

	// Ensure the clipping pane gets resized properly after switching in or out of fullscreen mode
	QD3D_OnWindowResized();

	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);

	if (gGamePrefs.fullscreen)
		SDL_HideCursor();
	else
		SDL_ShowCursor();
}

/****************************/
/*   	SPRITES.C	   	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

typedef struct
{
	int width;
	int height;
	uint8_t* pixels;
	uint8_t* mask;
} SpriteFrame;


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SPRITE_GROUPS		1
#define MAX_SHAPE_ANIMS			50


/*********************/
/*    VARIABLES      */
/*********************/

static int			gNumFrames[MAX_SPRITE_GROUPS];
static SpriteFrame	gSpriteFrames[MAX_SPRITE_GROUPS][MAX_SHAPE_ANIMS];


/******************* INIT SPRITE MANAGER *************************/

void InitSpriteManager(void)
{
	memset(gNumFrames, 0, sizeof(gNumFrames));
	memset(gSpriteFrames, 0, sizeof(gSpriteFrames));
}


/*************** LOAD SPRITE GROUP **************/

void LoadSpriteGroup(const char* groupName, short groupNum, int numFrames)
{
	GAME_ASSERT(groupNum >= 0);
	GAME_ASSERT(groupNum < MAX_SPRITE_GROUPS);

			/* SEE IF NUKE EXISTING */

	if (gNumFrames[groupNum])
		DisposeSpriteGroup(groupNum);

	GAME_ASSERT(gNumFrames[groupNum] == 0);

			/* LOAD ALL SPRITE FILES */

	for (int i = 0; i < numFrames; i++)
	{
		char path[256];
		OSErr err;
		uint8_t* pixels;
		TGAHeader header;

		snprintf(path, sizeof(path), ":sprites:%s%d.tga", groupName, 1000 + i);

		FSSpec spec;
		err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);
		GAME_ASSERT(noErr == err);

			/* READ TGA */

		err = ReadTGA(&spec, &pixels, &header, true);
		GAME_ASSERT(noErr == err);

			/* GENERATE MASK */

		bool hasMask = false;
		uint8_t* mask = (uint8_t*) NewPtr(header.width * header.height * 4);
		for (int p = 0; p < header.width*header.height; p++)
		{
			if (pixels[p*4 + 0] == 0)
			{
				hasMask = true;
				((uint32_t*) mask)[p] = 0xFFFFFFFF;
				((uint32_t*) pixels)[p] = 0;
			}
			else
			{
				((uint32_t*) mask)[p] = 0;
			}
		}

			/* TOSS MASK IF FULLY OPAQUE */

		if (!hasMask)
		{
			DisposePtr((Ptr) mask);
			mask = nil;
		}

			/* SAVE FRAME INFO */

		gSpriteFrames[groupNum][i].width = header.width;
		gSpriteFrames[groupNum][i].height = header.height;
		gSpriteFrames[groupNum][i].pixels = pixels;
		gSpriteFrames[groupNum][i].mask = mask;
		gNumFrames[groupNum]++;
	}
}


/**************** DISPOSE SPRITE GROUP ***********************/

void DisposeSpriteGroup(short groupNum)
{
	for (int i = 0; i < gNumFrames[groupNum]; i++)
	{
		if (gSpriteFrames[groupNum][i].pixels)
		{
			DisposePtr((Ptr) gSpriteFrames[groupNum][i].pixels);
		}

		if (gSpriteFrames[groupNum][i].mask)
		{
			DisposePtr((Ptr) gSpriteFrames[groupNum][i].mask);
		}
	}

	memset(&gSpriteFrames[groupNum], 0, sizeof(gSpriteFrames[groupNum]));
	gNumFrames[groupNum] = 0;
}	


/**************** DRAW SPRITE FRAME TO SCREEN ********************/

void DrawSpriteFrameToScreen(short group, int frame, int x, int y)
{
	GAME_ASSERT(frame >= 0);
	GAME_ASSERT(frame < gNumFrames[group]);


	SpriteFrame* sfh = &gSpriteFrames[group][frame];		// point to desired frame

	int width 	= sfh->width;								// get frame width
	int height 	= sfh->height;								// get frame height
	const uint32_t* srcMaskPtr	= (uint32_t *) sfh->mask;	// get frame mask
	const uint32_t* srcPtr		= (uint32_t *) sfh->pixels;	// get ptr to pixel data

			/*******************************/
			/* SEE IF USE CLIPPING VERSION */
			/*******************************/
			
	if ((x < 0) || ((x+width) > GAME_VIEW_WIDTH) || (y < 0) ||
		((y+height) >= GAME_VIEW_HEIGHT))
	{
		DoFatalAlert("Clipped sprite code not written yet!");
	}
			/**************/
			/* DON'T CLIP */
			/**************/
	else
	{
		uint32_t* destPtr = (uint32_t *)(gCoverWindowPixPtr + (y * GAME_VIEW_WIDTH) + x);		// calc start addr
	
	
				/* DRAW WITH MASK */
				
		if (srcMaskPtr)
		{
			for (int v = 0; v < height; v++)
			{
				for (int h = 0; h < width; h++)
				{
					destPtr[h] = (destPtr[h] & srcMaskPtr[h]) | srcPtr[h];
				}
				destPtr += GAME_VIEW_WIDTH;
				srcPtr += width;
				srcMaskPtr += width;
			}
		}
				/* DRAW NO MASK */
		else
		{
			for (int h = 0; h < height; h++)
			{
				memcpy(destPtr, srcPtr, width*4);
				destPtr += GAME_VIEW_WIDTH;
				srcPtr += width;
			}
		}
	}

	Rect damage = { y, x, y + height, x + width };
	DamagePortRegion(&damage);
}


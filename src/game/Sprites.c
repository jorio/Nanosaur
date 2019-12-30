/****************************/
/*   	SPRITES.C	   	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "misc.h"
#include "sprites.h"
#include "windows.h"

extern	short		gMainAppRezFile;
extern	u_short		*gCoverWindowPixPtr;
extern	u_long		gCoverWindowRowBytes,gCoverWindowRowBytes2;



/****************************/
/*    PROTOTYPES            */
/****************************/

static void DisassembleFrameFile(short groupNum);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SPRITE_GROUPS		50


/*********************/
/*    VARIABLES      */
/*********************/

ShapeTableHeader	gShapeTables[MAX_SPRITE_GROUPS];


/******************* INIT SPRITE MANAGER *************************/

void InitSpriteManager(void)
{
short	i;

	for (i=0; i < MAX_SPRITE_GROUPS; i++)
	{
		gShapeTables[i].numFrames = 0;
		gShapeTables[i].numAnims = 0;
		gShapeTables[i].frameHeaders = nil;
	}
}


/*************** LOAD FRAMES FILE **************/
//
// INPUT: 	inFile = spec of file to load
//			groupNum = group # to load file into
//

void LoadFramesFile(FSSpec *inFile, short groupNum)
{
QDErr		iErr;
short		fRefNum;

			/* SEE IF NUKE EXISTING */
			
	if (gShapeTables[groupNum].frameHeaders)
		DisposeSpriteGroup(groupNum);


				/* OPEN THE REZ-FORK */
			
	fRefNum = FSpOpenResFile(inFile,fsRdPerm);
	if (fRefNum == -1)
		DoFatalAlert("LoadFramesFile: Error opening Frames Rez file");
				
	UseResFile(fRefNum);
	if (iErr = ResError())
		DoFatalAlert("Error using Rez file!");

			/* EXTRACT INFO FROM FILE */

	DisassembleFrameFile(groupNum);

		
			/* CLOSE REZ FILE */
			
	CloseResFile(fRefNum);
	if (iErr = ResError())
		DoFatalAlert("Error closing Rez file!");
	
	UseResFile(gMainAppRezFile);		
}


/********************* DISASSEMBLE FRAME FILE *************************/

static void DisassembleFrameFile(short groupNum)
{
Handle					hand;
FramesFile_Header_Type	*headerPtr;
FrameHeaderType			*frameHeaderPtr;
long					i,w,h,j;
u_short					*pixelPtr,**destPixelHand,*destPixelPtr;
short					numAnims,numFrames,numLines;
ShapeFrameHeader		*sfh;
Boolean					hasMask;

			/* READ HEADER RESOURCE */

	hand = GetResource('Hedr',1000);
	if (hand == nil)
	{
		DoAlert("Error reading header resource!");
		return;
	}
	
	headerPtr = (FramesFile_Header_Type *)*hand;
	
	numAnims = headerPtr->numAnims;									// get # anims
	numFrames = headerPtr->numFrames;								// get # frames
	ReleaseResource(hand);

	if (numAnims > MAX_SHAPE_ANIMS)									// see if overload
		DoFatalAlert("numAnims > MAX_SHAPE_ANIMS");

	gShapeTables[groupNum].numFrames = numFrames;				// set # frames
	gShapeTables[groupNum].numAnims = numAnims;					// set # anims


			/* ALLOC MEM FOR SHAPE FRAME HEADERS */
				
	gShapeTables[groupNum].frameHeaders = (ShapeFrameHeader **)AllocHandle(sizeof(ShapeFrameHeader) * numFrames);
	if (gShapeTables[groupNum].frameHeaders == nil)
		DoFatalAlert("LoadFramesFile: AllocHandle failed");



		/***************/
		/* READ FRAMES */
		/***************/

	for (i=0; i < numFrames; i++)
	{
			/* READ HEADER */
			
		hand = GetResource('FrHd',1000+i);
		frameHeaderPtr = (FrameHeaderType *)*hand;
		
		sfh = (*gShapeTables[groupNum].frameHeaders)+i;		// get ptr to Shape Frame Header[i]
		sfh->width = frameHeaderPtr->width;						// get width
		sfh->height = frameHeaderPtr->height;					// get height
		sfh->xoffset = frameHeaderPtr->xoffset;					// get xoffset
		sfh->yoffset = frameHeaderPtr->yoffset;					// get yoffset
		hasMask = sfh->hasMask = frameHeaderPtr->hasMask;		// get hasMask
		ReleaseResource(hand);		

			
			/* READ PIXELS/MASK */
		
		hand = GetResource('Fram',1000+i);
		

		sfh = (*gShapeTables[groupNum].frameHeaders)+i;		// renew ptr
		w = sfh->width;			
		h = sfh->height;		
								
		if (sfh->hasMask)
			destPixelHand = sfh->pixelData = (u_short **)AllocHandle(w*h*2*2);	// alloc mem for pix & mask
		else
			destPixelHand = sfh->pixelData = (u_short **)AllocHandle(w*h*2);	// alloc mem for just pix
		
		pixelPtr = (u_short *)*hand;										// get ptr to file's data
		destPixelPtr = *destPixelHand;										// get ptr to dest 				


		for (j = 0; j < (w*h); j++)
		{
			*destPixelPtr++ = *pixelPtr++;									// get pixel
			if (hasMask)
				*destPixelPtr++ = *pixelPtr++;								// get mask
		}
		ReleaseResource(hand);				
	}



			/****************/
			/*  READ ANIMS  */
			/****************/
			
	for (i = 0; i < numAnims; i++)
	{
			/* READ ANIM HEADER */

		hand = GetResource('AnHd',1000+i);		
		numLines = gShapeTables[groupNum].anims[i].numLines =
					(**(AnimHeaderType **)hand).numLines;							// get # lines in anim
		ReleaseResource(hand);

			/* ALLOC MEMORY FOR ANIM DATA */
			
		gShapeTables[groupNum].anims[i].animData =
								(AnimEntryType **)AllocHandle(sizeof(AnimEntryType) * numLines);
		
			/* READ ANIM DATA */
	
		hand = GetResource('Anim',1000+i);
		**(gShapeTables[groupNum].anims[i].animData) = **(AnimEntryType **)hand;		
		BlockMove(*hand,*gShapeTables[groupNum].anims[i].animData,numLines*sizeof(AnimEntryType));
		ReleaseResource(hand);
	}

}



/**************** DISPOSE SPRITE GROUP ***********************/

void DisposeSpriteGroup(short groupNum)
{
long	numFrames,i,numAnims;
ShapeFrameHeader	*sfh;

	if (gShapeTables[groupNum].frameHeaders == nil)					// see if already nuked
		return;

				/* NUKE ALL OF THE FRAMES INFO */

	numFrames = gShapeTables[groupNum].numFrames;
	for (i = 0; i < numFrames; i++)
	{
		sfh = *gShapeTables[groupNum].frameHeaders;
		DisposeHandle((Handle)(sfh+i)->pixelData);						// nuke pixel data
	}
	DisposeHandle((Handle)gShapeTables[groupNum].frameHeaders);		// nuke array of frame headers		
	gShapeTables[groupNum].frameHeaders = nil;						// should set this to nil so we can tell it's an empty group
	
	
				/* NUKE ALL OF THE ANIM INFO */
				
	numAnims = 	gShapeTables[groupNum].numAnims;
	for (i = 0; i < numAnims; i++)
	{
		DisposeHandle((Handle)gShapeTables[groupNum].anims[i].animData);	// nuke anim data
	}
	
}	
	
	
	
/**************** DRAW SPRITE FRAME TO SCREEN ********************/
//
//
//

void DrawSpriteFrameToScreen(u_long group, u_long frame, long x, long y)
{
ShapeFrameHeader	*sfh;
u_long	width,height,h,v;
long	xoff,yoff;
Boolean	hasMask;
u_long	*srcPtr,*destPtr,*maskPtr,rowBytes;
u_short	i;

	if (frame >= gShapeTables[group].numFrames)
		DoFatalAlert("DrawSpriteFrameToScreen: illegal frame #");


	sfh = *gShapeTables[group].frameHeaders;						// get ptr to frame header array
	sfh += frame;													// point to the desired frame

	width 	= sfh->width;								// get frame width
	height 	= sfh->height;								// get frame height
	xoff 	= sfh->xoffset;								// get frame xoffset
	yoff 	= sfh->yoffset;								// get frame yoffset
	hasMask = sfh->hasMask;								// get frame has mask flag
	srcPtr 	= (u_long *)*(sfh->pixelData);				// get ptr to pixel/mask data

	x += xoff;											// adjust draw coords
	y += yoff;

	rowBytes = gCoverWindowRowBytes2/2;
	width /= 2;

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
		destPtr = (u_long *)(gCoverWindowPixPtr + (y * gCoverWindowRowBytes2) + x);		// calc start addr
	
	
				/* DRAW WITH MASK */
				
		if (hasMask)
		{
			maskPtr = srcPtr + (width*height);
			i = 0;
			for (v = 0; v < height; v++)
			{
				for (h = 0; h < width; h++)
				{
					destPtr[h] = (destPtr[h] & maskPtr[i]) | srcPtr[i];				// draw masked pixel
					i++;
				}
				destPtr += rowBytes;
			}
		}
				/* DRAW NO MASK */
		else
		{
			for (v = 0; v < height; v++)
			{
				for (h = 0; h < width; h++)
					destPtr[h] = srcPtr[h];		
				destPtr += rowBytes;
				srcPtr += width;
			}
		}
	}
}






	
	
	
	
	
	
	
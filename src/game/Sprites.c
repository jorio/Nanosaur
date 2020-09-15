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
#include "windows_nano.h"

extern	short		gMainAppRezFile;
extern	UInt32		*gCoverWindowPixPtr;



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
OSErr		iErr;
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
UInt16					*pixelPtr = nil;
UInt16					*maskPtr = nil;
Ptr						destPixelPtr = nil;
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
	
	headerPtr = structpack::UnpackObj<FramesFile_Header_Type>(*hand);
	
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
		frameHeaderPtr = structpack::UnpackObj<FrameHeaderType>(*hand);
		
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
								
		// --------- Begin source port mod ------------
		// Rewrote this part to unpack 16-bit (with optional 16-bit mask) to 32-bit ARGB
		pixelPtr = structpack::UnpackObj<UInt16>(*hand, w*h);				// get ptr to file's data
		if (sfh->hasMask)
			maskPtr = structpack::UnpackObj<UInt16>(*hand + w*h * 2, w*h);	// mask data is stored after color data
		else
			maskPtr = nil;

		sfh->pixelData = AllocHandle(w*h*4);								// alloc mem for pix & mask
		destPixelPtr = *sfh->pixelData;										// get ptr to dest

		for (j = 0; j < (w*h); j++)
		{
			UInt16 px = *pixelPtr++;
			UInt16 mask = hasMask ? ~(*maskPtr++) : 0xFFFF;	// opaque is stored as 0; we use the opposite convention for alpha
			destPixelPtr[0] = mask >> 8;					// alpha
			destPixelPtr[1] = ((px >> 10) & 0x1F) << 3;		// red
			destPixelPtr[2] = ((px >>  5) & 0x1F) << 3;		// green
			destPixelPtr[3] = ((px >>  0) & 0x1F) << 3;		// blue
			destPixelPtr += 4;
		}

		/*
		std::stringstream dumpFN;
		dumpFN << "spriteframe" << "_" << i << "_" << w << "x" << h << "_a" << (int)sfh->hasMask << ".tga";
		Pomme::DumpTGA(dumpFN.str().c_str(), sfh->width, sfh->height, (const char*)*sfh->pixelData);
		*/
		// --------- End source port mod ------------

		ReleaseResource(hand);				
	}



			/****************/
			/*  READ ANIMS  */
			/****************/
			// SOURCE PORT NOTE: Nanosaur doesn't use sprite animations at all. We could remove this and all associated anim structures.
			
	for (i = 0; i < numAnims; i++)
	{
			/* READ ANIM HEADER */

		hand = GetResource('AnHd',1000+i);		
		structpack::UnpackObj<AnimHeaderType>(*hand);
		numLines = gShapeTables[groupNum].anims[i].numLines =
					(**(AnimHeaderType **)hand).numLines;							// get # lines in anim
		ReleaseResource(hand);

			/* SEE IF WE HAVE ANIM DATA AT ALL */
			/* (Source port fix to avoid illegal memory access if 0 anims) */

		if (!numLines)
		{
			gShapeTables[groupNum].anims[i].animData = nil;
			continue;
		}

			/* ALLOC MEMORY FOR ANIM DATA */
			
		gShapeTables[groupNum].anims[i].animData =
								(AnimEntryType **)AllocHandle(sizeof(AnimEntryType) * numLines);
		
			/* READ ANIM DATA */
	
		hand = GetResource('Anim',1000+i);
		structpack::UnpackObj<AnimEntryType>(*hand, numLines);
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
		if (!(Handle)gShapeTables[groupNum].anims[i].animData)				// don't nuke if we had no animData (source port fix)
			continue;
		DisposeHandle((Handle)gShapeTables[groupNum].anims[i].animData);	// nuke anim data
	}
	
}	
	
	
	
/**************** DRAW SPRITE FRAME TO SCREEN ********************/
//
//
//
// Source port note: modified this to use ARGB32 as unpacked in DisassembleFrameFile.

void DrawSpriteFrameToScreen(UInt32 group, UInt32 frame, long x, long y)
{
ShapeFrameHeader	*sfh;
UInt32	width,height,h,v;
long	xoff,yoff;
Boolean	hasMask;
UInt32	*srcPtr,*destPtr;

	if (frame >= gShapeTables[group].numFrames)
		DoFatalAlert("DrawSpriteFrameToScreen: illegal frame #");


	sfh = *gShapeTables[group].frameHeaders;						// get ptr to frame header array
	sfh += frame;													// point to the desired frame

	width 	= sfh->width;								// get frame width
	height 	= sfh->height;								// get frame height
	xoff 	= sfh->xoffset;								// get frame xoffset
	yoff 	= sfh->yoffset;								// get frame yoffset
	hasMask = sfh->hasMask;								// get frame has mask flag
	srcPtr 	= (UInt32 *)*(sfh->pixelData);				// get ptr to pixel/mask data

	x += xoff;											// adjust draw coords
	y += yoff;

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
		destPtr = (UInt32 *)(gCoverWindowPixPtr + (y * GAME_VIEW_WIDTH) + x);		// calc start addr
	
	
				/* DRAW WITH MASK */
				
		if (hasMask)
		{
			for (v = 0; v < height; v++)
			{
				for (h = 0; h < width; h++)
				{
					if (*(char*)&srcPtr[h])											// alpha is not 0 (first component in ARGB32)
						destPtr[h] = srcPtr[h];										// draw masked pixel
				}
				destPtr += GAME_VIEW_WIDTH;
				srcPtr += width;
			}
		}
				/* DRAW NO MASK */
		else
		{
			for (v = 0; v < height; v++)
			{
				for (h = 0; h < width; h++)
					destPtr[h] = srcPtr[h];		
				destPtr += GAME_VIEW_WIDTH;
				srcPtr += width;
			}
		}
	}
	Pomme_SetPortDirty(true);
	//Pomme::DumpTGA("cover.tga", 640, 480, (const char*)gCoverWindowPixPtr);
}






	
	
	
	
	
	
	
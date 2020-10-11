/****************************/
/*      MISC ROUTINES       */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include <QD3D.h>
#include <QD3DErrors.h>
#include <math.h>

#include	"globals.h"
#include	"misc.h"
#include	"windows_nano.h"
#include "sound2.h"
#include "file.h"
#include "player_control.h"
#include 	"selfrundemo.h"
#include "objects.h"
#include "input.h"
#include "3dmf.h"
#include "skeletonobj.h"
#include "title.h"

#include <SDL.h> // source port addition for message boxes

extern	long		gOriginalSystemVolume;
extern	short		gMainAppRezFile;
extern	Boolean		gGameOverFlag,gAbortedFlag;
extern	Boolean		gUsingDSP;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	Boolean		gQD3DInitialized;
extern  WindowPtr				gCoverWindow;
extern  PrefsType	gGamePrefs;

/****************************/
/*    CONSTANTS             */
/****************************/

#define		ERROR_ALERT_ID		401

/**********************/
/*     VARIABLES      */
/**********************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;

unsigned long seed0 = 0, seed1 = 0, seed2 = 0;
unsigned long seed0_alt = 0, seed1_alt = 0, seed2_alt = 0;


/****************** DO SYSTEM ERROR ***************/

void ShowSystemErr(long err)
{
Str255		numStr;

	NumToStringC(err, numStr);
	DoAlert (numStr);
	CleanQuit();
}

/****************** DO SYSTEM ERROR : NONFATAL ***************/
//
// nonfatal
//
void ShowSystemErr_NonFatal(long err)
{
Str255		numStr;

	NumToStringC(err, numStr);
	DoAlert (numStr);
}

/*********************** DO ALERT *******************/

void DoAlert(const char* s)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, __func__, s, NULL);
}

		
/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* s)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, __func__, s, NULL);
	CleanQuit();
}

/*********************** DO FATAL ALERT 2 *******************/

void DoFatalAlert2(const char* s1, const char* s2)
{
	static char alertbuf[1024];
	snprintf(alertbuf, 1024, "%s\n%s", s1, s2);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, __func__, alertbuf, NULL);
	ExitToShell();
}


/************ CLEAN QUIT ***************/

void CleanQuit(void)
{
static	Boolean beenHere = false;

	if (!beenHere)
	{
		beenHere = true;
		
		UseResFile(gMainAppRezFile);
		
		StopAllEffectChannels();
		KillSong();
//		SetDefaultOutputVolume((gOriginalSystemVolume<<16)|gOriginalSystemVolume); // reset system volume

		TurnOffISp();
		
		ShowBugdomAd();
		
#if 0
		CleanupDisplay();								// unloads Draw Sprocket
		
		ISpStop();										// unload input sprocket		
#endif
		if (gQD3DInitialized)
			Q3Exit();
	}

	// Source port addition: save prefs before quitting if any setting was
	// changed without going through the settings screen (e.g. fullscreen mode)
	SavePrefs(&gGamePrefs);
	
	InitCursor();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	ExitToShell();		
}

/********************** WAIT **********************/

void Wait(long ticks)
{
long	start;
	
	start = TickCount();

	while (TickCount()-start < ticks); 

}


#if 0  // Source port removal - unused in game
/***************** NUM TO HEX *****************/
//
// NumToHex - fixed length, returns a C string
//

unsigned char *NumToHex(unsigned short n)
{
static unsigned char format[] = "0xXXXX";				// Declare format static so we can return a pointer to it.
static const char *conv = "0123456789ABCDEF";
short i;

	for (i = 0; i < 4; n >>= 4, ++i)
			format[5 - i] = conv[n & 0xf];
	return format;
}


/***************** NUM TO HEX 2 **************/
//
// NumToHex2 -- allows variable length, returns a ++PASCAL++ string.
//

unsigned char *NumToHex2(unsigned long n, short digits)
{
static unsigned char format[] = "_$XXXXXXXX";				// Declare format static so we can return a pointer to it
static const char *conv = "0123456789ABCDEF";
unsigned long i;

	if (digits > 8 || digits < 0)
			digits = 8;
	format[0] = digits + 1;							// adjust length byte of output string

	for (i = 0; i < digits; n >>= 4, ++i)
			format[(digits + 1) - i] = conv[n & 0xf];
	return format;
}


/*************** NUM TO DECIMAL *****************/
//
// NumToDecimal --  returns a ++PASCAL++ string.
//

unsigned char *NumToDec(unsigned long n)
{
static unsigned char format[] = "_XXXXXXXX";				// Declare format static so we can return a pointer to it
static const char *conv = "0123456789";
short		 i,digits;
unsigned long temp;

	if (n < 10)										// fix digits
		digits = 1;
	else if (n < 100)
		digits = 2;
	else if (n < 1000)
		digits = 3;
	else if (n < 10000)
		digits = 4;
	else if (n < 100000)
		digits = 5;
	else
		digits = 6;

	format[0] = digits;								// adjust length byte of output string

	for (i = 0; i < digits; ++i)
	{
		temp = n/10;
		format[digits-i] = conv[n-(temp*10)];
		n = n/10;
	}
	return format;
}

#endif




/******************** MY RANDOM LONG **********************/
//
// My own random number generator that returns a LONG
//
// NOTE: call this instead of MyRandomShort if the value is going to be
//		masked or if it just doesnt matter since this version is quicker
//		without the 0xffff at the end.
//

unsigned long MyRandomLong(void)
{
  return seed2 ^= (((seed1 ^= (seed2>>5)*1568397607UL)>>7)+
                   (seed0 = (seed0+1)*3141592621UL))*2435386481UL;
}
 

/******************** MY RANDOM LONG: ALT **********************/
//
// This is identical to one above except it uses different vars.
// The reason for this is so that random NON-INCIDENTAL numbers can
// be generated on one machine during a 2-player network game w/o
// either machine falling out of sync.
//

unsigned long MyRandomLong_Alt(void)
{
  return seed2_alt ^= (((seed1_alt ^= (seed2_alt>>5)*1568397607UL)>>7)+
                   (seed0_alt = (seed0_alt+1)*3141592621UL))*2435386481UL;
}


/************** RANDOM FLOAT ********************/
//
// returns a random float between 0 and 1
//

float RandomFloat(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;		
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f / (float)0xfff;					// get # between 0..1
	return(f);
} 
 


/**************** SET MY RANDOM SEED *******************/

void SetMyRandomSeed(unsigned long seed)
{
	seed0 = seed;
	seed1 = 0;
	seed2 = 0;	
	
	seed0_alt = seed;
	seed1_alt = 0;
	seed2_alt = 0;	
	
}

/**************** INIT MY RANDOM SEED *******************/

void InitMyRandomSeed(void)
{
	seed0 = 0x2a80ce30;
	seed1 = 0;
	seed2 = 0;	

	seed0_alt = 0x2a80ce30;
	seed1_alt = 0;
	seed2_alt = 0;	
}


/******************* FLOAT TO STRING *******************/

#if 0  // Source port removal
void FloatToString(float num, Str255 string)
{
Str255	sf;
long	i,f;

	i = num;						// get integer part
	
	
	f = (fabs(num)-fabs((float)i)) * 10000;		// reduce num to fraction only & move decimal --> 5 places	

	if ((i==0) && (num < 0))		// special case if (-), but integer is 0
	{
		string[0] = 2;
		string[1] = '-';
		string[2] = '0';
	}
	else
		NumToString(i,string);		// make integer into string
		
	NumToString(f,sf);				// make fraction into string
	
	string[++string[0]] = '.';		// add "." into string
	
	if (f >= 1)
	{
		if (f < 1000)
			string[++string[0]] = '0';	// add 1000's zero
		if (f < 100)
			string[++string[0]] = '0';	// add 100's zero
		if (f < 10)
			string[++string[0]] = '0';	// add 10's zero
	}
	
	for (i = 0; i < sf[0]; i++)
	{
		string[++string[0]] = sf[i+1];	// copy fraction into string
	}
}
#endif

/****************** ALLOC HANDLE ********************/

Handle	AllocHandle(long size)
{
Handle	hand;
OSErr	err;

	hand = NewHandleClear(size);					// alloc in APPL
	if (hand == nil)
	{
		hand = TempNewHandle(size,&err);			// try TEMP mem
		if (hand == nil)
		{
			DoFatalAlert("AllocHandle: failed!");
			return(nil);
		}
		else
			return(hand);							// use TEMP
	}

	return(hand);									// use APPL	
}


/****************** ALLOC PTR ********************/

Ptr	AllocPtr(long size)
{
Ptr	pr;

	pr = NewPtr(size);						// alloc in Application
	if (pr == nil)
	{
		pr = NewPtrSys(size);				// alloc in SYS
		if (pr == nil)
			DoFatalAlert("AllocPtr: failed!");	
		return(pr);
	}
	else
		return(pr);
}


/***************** P STRING TO C ************************/

void PStringToC(char *pString, char *cString)
{
Byte	pLength,i;

	pLength = pString[0];
	
	for (i=0; i < pLength; i++)					// copy string
		cString[i] = pString[i+1];
		
	cString[pLength] = 0x00;					// add null character to end of c string
}


#if 0
/***************** DRAW C STRING ********************/

void DrawCString(char *string)
{
	while(*string != 0x00)
		DrawChar(*string++);
}
#endif

#pragma mark -

/******************* VERIFY SYSTEM ******************/

void VerifySystem(void)
{
OSErr	iErr;
NumVersion	nVers;
long		createdDirID;


			/* CHECK PREFERENCES FOLDER */
			
	iErr = FindFolder(kOnSystemDisk,kPreferencesFolderType,kDontCreateFolder,		// locate the folder
					&gPrefsFolderVRefNum,&gPrefsFolderDirID);
	if (iErr != noErr)
		DoAlert("Warning: Cannot locate Preferences folder.");

	DirCreate(gPrefsFolderVRefNum,gPrefsFolderDirID,"Nanosaur",&createdDirID);		// make PillowF folder in there


				/* CHECK SOUND MANAGER 3.1 */

	nVers = SndSoundManagerVersion();
	if ((nVers.majorRev < 3) ||
		((nVers.majorRev == 3) && (nVers.minorAndBugRev < 1)))
		DoFatalAlert("This program requires Sound Manager 3.1 or better to run.");
		

}


#pragma mark -


/******************** REGULATE SPEED ***************/

void RegulateSpeed(short fps)
{
short	n;
static UInt32 oldTick = 0;
	
	n = 60 / fps;
	while ((TickCount() - oldTick) < n);			// wait for n ticks
	oldTick = TickCount();							// remember current time
}


/************* COPY PSTR **********************/

void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr)
{
short	dataLen = inSourceStr[0] + 1;
	
	BlockMoveData(inSourceStr, outDestStr, dataLen);
	outDestStr[0] = dataLen - 1;
}


/***************** APPLY FICTION TO DELTAS ********************/

void ApplyFrictionToDeltas(float f,TQ3Vector3D *d)
{
	if (d->x < 0.0f)
	{
		d->x += f;
		if (d->x > 0.0f)
			d->x = 0;
	}
	else
	if (d->x > 0.0f)
	{
		d->x -= f;
		if (d->x < 0.0f)
			d->x = 0;
	}

	if (d->z < 0.0f)
	{
		d->z += f;
		if (d->z > 0.0f)
			d->z = 0;
	}
	else
	if (d->z > 0.0f)
	{
		d->z -= f;
		if (d->z < 0.0f)
			d->z = 0;
	}
}

#if 0 // not needed by Nanosaur or reimplemented elsewhere by source port

#pragma mark -

/**************** DRAW PICTURE INTO GWORLD ***********************/
//
// Uses Quicktime to load any kind of picture format file and draws
// it into the GWorld
//
//
// INPUT: myFSSpec = spec of image file
//
// OUTPUT:	theGWorld = gworld contining the drawn image.
//

OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld)
{
OSErr						iErr;
GraphicsImportComponent		gi;
Rect						r;
ComponentResult				result;
	

			/* PREP IMPORTER COMPONENT */
			
	result = GetGraphicsImporterForFile(myFSSpec, &gi);		// load importer for this image file
	if (result != noErr)
	{
		DoAlert("DrawPictureIntoGWorld: GetGraphicsImporterForFile failed!");
		return(result);
	}
	GraphicsImportGetBoundsRect(gi, &r);					// get dimensions of image


			/* UPDATE PICT GWORLD */
	
	iErr = NewGWorld(theGWorld, 16, &r, nil, nil, 0);					// try app mem
	if (iErr)
	{
		iErr = NewGWorld(theGWorld, 16, &r, nil, nil, useTempMem);		// try sys mem
		if (iErr)
		{
			DoAlert("DrawPictureIntoGWorld: MakeMyGWorld failed");
			return(1);
		}
	}


			/* DRAW INTO THE GWORLD */
	
	DoLockPixels(*theGWorld);	
	GraphicsImportSetGWorld(gi, *theGWorld, nil);				// set the gworld to draw image into
	result = GraphicsImportDraw(gi);						// draw into gworld
	CloseComponent(gi);										// cleanup
	if (result != noErr)
	{
		DoAlert("DrawPictureIntoGWorld: GraphicsImportDraw failed!");
		ShowSystemErr(result);
		DisposeGWorld (*theGWorld);
		return(result);
	}
	return(noErr);
}

/**************** DRAW PICTURE TO SCREEN ***********************/
//
// Uses Quicktime to load any kind of picture format file and draws
//
//
// INPUT: myFSSpec = spec of image file
//

OSErr DrawPictureToScreen(FSSpec *myFSSpec, short x, short y)
{
OSErr						iErr;
GraphicsImportComponent		gi;
Rect						r;
ComponentResult				result;
GWorldPtr                   gworld;


			/* PREP IMPORTER COMPONENT */

	result = GetGraphicsImporterForFile(myFSSpec, &gi);		// load importer for this image file
	if (result != noErr)
	{
		DoAlert("DrawPictureIntoGWorld: GetGraphicsImporterForFile failed!");
		return(result);
	}
	GraphicsImportGetBoundsRect(gi, &r);					// get dimensions of image


			/* UPDATE PICT GWORLD */
	
	iErr = NewGWorld(&gworld, 16, &r, nil, nil, 0);				    	// try app mem
	if (iErr)
	{
		iErr = NewGWorld(&gworld, 16, &r, nil, nil, useTempMem);		// try sys mem
		if (iErr)
		{
			DoAlert("DrawPictureIntoGWorld: MakeMyGWorld failed");
			return(1);
		}
	}


			/* DRAW INTO THE GWORLD */
	
	DoLockPixels(gworld);	
	GraphicsImportSetGWorld(gi, gworld, nil);				// set the gworld to draw image into
	result = GraphicsImportDraw(gi);						// draw into gworld
	CloseComponent(gi);										// cleanup
	if (result != noErr)
	{
		DoAlert("DrawPictureIntoGWorld: GraphicsImportDraw failed!");
		ShowSystemErr(result);
		DisposeGWorld (gworld);
		return(result);
	}
	
	        /* DUMP TO SCREEN */
	        
	r.left += x;
	r.right += x;
	r.top += y;
	r.bottom += y;
    DumpGWorld2(gworld, gCoverWindow, &r);
	
	DisposeGWorld(gworld);

	return(noErr);
}

#endif









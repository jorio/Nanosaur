/****************************/
/*      MISC ROUTINES       */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include <TextUtils.h>
#include <QD3D.h>
#include <QD3DErrors.h>
#include <math.h>
#include <sound.h>
#include <folders.h>
#include <palettes.h>
#include <osutils.h>
#include <timer.h>
#include 	<DrawSprocket.h>
#include <InputSprocket.h>
#include 	<Gestalt.h>

#include	"globals.h"
#include	"misc.h"
#include	"windows.h"
#include "sound2.h"
#include "file.h"
#include "player_control.h"
#include 	"selfrundemo.h"
#include "objects.h"
#include "input.h"
#include "3dmf.h"
#include "skeletonobj.h"
#include "title.h"

extern	EventRecord	gTheEvent;
extern	unsigned long gOriginalSystemVolume;
extern	short		gMainAppRezFile;
extern	Boolean		gGameOverFlag,gAbortedFlag;
extern	Boolean		gUsingDSP;
extern	DSpContextReference 	gDisplayContext;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	Boolean		gISpActive,gQD3DInitialized;
extern  WindowPtr				gCoverWindow;

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

        /* REGISTRATION */


#define REG_LENGTH      12

Boolean     gGameIsRegistered = false;
float       gDemoVersionTimer = 0;


/**********************/
/*     PROTOTYPES     */
/**********************/

static Boolean VerifyAppleOEMMachine(void);
static OSStatus GetMacName (StringPtr *macName);
static Boolean ValidateRegistrationNumber(unsigned char *regInfo);
static void DoRegistrationDialog(unsigned char *out);
static void DoDemoExpiredScreen(void);


/****************** DO SYSTEM ERROR ***************/

void ShowSystemErr(long err)
{
Str255		numStr;

	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	UseResFile(gMainAppRezFile);
	NumToString(err, numStr);
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

	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	UseResFile(gMainAppRezFile);
	NumToString(err, numStr);
	DoAlert (numStr);
}

/*********************** DO ALERT *******************/

void DoAlert(Str255 s)
{
Boolean	oldISpFlag = gISpActive;

	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	TurnOffISp();										// MUST TURN OFF INPUT SPROK TO GET KEYBOARD EVENTS!!!
	UseResFile(gMainAppRezFile);
	InitCursor();
	ParamText(s,NIL_STRING,NIL_STRING,NIL_STRING);
	NoteAlert(ERROR_ALERT_ID,nil);

	if (oldISpFlag)
		TurnOnISp();									// resume input sprockets if needed	
}

		
/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(Str255 s)
{
OSErr	iErr;
	
	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	TurnOffISp();										// MUST TURN OFF INPUT SPROK TO GET KEYBOARD EVENTS!!!
	UseResFile(gMainAppRezFile);

	InitCursor();
	ParamText(s,NIL_STRING,NIL_STRING,NIL_STRING);
	iErr = NoteAlert(ERROR_ALERT_ID,nil);
	CleanQuit();
}

/*********************** DO FATAL ALERT 2 *******************/

void DoFatalAlert2(Str255 s1, Str255 s2)
{
	if (gDisplayContext)
		GammaOn();
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	TurnOffISp();										// MUST TURN OFF INPUT SPROK TO GET KEYBOARD EVENTS!!!
	UseResFile(gMainAppRezFile);
	InitCursor();
	ParamText(s1,s2,NIL_STRING,NIL_STRING);
	Alert(402,nil);
//	ShowMenuBar();
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
#if !OEM
		if (!gGameIsRegistered)
			ShowBugdomAd();
#endif		
		CleanupDisplay();								// unloads Draw Sprocket
		
		ISpStop();										// unload input sprocket		
		if (gQD3DInitialized)
			Q3Exit();
			
	    SaveDemoTimer();			
	}
	
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


/***************** NUM TO HEX *****************/
//
// NumToHex - fixed length, returns a C string
//

unsigned char *NumToHex(unsigned short n)
{
static unsigned char format[] = "0xXXXX";				// Declare format static so we can return a pointer to it.
char *conv = "0123456789ABCDEF";
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
static unsigned char format[] = "\p$XXXXXXXX";				// Declare format static so we can return a pointer to it
char *conv = "0123456789ABCDEF";
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
static unsigned char format[] = "\pXXXXXXXX";				// Declare format static so we can return a pointer to it
char *conv = "0123456789";
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
			DoFatalAlert("\pAllocHandle: failed!");
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
			DoFatalAlert("\pAllocPtr: failed!");	
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


/***************** DRAW C STRING ********************/

void DrawCString(char *string)
{
	while(*string != 0x00)
		DrawChar(*string++);
}

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
		DoAlert("\pWarning: Cannot locate Preferences folder.");

	DirCreate(gPrefsFolderVRefNum,gPrefsFolderDirID,"\pNanosaur",&createdDirID);		// make PillowF folder in there


			/* VERIFY MACHINE IS OEM */
		
#if 0			
#if OEM
	if (!VerifyAppleOEMMachine())
	{
		DoFatalAlert("\pSorry, but this free OEM version of Nanosaur is not licensed for this machine.  To get your own copy of Nanosaur, please visit www.pangeasoft.net");
	}
#endif
#endif

				/* CHECK SOUND MANAGER 3.1 */

	nVers = SndSoundManagerVersion();
	if ((nVers.majorRev < 3) ||
		((nVers.majorRev == 3) && (nVers.minorAndBugRev < 1)))
		DoFatalAlert("\pThis program requires Sound Manager 3.1 or better to run.");
		

}



/********************** CHECK GAME REGISTRATION *************************/

void CheckGameRegistration(void)
{
#if OEM

    gGameIsRegistered = true;

#else
OSErr   		iErr;
FSSpec 			spec;
Str255 			regFileName = "\p:Nanosaur:Info";
short			fRefNum;
long        	numBytes = REG_LENGTH;
unsigned char	regInfo[REG_LENGTH];

            /* GET SPEC TO REG FILE */

	iErr = FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, regFileName, &spec);
    if (iErr)
        goto game_not_registered;


            /*************************/
            /* VALIDATE THE REG FILE */
            /*************************/

            /* READ REG DATA */

    if (FSpOpenDF(&spec,fsCurPerm,&fRefNum) != noErr)
        goto game_not_registered;
    	
	FSRead(fRefNum,&numBytes,regInfo);
	
    FSClose(fRefNum);

            /* VALIDATE IT */

    if (!ValidateRegistrationNumber(&regInfo[0]))
        goto game_not_registered;        

    gGameIsRegistered = true;
    return;

        /* GAME IS NOT REGISTERED YET, SO DO DIALOG */

game_not_registered:

    DoRegistrationDialog(&regInfo[0]);

    if (gGameIsRegistered)                                  // see if write out reg file
    {
	    FSpDelete(&spec);	                                // delete existing file if any
	    iErr = FSpCreate(&spec,'MMik','xxxx',-1);
        if (iErr == noErr)
        {
        	numBytes = REG_LENGTH;
			FSpOpenDF(&spec,fsCurPerm,&fRefNum);
			FSWrite(fRefNum,&numBytes,regInfo);
		    FSClose(fRefNum);
     	}  
    }
    
            /* DEMO MODE */
    else
    {
		/* SEE IF TIMER HAS EXPIRED */

        GetDemoTimer();
    	if (gDemoVersionTimer > (60 * 30))		// let play for n minutes
    	{
    		DoDemoExpiredScreen();	
    		CleanQuit();
    	}
    }
#endif
}


/**************** DO DEMO EXPIRED SCREEN ***********************/

static void DoDemoExpiredScreen(void)
{
	ShowCursor();
	Alert(403,nil);
    CleanQuit();
}


/********************* VALIDATE REGISTRATION NUMBER ******************/

static Boolean ValidateRegistrationNumber(unsigned char *regInfo)
{
short   i,j;

            /* EXTRACT COMPONENTS */
    
    for (i = 0, j = REG_LENGTH-1; i < REG_LENGTH; i += 2, j -= 2)     // get checksum 
    {
        Byte    value,c,d;

		if ((regInfo[i] >= 'a') && (regInfo[i] <= 'z'))	// convert to upper case
			regInfo[i] = 'A' + (regInfo[i] - 'a');

		if ((regInfo[j] >= 'a') && (regInfo[j] <= 'z'))	// convert to upper case
			regInfo[j] = 'A' + (regInfo[j] - 'a');

        value = regInfo[i] - 'A';           // convert letter to digit 0..9
        c = ('Q' - regInfo[j]);             // convert character to number

        d = c - value;                      // the difference should be == i

        if (d != 0)
            return(false);
    }

    return(true);
}


/****************** DO REGISTRATION DIALOG *************************/

static void DoRegistrationDialog(unsigned char *out)
{
DialogPtr 		myDialog;
Boolean			dialogDone = false, isValid;
short			itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;
Str255          regInfo;

	InitCursor();
	
	FlushEvents ( everyEvent, REMOVE_ALL_EVENTS);
	myDialog = GetNewDialog(128,nil,MOVE_TO_FRONT);
	
				/* DO IT */
				
	while(dialogDone == false)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case	1:									        // Register
					GetDialogItem(myDialog,4,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,regInfo);
                    BlockMove(&regInfo[1], &regInfo[0], 100);         // shift out length byte

                    isValid = ValidateRegistrationNumber(regInfo);    // validate the number
    
                    if (isValid == true)
                    {
                        gGameIsRegistered = true;
                        dialogDone = true;
                        BlockMove(regInfo, out, REG_LENGTH);		// copy to output
                    }
                    else
                    {
                        DoAlert("\pSorry, that registration code is not valid.  Note that validation codes are case sensitive.  Please try again.");
                    }
					break;

            case    2:                                  // Demo
                    dialogDone = true;
                    break;
					
			case 	3:									// QUIT
                    CleanQuit();
					break;					
		}
	}
	DisposeDialog(myDialog);
	
	HideCursor();
}


#pragma mark -


/******************** REGULATE SPEED ***************/

void RegulateSpeed(short fps)
{
short	n;
static oldTick = 0;
	
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




#pragma mark -

#if OEM

#include <Gestalt.h>
#include <NameRegistry.h>
#include <string.h>

static OSStatus GetMacName (StringPtr *macName);


/*********************** VERIFY APPLE OEM MACHINE **************************/

static Boolean VerifyAppleOEMMachine(void)
{
Boolean					result 			= true;
OSStatus				err				= noErr;
StringPtr				macName			= nil,
						macNameGestalt	= nil;


				/*******************************/
				/* GET THE MACHINE NAME STRING */
				/*******************************/
				
//	err = Gestalt ('mnam', (long*)&macNameGestalt);
//	if (err == noErr)
//	{
//		/* Make a copy since we can't modify the returned value. */
//		/* Make sure we dispose of the copy when we are done with it. */
//		macName = (StringPtr)NewPtr (macNameGestalt[0] + 1);
//		if (macName != nil)
//			BlockMoveData (macNameGestalt, macName, macNameGestalt[0] + 1);
//	}
//	else
	{
		/* We have to dispose of macName when we are done with it. */
		err = GetMacName (&macName);
	}


		/*******************************************************/
		/* SEE IF MACHINE NAME MATCHES ANY OF THE OEM MACHINES */
		/*******************************************************/


	if (macName != nil)
	{
		if ((CompareString(macName, "\pPowerMac2,2", nil) == 0) 	||			// see if either machine name is an EXACT match
			(CompareString(macName, "\pPowerBook2,2", nil) == 0) 	||
			(CompareString(macName, "\pPowerMac2,1", nil) == 0) 	||
			(CompareString(macName, "\pPowerBook2,1", nil) == 0))
		{
			result = true;
		}
		else
			result = false;

		DisposePtr ((Ptr)macName);
		macName = nil;
	}
	return(result);
}

/* GetMacName returns a c formatted (null terminated string with the
 * model property string.
 * Input  macName - pointer to a buffer where the model property name
 *                  will be returned, if the call succeeds
 *
 * Output function result - noErr indicates that the model name was
 *                          read successfully
 *        macName - will contain the model name property if noErr
 *
 * Notes:
 *	  Caller is responsible for disposing of macName. Use DisposePtr.
 */
 
static OSStatus GetMacName (StringPtr *macName)
{
OSStatus                err = noErr;
RegEntryID              compatibleEntry;
RegPropertyValueSize	length;
RegCStrEntryNamePtr		compatibleValue;

	if (macName != nil)
	{
		*macName = 0;

	    err = RegistryEntryIDInit (&compatibleEntry);

	    if (err == noErr)
	        err = RegistryCStrEntryLookup (nil, "Devices:device-tree", &compatibleEntry);

		if (err == noErr)
			err = RegistryPropertyGetSize (&compatibleEntry, "compatible", &length);

		if (err == noErr)
		{
			compatibleValue = (RegCStrEntryNamePtr)NewPtr (length);
			err = MemError ();
		}

	    if (err == noErr)
	        err = RegistryPropertyGet (&compatibleEntry, "compatible", compatibleValue, &length);

		if (err == noErr)
		{
			SetPtrSize (compatibleValue, strlen (compatibleValue) + 1);
			/* SetPtrSize shouldn't fail because we are shrinking the pointer, but make sure. */
			err = MemError ();
		}

		if (err == noErr)
			*macName = c2pstr (compatibleValue);

	    (void)RegistryEntryIDDispose (&compatibleEntry);
	}

    return err;
}


#endif


#pragma mark -


/******************* GET DEMO TIMER *************************/

void GetDemoTimer(void)
{
OSErr				iErr;
short				refNum;
FSSpec				file;
long				count;

				/* READ TIMER FROM FILE */
					
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, "\pDriveData", &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);	
	if (iErr)
		gDemoVersionTimer = 0;
	else
	{
		count = sizeof(gDemoVersionTimer);
		iErr = FSRead(refNum, &count,  &gDemoVersionTimer);			// read data from file
		if (iErr)
		{
			FSClose(refNum);			
			FSpDelete(&file);										// file is corrupt, so delete
			gDemoVersionTimer = 0;
			return;
		}
		FSClose(refNum);			
	}	
	
}


/************************ SAVE DEMO TIMER ******************************/

void SaveDemoTimer(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

				/* CREATE BLANK FILE */
				
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, "\pDriveData", &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, '????', 'xxxx', smSystemScript);					// create blank file
	if (iErr)
		return;


				/* OPEN FILE */
					
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, "\pDriveData", &file);
	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
		return;

				/* WRITE DATA */
				
	count = sizeof(gDemoVersionTimer);
	FSWrite(refNum, &count, &gDemoVersionTimer);	
	FSClose(refNum);			
}


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
		DoAlert("\pDrawPictureIntoGWorld: GetGraphicsImporterForFile failed!");
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
			DoAlert("\pDrawPictureIntoGWorld: MakeMyGWorld failed");
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
		DoAlert("\pDrawPictureIntoGWorld: GraphicsImportDraw failed!");
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
		DoAlert("\pDrawPictureIntoGWorld: GetGraphicsImporterForFile failed!");
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
			DoAlert("\pDrawPictureIntoGWorld: MakeMyGWorld failed");
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
		DoAlert("\pDrawPictureIntoGWorld: GraphicsImportDraw failed!");
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










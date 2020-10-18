/****************************/
/*        WINDOWS           */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include 	<stdlib.h>
#include 	<QD3D.h>
#include	"globals.h"
#include	"windows_nano.h"
#include	"misc.h"
#include "objects.h"
#include "file.h"
#include "input.h"
#include "GamePatches.h"

#define	ALLOW_FADE		1

static void MoveFadeEvent(ObjNode *theNode);

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode	*gCurrentNode;
extern	float	gFramesPerSecondFrac,gAdditionalClipping;
extern  WindowPtr				gCoverWindow;
extern	PrefsType	gGamePrefs;

Boolean					gGammaIsOn = true;

#if 0

extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void PrepDrawSprockets(void);
static void MoveFadeEvent(ObjNode *theNode);
static Boolean SetupEventProc(EventRecord *event);
static pascal Boolean VideoModeTimeOut (DialogPtr dp,EventRecord *event, short *item);
static void DoVideoConfirmDialog(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	kQAVendor_ATI	1


#define	ALLOW_FADE		1


#if 0
#define MONITORS_TO_FADE	gDisplayContext
#else
#define MONITORS_TO_FADE	nil
#endif

/**********************/
/*     VARIABLES      */
/**********************/

long					gScreenXOffset,gScreenYOffset;
WindowPtr				gCoverWindow;
DSpContextReference 	gDisplayContext = nil;
Boolean					gLoadedDrawSprocket = false;
RgnHandle 				gMyRegion;

Boolean					gATI = false;

static unsigned long	gVideoModeTimoutCounter;
static Boolean			gVideoModeTimedOut;

UInt16					*gCoverWindowPixPtr;
UInt32					gCoverWindowRowBytes,gCoverWindowRowBytes2;

TQADevice	gATIRaveDevice;
TQAEngine	*gATIRaveEngine;
Boolean		gATIBadFog = false;
Boolean		gATIis431 = false;

Boolean					gGammaIsOn = true;


/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
GDHandle		mainScreen,selectedScreen;		
OSErr			iErr;
Rect			r;
long			width,height;
DisplayIDType	displayID;
DSpContextAttributes 	realConfig;
TQADevice	raveDevice;
TQAEngine	*myEngine;
UInt32	u32FastTexMem;
int		neededTexMem = 0x170000;			// need 1.5 megs free to play

			/* INIT WITH DRAW SPROCKETS */
			
	PrepDrawSprockets();


				/* CLEAR SCREEN & MAKE WINDOW */
				
	iErr = DSpContext_GetFrontBuffer(gDisplayContext,(CGrafPtr *)&gCoverWindow);
	if (iErr)
		DoFatalAlert("InitWindowStuff: DSpContext_GetFrontBuffer failed!");
		
		
				/* HACK TO FIX QD3D BUG */

	DSpContext_GetDisplayID(gDisplayContext,&displayID);			// get device which was selected
	DMGetGDeviceByDisplayID(displayID, &selectedScreen, true);
	mainScreen = GetMainDevice();									// get main monitor device		
	
	DSpContext_GetAttributes(gDisplayContext, &realConfig);			// get screen configuration	
	
	if ((mainScreen != selectedScreen) || (realConfig.displayWidth != GAME_VIEW_WIDTH) ||	// if not main monitor, or size isn't 640x480, then make new window to display into
		(realConfig.displayHeight != GAME_VIEW_HEIGHT)) 
	{
		r = (*selectedScreen)->gdRect;								// get global rect of screen
		
		width = r.right-r.left;										// calc a 512x384 window in center
		height = r.bottom-r.top;
		
		gScreenXOffset = (width-GAME_VIEW_WIDTH)/2;										// calc offsets to center it
		gScreenYOffset = (height-GAME_VIEW_HEIGHT)/2;	
		
		r.left += gScreenXOffset;
		r.right = r.left + GAME_VIEW_WIDTH; 
		r.top += (height-GAME_VIEW_HEIGHT)/2;
		r.bottom = r.top + GAME_VIEW_HEIGHT;

		gCoverWindow = NewCWindow(nil, &r, "", true, plainDBox,	// make new window to cover screen
								 MOVE_TO_FRONT, false, nil);
	
	}
	else
	{
		gScreenXOffset = 0;										// calc offsets to center it
		gScreenYOffset = 0;	
	}
		
	SetPort(gCoverWindow);
	ForeColor(whiteColor);
	BackColor(blackColor);

			/* CALC DISPLAY INFO */

	r = gCoverWindow->portRect;
	width = r.right-r.left;
	height = r.bottom-r.top;
	
	GetWindowDrawInfo(gCoverWindow, &gCoverWindowPixPtr, &gCoverWindowRowBytes);
	gCoverWindowRowBytes2 = gCoverWindowRowBytes/2;

	gCoverWindowPixPtr +=  (gScreenYOffset*gCoverWindowRowBytes2) + gScreenXOffset;	// point to our upper/left corner, not the window's
	

			/*************************************************/
			/* DETERMINE 3D HARDWARE INFO ABOUT THIS DISPLAY */
			/*************************************************/

	if (gDisplayContext)
	{
		DisplayIDType	displayID;
		long		response;
		
				/* DO ATI VERIFICATION */

		DSpContext_GetDisplayID(gDisplayContext,&displayID);						// get gdevice which was selected
		DMGetGDeviceByDisplayID(displayID, &raveDevice.device.gDevice, true);
		raveDevice.deviceType = kQADeviceGDevice;

		for (myEngine = QADeviceGetFirstEngine(&raveDevice); myEngine; myEngine = QADeviceGetNextEngine(&raveDevice,myEngine))
		{
			if (QAEngineGestalt(myEngine, kQAGestalt_VendorID, &response) == kQANoErr)
				if (response == kQAVendor_ATI)
				{
					gATIRaveDevice = raveDevice;
					gATIRaveEngine = myEngine;
					gATI = true;
					break;
				}				
		}

#if WARN_ATI						
		if (!gATI)
			DoAlert("For best results, Nanosaur should be run with an ATI 3D card.");
#endif		
		
			/*********************************************/
			/* SEE IF USING BAD VERSION OF ATI 3D DRIVER */
			/*********************************************/
			//
			// Here I check for version 4.2.8 which is the only version that works 100%.
			// Previous versions of the ATI Driver cannot do fog and alpha simulaneously.
			//
			// v4.2.2 has a bug in the kQAGestalt_FastTextureMemory call which reports 8 megs more than it really has.
			// If 4.2.2 is running, I don't allow fog and I tweak the needed texture memory by 8 megs.
			//
				
		if (gATI)
		{
			long	 major,minor;
						
					/* GET ATI 3D VERSION NUMBER */
					
			QAEngineGestalt(myEngine, kQAGestalt_EngineID, &major);
			QAEngineGestalt(myEngine, kQAGestalt_Revision, &minor);

					/* SEE IF PRE 4.0 */
					
			if (major < 4)
				DoFatalAlert("This version of the ATI 3D Accelerator Extension is ancient.  Get the latest version from ATI");
				
					/* SEE IF EARLIER THAN 4.3 */

			if ((major == 4) && (minor < 30))
			{
				gATIBadFog = true;								// fog is bad prior to 4.30
				
						/* SEE IF 4.2.2 */
						
				if (minor == 22)
					neededTexMem += 0x800000;					// adjust memory needs
			}
			
					/* EQUAL OR LATER THAN 4.30 */
			else
			{
				if ((major == 4) && (minor >= 31))
					gATIis431 = true;
				else
				if (major > 4)
					gATIis431 = true;
			}
		}

				/* NOW SEE IF CARD HAS ENOUGH VRAM */
				
#if !TWO_MEG_VERSION	
		if (myEngine)
			if (QAEngineGestalt(myEngine, kQAGestalt_FastTextureMemory,&u32FastTexMem ) == kQANoErr)
			{
				if (u32FastTexMem < neededTexMem)
					DoFatalAlert("Sorry, but you don't seem to have enough VRAM installed on this 3D accelerator card.  This version of Nanosaur requires 4 Megs of VRAM.");
			}
#endif			
	}
}


/*==============================================================================
* Dobold ()
* this is the user item procedure to make the thick outline around the default
* button (assumed to be item 1)
*=============================================================================*/

pascal void DoBold (WindowPtr dlogPtr, short item)
{
short		itype;
Handle		ihandle;
Rect		irect;

	GetDialogItem (dlogPtr, 1, &itype, (Handle *)&ihandle, &irect);	/* get the buttons rect */
	PenSize (3, 3);											/* make thick lines	*/
	InsetRect (&irect, -4, -4);							/* grow rect a little   */
	FrameRoundRect (&irect, 16, 16);						/* frame the button now */
	PenNormal ();
}

/*==============================================================================
* DoOutline ()
* this is the user item procedure to make the thin outline around the given useritem
*=============================================================================*/

pascal void DoOutline (WindowPtr dlogPtr, short item)
{
short		itype;
Handle		ihandle;
Rect		irect;

	GetDialogItem (dlogPtr, item, &itype, (Handle *)&ihandle, &irect);	// get the user item's rect 
	FrameRect (&irect);						// frame the button now 
	PenNormal();
}


/*************** HOME *****************/

void Home(void)
{
	Move(10,10);
}

/************* DO CR *****************/

void DoCR(void)
{
Point	pt;
GrafPtr	port;
Rect	r;

	GetPen(&pt);
	MoveTo(10,pt.v+10);
	
	GetPort(&port);
	r = port->portRect;
	GetPen(&pt);
	if (pt.v >= (r.bottom-20))
	{
		ScrollRect(&r, 0, -10, nil);
		Move(0,-10);
	}
}


/****************** PREP DRAW SPROCKETS *********************/

static void PrepDrawSprockets(void)
{
DSpContextAttributes 	displayConfig,realConfig;
OSStatus 				theError;
Boolean					canSelect,confirmIt = false;

		/* startup DrawSprocket */

	theError = DSpStartup();
	if( theError )
	{
		DoFatalAlert("DSpStartup failed!");
	}
	gLoadedDrawSprocket = true;


				/*************************/
				/* SETUP A REQUEST BLOCK */
				/*************************/
		
	displayConfig.frequency					= 00;
	displayConfig.displayWidth				= GAME_VIEW_WIDTH;
	displayConfig.displayHeight				= GAME_VIEW_HEIGHT;
	displayConfig.reserved1					= 0;
	displayConfig.reserved2					= 0;
	displayConfig.colorNeeds				= kDSpColorNeeds_Require;
	displayConfig.colorTable				= NULL;
	displayConfig.contextOptions			= 0; //kDSpContextOption_QD3DAccel;
	displayConfig.backBufferDepthMask		= kDSpDepthMask_1;
	displayConfig.displayDepthMask			= kDSpDepthMask_16;
	displayConfig.backBufferBestDepth		= 1;
	displayConfig.displayBestDepth			= 16;
	displayConfig.pageCount					= 1;
	displayConfig.gameMustConfirmSwitch		= false;
	displayConfig.reserved3[0]				= 0;
	displayConfig.reserved3[1]				= 0;
	displayConfig.reserved3[2]				= 0;
	displayConfig.reserved3[3]				= 0;
			
				/* SEE IF LET USER SELECT DISPLAY */

	DSpCanUserSelectContext(&displayConfig, &canSelect);							// see if can do it
	if (canSelect)
	{
		InitCursor();
		theError = DSpUserSelectContext(&displayConfig,nil,nil,&gDisplayContext);	// let user select it
		if (theError)									
			CleanQuit();
		HideCursor();
		if (gDisplayContext == nil)													// see if something went horribly wrong
			CleanQuit();
	}
				/* AUTOMATICALLY FIND BEST CONTEXT */
	else
	{
findit:	
		theError = DSpFindBestContext( &displayConfig, &gDisplayContext );
		if (theError)
		{
			DoFatalAlert("PrepDrawSprockets: DSpFindBestContext failed");
		}
	}
				/* RESERVE IT */

	theError = DSpContext_Reserve( gDisplayContext, &displayConfig );
	if( theError )
		DoFatalAlert("PrepDrawSprockets: DSpContext_Reserve failed");
		
			/* MAKE STATE ACTIVE */
	
	theError = DSpContext_SetState( gDisplayContext, kDSpContextState_Active );
	if (theError == kDSpConfirmSwitchWarning)
	{
		confirmIt = true;
	}
	else
	if (theError)
	{
		DSpContext_Release( gDisplayContext );
		gDisplayContext = nil;
		DoFatalAlert("PrepDrawSprockets: DSpContext_SetState failed");
		return;
	}

			/* GET ATTRIBS OF THE CONTEXT */

	DSpContext_GetAttributes(gDisplayContext, &realConfig);
	confirmIt = displayConfig.gameMustConfirmSwitch;			//--- override setting from above cuz Carry has bug

#if ALLOW_FADE	
	DSpContext_FadeGamma(MONITORS_TO_FADE,100,nil);
#endif	
}


/**************** DO VIDEO CONFIRM DIALOG *********************/

static void DoVideoConfirmDialog(void)
{
DialogPtr			myDialog;
UniversalProcPtr	myProc;
Boolean				dialogDone;
short				itemHit;

#if ALLOW_FADE	
	DSpContext_FadeGamma(MONITORS_TO_FADE,100,nil);						// make sure we can see it
#endif

	myDialog = GetNewDialog(128,nil,MOVE_TO_FRONT);

	dialogDone = false;
	gVideoModeTimedOut = false;
	
	myProc = NewModalFilterProc(VideoModeTimeOut);
	gVideoModeTimoutCounter = TickCount();					// get start time for timeout clock
	
	
	while(!dialogDone)
	{
		InitCursor();	
		ModalDialog(myProc, &itemHit);
		switch (itemHit)
		{
			case	2:				
					dialogDone = true;
					break;	
		}
		
		if (gVideoModeTimedOut)								// see if timed out
			break;
	}

	DisposeRoutineDescriptor(myProc);
	DisposeDialog(myDialog);
	HideCursor();	
}

/****************** VIDEO MODE TIMOUT *************************/
//
// Callback routine from above
//

static pascal Boolean VideoModeTimeOut (DialogPtr dp,EventRecord *event, short *item)
{
Boolean handled = false;
long			tick;
static long		oldTick = -1;
short			itemType;
Handle			itemHandle;
Rect			itemRect;
Str255			s;

			/* HANDLE DIALOG EVENTS */
#if 0			
	switch (event->what)
	{
		case keyDown:
			switch (event->message & charCodeMask)
			{
				case 	0x03:  					// Enter
				case 	0x0D: 					// Return
						*item = 1;
						handled = true;
						break;				
			}
	}
#endif	
				/* CHECK TIMEOUT */
			
	tick = 	TickCount() - gVideoModeTimoutCounter;
	tick /= 60;
	tick = 5-tick;
	
	if (tick <= 0)
	{
		gVideoModeTimedOut = true;
		handled = true;
		SysBeep(0);
	}
	
			/* UPDATE TIMER VALUE */
			
	if (tick < 0)
		tick = 0;
	if (tick != oldTick)
	{
		NumToString(tick,s);
		GetDialogItem(dp,1,&itemType,(Handle *)&itemHandle,&itemRect);		
		SetDialogItemText((Handle)itemHandle,s);
		oldTick = tick;
	}

	return(handled);
}

#endif

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

#if 0

/********************** GAMMA ON *********************/

void GammaOn(void)
{
#if ALLOW_FADE	

	if (gDisplayContext)
		if (!gGammaIsOn)
			DSpContext_FadeGamma(MONITORS_TO_FADE,100,nil);
#endif		
}


/***************** SETUP EVENT PROC ******************/

static Boolean SetupEventProc(EventRecord *event)
{

	return(false);
}


/****************** CLEANUP DISPLAY *************************/

void CleanupDisplay(void)
{
OSStatus 		theError;
	
	if(gDisplayContext != nil)
	{	
#if ALLOW_FADE		
		DSpContext_FadeGammaOut(gDisplayContext,nil);						// fade out	ours
#endif		
		DSpContext_SetState( gDisplayContext, kDSpContextState_Inactive );	// deactivate
#if ALLOW_FADE			
		DSpContext_FadeGamma(MONITORS_TO_FADE,100,nil);						// gamme on all
#endif		
		DSpContext_Release( gDisplayContext );								// release
	
		gDisplayContext = nil;
	}
	
	
	/* shutdown draw sprocket */
	
	if (gLoadedDrawSprocket)
	{
		theError = DSpShutdown();
		gLoadedDrawSprocket = false;
	}
}

#endif

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


#if 0
/************************ GAME SCREEN TO BLACK ************************/

void GameScreenToBlack(void)
{
	SetPort(gCoverWindow);
	BackColor(blackColor);
	EraseRect(&gCoverWindow->portRect);
}


/*********************** CLEAN SCREEN BORDER ****************************/
//
// This clears to black the border around the QD3D view and the cover window's rect.
//

void CleanScreenBorder(void)
{
Rect	r;

	if (gAdditionalClipping == 0)
		return;

	SetPort(gCoverWindow);
	BackColor(blackColor);
	
			/* TOP MARGIN */
			
	r = gCoverWindow->portRect;
	r.bottom = r.top + (gAdditionalClipping*.75);
	EraseRect(&r);

			/* BOTTOM MARGIN */
			
	r = gCoverWindow->portRect;
	r.top = r.bottom - (gAdditionalClipping*.75);
	EraseRect(&r);

			/* LEFT MARGIN */
			
	r = gCoverWindow->portRect;
	r.right = r.left + gAdditionalClipping;
	EraseRect(&r);
	
			/* RIGHT MARGIN */
			
	r = gCoverWindow->portRect;
	r.left = r.right - gAdditionalClipping;
	EraseRect(&r);
}

#pragma mark -

/********************** GET WINDOW DRAW INFO **********************/
//
// This is for 16 bit only!  Must modify for other depths!
//
// 	INPUT:		w 		:	the WindowPtr to get info on
//
//	OUTPUT:		pixelPtr	:	Ptr to the ptr to contain address of window's pixels
//				rowBytes	:	Ptr to variable to contain the rowbytes of the window.
//

void GetWindowDrawInfo(WindowPtr w, UInt16 **pixelPtr, UInt32 *rowBytes)
{
Rect			portRect;
PixMapHandle	portPixMap;
Ptr				addr;
long			x,y;

GDHandle        hDevice;
Rect            intersect;

#if 0
	if (w == nil)
		goto domain;

	if (((WindowPeek)w)->contRgn == nil)
		goto domain;
		

			/* MUST SCAN FOR GDEVICE WHICH ENCOMPASSES THE WINDOW */
			
	portRect = ((*(((WindowPeek)w)->contRgn))->rgnBBox);
	if (portRect.left < 0)
		portRect = w->portRect;

    for (hDevice = GetDeviceList(); hDevice; hDevice = GetNextDevice(hDevice))
    {
    	if (hDevice == nil)
    		break;
    		
        /*
         * test for active monitors that entirely contain rect
         */
        if (!TestDeviceAttribute(hDevice, screenDevice))
        	continue;
        	
        if (!TestDeviceAttribute(hDevice, screenActive))
        	continue;
        	
        if (!SectRect(&portRect, &((*hDevice)->gdRect), &intersect))
        	continue;
        	
        if (!EqualRect(&portRect, &intersect))
        	continue;
        
		goto got_it;
    }
    
domain:    
#endif
    hDevice = GetMainDevice();


got_it:
	portPixMap = (**hDevice).gdPMap;
	addr = (**portPixMap).baseAddr;					// get addr of the screen

	*rowBytes = (**portPixMap).rowBytes & 0x3fff;	// get & return rowbytes

	x = 0 ;//portRect.left;								// get coords of window in the pixmap
	y = 0; //portRect.top;
	
	if ((**portPixMap).pixelSize != 16)				// make sure 16bit
		DoFatalAlert("GetWindowDrawInfo: The monitor is not set to 16-bit display mode.");
	
//	addr += (y * (*rowBytes)) + (x*2);				// calc window addr for 16bit pixels
	
	*pixelPtr = (UInt16 *)addr;					// return ptr
}
#endif



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

#if 0
	DoLockPixels(destWorld);
	DoLockPixels(thisWorld);
#endif

	pm = GetGWorldPixMap(thisWorld);	
	if ((pm == nil) | (*pm == nil) )
		DoAlert("PixMap Handle or Ptr = Null?!");

	pm2 = GetGWorldPixMap(destWorld);	
	if ((pm2 == nil) | (*pm2 == nil) )
		DoAlert("PixMap Handle or Ptr = Null?!");

		
#if 0
	CopyBits((BitMap *)*pm,(BitMap *)*pm2,
			srcRect,destRect,srcCopy,0);
#else
	CopyBits(*pm,*pm2,srcRect,destRect,0,0);
#endif
	SetGWorld(oldGW,oldGD);								// restore gworld

}

#if 0
/*********************** DUMP GWORLD 2 **********************/
//
//    copies to a destination RECT
//

void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect)
{
PixMapHandle pm;
GrafPtr		oldPort;

	DoLockPixels(thisWorld);

	GetPort(&oldPort);
	pm = GetGWorldPixMap(thisWorld);	
	if ((pm == nil) | (*pm == nil) )
		DoAlert("PixMap Handle or Ptr = Null?!");

	SetPort(thisWindow);

	ForeColor(blackColor);
	BackColor(whiteColor);
		
	CopyBits((BitMap *)*pm, &(thisWindow->portBits), &(thisWorld->portRect), destRect, srcCopy, 0);
	SetPort(oldPort);
}


/******************* DO LOCK PIXELS **************/

void DoLockPixels(GWorldPtr world)
{
PixMapHandle pm;
	
	pm = GetGWorldPixMap(world);
	if (LockPixels(pm) == false)
		DoFatalAlert("PixMap Went Bye,Bye?!");
}


/***************** DO UNLOCK PIXELS **************/

void DoUnlockPixels(GWorldPtr world)
{
	PixMapHandle pm;
	
		pm = GetGWorldPixMap(world);
		UnlockPixels(pm);
}
#endif








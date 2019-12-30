/****************************/
/*     INPUT.C			    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include 	<DrawSprocket.h>
#include <InputSprocket.h>

#include "globals.h"
#include "misc.h"
#include "input.h"
#include "player_control.h"
#include "selfrundemo.h"
#include "windows.h"
#include "title.h"

extern	EventRecord	gTheEvent;
extern	unsigned long gOriginalSystemVolume;
extern	short		gMainAppRezFile;
extern	Byte		gDemoMode;
extern	Boolean		gAbortedFlag,gUsingDSP,gGameOverFlag,gAbortDemoFlag;
extern	DemoCacheKeyType	*gDemoCachePtr;
extern	long		gDemoCacheIndex;
extern	DSpContextReference 	gDisplayContext;
extern	WindowPtr	gCoverWindow;

/**********************/
/*     PROTOTYPES     */
/**********************/

static void MyGetKeys(KeyMap *keyMap);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	USE_ISP		1

/**********************/
/*     VARIABLES      */
/**********************/


KeyMap gKeyMap,gNewKeys,gOldKeys,gKeyMap_Real,gNewKeys_Real,gOldKeys_Real;

Boolean	gReadFromInputSprockets = false;
Boolean	gISpActive = false;							


		/* CONTORL NEEDS */
		
#define	NUM_CONTROL_NEEDS	22
static ISpNeed		gControlNeeds[NUM_CONTROL_NEEDS] =
{
	{													// 0
		"Jump",
		138,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Jump,
		0,
		0,
		0,
		0
	},
	
	{													// 1
		"Fire",
		131,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Fire,
		0,
		0,
		0,
		0
	},
	{													// 2
		"Select Weapon",
		136,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Select,
		0,
		0,
		0,
		0
	},


	{													// 3
		"Jet Up",
		148,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Fire,
		0,
		0,
		0,
		0
	},
	{													// 4
		"Jet Down",
		149,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Select,
		0,
		0,
		0,
		0
	},
	
	
	{													// 5
		"Walk Forward",
		132,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_MoveForward,
		0,
		0,
		0,
		0
	},
	{													// 6
		"Walk Backward",
		133,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_MoveBackward,
		0,
		0,
		0,
		0
	},
	{													// 7
		"Turn Left",
		134,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_TurnLeft,
		0,
		0,
		0,
		0
	},
	{													// 8
		"Turn Right",
		135,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_TurnRight,
		0,
		0,
		0,
		0
	},
	
	{													// 9
		"Pause",
		137,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_StartPause,
		0,
		0,
		0,
		0
	},
	
	{													// 10
		"Swivel Camera Left",
		141,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_LookLeft,
		0,
		0,
		0,
		0
	},
	{													// 11
		"Swivel Camera Right",
		142,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_LookRight,
		0,
		0,
		0,
		0
	},
	{													// 12
		"Zoom In",
		143,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Pad_Move,
		0,
		0,
		0,
		0
	},
	{													// 13
		"Zoom Out",
		144,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Pad_Move,
		1,
		0,
		0,
		0
	},
	{													// 14
		"Change Camera Mode",
		140,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		0,
		0,
		0,
		0
	},
	
	{													// 15
		"Pickup/Throw",
		147,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		0,
		0,
		0,
		0
	},

	{													// 16
		"Toggle Music",
		129,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		0,
		0,
		0,
		0
	},
	{													// 17
		"Toggle Ambient Sound",
		130,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		0,
		0,
		0,
		0
	},
	{													// 18
		"Raise Volume",
		145,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		0,
		0,
		0,
		0
	},
	{													// 19
		"Lower Volume",
		139,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		0,
		0,
		0,
		0
	},
	
	{													// 20
		"Toggle GPS",
		146,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		0,
		0,
		0,
		0
	},

	{													// 21
		"Quit Application",
		202,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Quit,
		0,
		0,
		0,
		0
	}
};


ISpElementReference	gVirtualElements[NUM_CONTROL_NEEDS];


short	gNeedToKey[NUM_CONTROL_NEEDS] =				// table to convert need # into key equate value
{
	kKey_Jump,
	kKey_Attack,
	kKey_AttackMode,

	kKey_JetUp,
	kKey_JetDown,

	kKey_Forward,
	kKey_Backward,
	kKey_TurnLeft,
	kKey_TurnRight,

	kKey_Pause,
	
	kKey_SwivelCameraLeft,
	kKey_SwivelCameraRight,
	kKey_ZoomIn,
	kKey_ZoomOut,
	kKey_CameraMode,
	
	kKey_PickUp,
	
	kKey_ToggleMusic,
	kKey_ToggleAmbient,
	kKey_RaiseVolume,
	kKey_LowerVolume,
	
	kKey_ToggleGPS
};


/************************* INIT INPUT *********************************/

void InitInput(void)
{
#if USE_ISP	
OSErr		iErr;
ISpDeviceReference	dev[10];
UInt32		count = 0;


				/* CREATE NEW NEEDS */
	
	iErr = ISpElement_NewVirtualFromNeeds(NUM_CONTROL_NEEDS, gControlNeeds, gVirtualElements, 0);
	if (iErr)
	{
		DoAlert("InitInput: ISpElement_NewVirtualFromNeeds failed!");
		ShowSystemErr(iErr);
	}
		
	iErr = ISpInit(NUM_CONTROL_NEEDS, gControlNeeds, gVirtualElements, 'NanO','Nan2', 0, 1000, 0);
	if (iErr)
	{
		DoAlert("InitInput: ISpInit failed!");
		ShowSystemErr(iErr);
	}
	 
			/* ACTIVATE ALL DEVICES */

	gISpActive = true;
	iErr = ISpDevices_Extract(10,&count,dev);
	if (iErr)
		DoFatalAlert("InitInput: ISpDevices_Extract failed!");
	iErr = ISpDevices_Activate(count, dev);
	if (iErr)
		DoFatalAlert("InitInput: ISpDevices_Activate failed!");


			/* DEACTIVATE JUST THE MOUSE SINCE WE DONT NEED THAT */
				
//	ISpDevices_ExtractByClass(kISpDeviceClass_Mouse,10,&count,dev);
//	ISpDevices_Deactivate(count, dev);


	TurnOffISp();

 #else
	gISpActive = false;	
	gReadFromInputSprockets = false;
#endif	
}


/**************** READ KEYBOARD *************/

void ReadKeyboard(void)
{
KeyMap tempKeys;

					/*****************/
					/* DEMO PLAYBACK */
					/*****************/

	if (gDemoMode == DEMO_MODE_PLAYBACK)								// see if read from demo file
	{
		if (gDemoCachePtr[gDemoCacheIndex].count <= 0)					// see if need to get next keymap
		{
			gDemoCacheIndex++;											// next recorded keymap
			if (gDemoCachePtr[gDemoCacheIndex].count == END_DEMO_MARK)	// see if end of file
			{
				StopDemo();
				return;
			}
			
			gKeyMap[0] = gDemoCachePtr[gDemoCacheIndex].keyMap[0];		// get new keys
			gKeyMap[1] = gDemoCachePtr[gDemoCacheIndex].keyMap[1];
			gKeyMap[2] = gDemoCachePtr[gDemoCacheIndex].keyMap[2];
			gKeyMap[3] = gDemoCachePtr[gDemoCacheIndex].keyMap[3];
		}
		
		gDemoCachePtr[gDemoCacheIndex].count--;							// decrement iteration counter
		
		MyGetKeys(&tempKeys);												// read real keyboard only to check for abort
		if (tempKeys[0] || tempKeys[1] || tempKeys[2] || tempKeys[3])	// any key aborts
			StopDemo();
	}
	else
		MyGetKeys(&gKeyMap);										// READ THE REAL KEYBOARD


			/* CALC WHICH KEYS ARE NEW THIS TIME */
			
	gNewKeys[0] = (gOldKeys[0] ^ gKeyMap[0]) & gKeyMap[0];
	gNewKeys[1] = (gOldKeys[1] ^ gKeyMap[1]) & gKeyMap[1];
	gNewKeys[2] = (gOldKeys[2] ^ gKeyMap[2]) & gKeyMap[2];
	gNewKeys[3] = (gOldKeys[3] ^ gKeyMap[3]) & gKeyMap[3];



					/***************/
					/* RECORD DEMO */
					/***************/
	
	if (gDemoMode == DEMO_MODE_RECORD)							// see if record keyboard
	{
		if ((gKeyMap[0] == gOldKeys[0]) &&						// see if same as last occurrence
			(gKeyMap[1] == gOldKeys[1]) &&
			(gKeyMap[2] == gOldKeys[2]) &&
			(gKeyMap[3] == gOldKeys[3]))
		{
			gDemoCachePtr[gDemoCacheIndex].count++;				// inc iteration count
		}
		else														// new KeyMap, so reset and add
		{	
			gDemoCacheIndex++;										// key cache key slot
			gDemoCachePtr[gDemoCacheIndex].count = 1;				// init iteration count to 1
			gDemoCachePtr[gDemoCacheIndex].keyMap[0] = gKeyMap[0];	// set keymap
			gDemoCachePtr[gDemoCacheIndex].keyMap[1] = gKeyMap[1];
			gDemoCachePtr[gDemoCacheIndex].keyMap[2] = gKeyMap[2];
			gDemoCachePtr[gDemoCacheIndex].keyMap[3] = gKeyMap[3];			
		}
		
		if ((sizeof(DemoCacheKeyType) * gDemoCacheIndex) >= MAX_DEMO_SIZE)	// see if overflowed
			DoFatalAlert("Demo Record Buffer Overflow!");

					/* SEE IF END OF DEMO */
					
		if (Nano_GetKeyState(KEY_ESC))
		{
			SaveDemoData();
			gGameOverFlag = gAbortedFlag = gAbortDemoFlag = true;
		}
	}
	
				/* SEE IF QUIT GAME */
				
	if (GetKeyState_Real(KEY_Q) && GetKeyState_Real(KEY_APPLE))			// see if key quit
		CleanQuit();	
		
	if (gISpActive)
		if (Nano_GetKeyState(kKey_Quit))										// see if ISP quit
			CleanQuit();	

		
			/* REMEMBER AS OLD MAP */

	gOldKeys[0] = gKeyMap[0];
	gOldKeys[1] = gKeyMap[1];
	gOldKeys[2] = gKeyMap[2];
	gOldKeys[3] = gKeyMap[3];
}


/**************** READ KEYBOARD_REAL *************/
//
// This just does a simple read of the REAL keyboard (regardless of Input Sprockets)
//

void ReadKeyboard_Real(void)
{


	GetKeys(gKeyMap_Real);										

			/* CALC WHICH KEYS ARE NEW THIS TIME */
			
	gNewKeys_Real[0] = (gOldKeys_Real[0] ^ gKeyMap_Real[0]) & gKeyMap_Real[0];
	gNewKeys_Real[1] = (gOldKeys_Real[1] ^ gKeyMap_Real[1]) & gKeyMap_Real[1];
	gNewKeys_Real[2] = (gOldKeys_Real[2] ^ gKeyMap_Real[2]) & gKeyMap_Real[2];
	gNewKeys_Real[3] = (gOldKeys_Real[3] ^ gKeyMap_Real[3]) & gKeyMap_Real[3];


			/* REMEMBER AS OLD MAP */

	gOldKeys_Real[0] = gKeyMap_Real[0];
	gOldKeys_Real[1] = gKeyMap_Real[1];
	gOldKeys_Real[2] = gKeyMap_Real[2];
	gOldKeys_Real[3] = gKeyMap_Real[3];
}


/****************** GET KEY STATE: REAL ***********/
//
// for data from ReadKeyboard_Real
//

Boolean GetKeyState_Real(unsigned short key)
{
unsigned char *keyMap;

	keyMap = (unsigned char *)&gKeyMap_Real;
	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}

/****************** GET NEW KEY STATE: REAL ***********/
//
// for data from ReadKeyboard_Real
//

Boolean GetNewKeyState_Real(unsigned short key)
{
unsigned char *keyMap;

	keyMap = (unsigned char *)&gNewKeys_Real;
	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}


/****************** GET KEY STATE ***********/
//
// NOTE: Assumes that ReadKeyboard has already been called!!
//

Boolean Nano_GetKeyState(unsigned short key)
{
unsigned char *keyMap;

	keyMap = (unsigned char *)&gKeyMap;
	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}


/****************** GET NEW KEY STATE ***********/
//
// NOTE: Assumes that ReadKeyboard has already been called!!
//

Boolean GetNewKeyState(unsigned short key)
{
unsigned char *keyMap;

	keyMap = (unsigned char *)&gNewKeys;
	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}

/********************** MY GET KEYS ******************************/
//
// Depending on mode, will either read key map from GetKeys or
// will "fake" a keymap using Input Sprockets.
//

static void MyGetKeys(KeyMap *keyMap)
{
short	i,key,j,q;
UInt32	keyState;
unsigned char *keyBytes;

	ReadKeyboard_Real();												// always read real keyboard anyway

	if (!gReadFromInputSprockets)
	{
		GetKeys(*keyMap);
	}
	else
	{
		keyBytes = (unsigned char *)keyMap;
		(*keyMap)[0] = (*keyMap)[1] = (*keyMap)[2] = (*keyMap)[3] = 0;		// clear out keymap
		
			/* POLL KEYS FROM INPUT SPROCKETS */
			
		for (i = 0; i < NUM_CONTROL_NEEDS; i++)	
		{
			ISpElement_GetSimpleState(gVirtualElements[i],&keyState);		// get state of this one
			if (keyState == kISpButtonDown)
			{
				key = gNeedToKey[i];										// get keymap value for this "need"
				j = key>>3;	
				q = (1<<(key&7));		
				keyBytes[j] |= q;											// set correct bit in keymap	
			}
		}
	}
}



/************************ DO KEY CONFIG DIALOG ***************************/

void DoKeyConfigDialog(void)
{
Boolean	o = gISpActive;

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);

				/* DO ISP CONFIG DIALOG */
			
	if (!gISpActive)
		TurnOnISp();
		
	ISpConfigure(nil);	
	
	
	if (!o)
		TurnOffISp() ;
}


/******************** TURN ON ISP *********************/

void TurnOnISp(void)
{
#if USE_ISP
ISpDeviceReference	dev[10];
UInt32		count = 0;
OSErr		iErr;

	if (!gISpActive)
	{
		gReadFromInputSprockets = true;								// player control uses input sprockets
		ISpResume();
		gISpActive = true;
		
				/* ACTIVATE ALL DEVICES */

		iErr = ISpDevices_Extract(10,&count,dev);
		if (iErr)
			DoFatalAlert("TurnOnISp: ISpDevices_Extract failed!");
		iErr = ISpDevices_Activate(count, dev);
		if (iErr)
			DoFatalAlert("TurnOnISp: ISpDevices_Activate failed!");
			
			/* DEACTIVATE JUST THE MOUSE SINCE WE DONT NEED THAT */
				
//		ISpDevices_ExtractByClass(kISpDeviceClass_Mouse,10,&count,dev);
//		ISpDevices_Deactivate(count, dev);
	}
#endif	
}

/******************** TURN OFF ISP *********************/

void TurnOffISp(void)
{
#if USE_ISP
ISpDeviceReference	dev[10];
UInt32		count = 0;

	if (gISpActive)
	{	
				/* DEACTIVATE ALL DEVICES */

		ISpDevices_Extract(10,&count,dev);
		ISpDevices_Deactivate(count, dev);
		ISpSuspend();		
	
		gISpActive = false;
		gReadFromInputSprockets = false;
	}
#endif	
}




















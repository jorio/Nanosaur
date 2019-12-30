/****************************/
/*     INPUT.C			    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/


#include "globals.h"
#include "misc.h"
#include "input.h"
#include "player_control.h"
#include "selfrundemo.h"
#include "windows_nano.h"
#include "title.h"

extern	unsigned long gOriginalSystemVolume;
extern	short		gMainAppRezFile;
extern	Byte		gDemoMode;
extern	Boolean		gAbortedFlag,gUsingDSP,gGameOverFlag,gAbortDemoFlag;
extern	DemoCacheKeyType	*gDemoCachePtr;
extern	long		gDemoCacheIndex;
extern	WindowPtr	gCoverWindow;

/**********************/
/*     PROTOTYPES     */
/**********************/

static void MyGetKeys(KeyMap *keyMap);



/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/


KeyMap gKeyMap,gNewKeys,gOldKeys,gKeyMap_Real,gNewKeys_Real,gOldKeys_Real;


		/* CONTORL NEEDS */
		
#define	NUM_CONTROL_NEEDS	22

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
	ReadKeyboard_Real();
	GetKeys(*keyMap);
}



/************************ DO KEY CONFIG DIALOG ***************************/

void DoKeyConfigDialog(void)
{
	// no-op without inputsprockets
}


/******************** TURN ON ISP *********************/

void TurnOnISp(void)
{
	// no-op without inputsprockets
}

/******************** TURN OFF ISP *********************/

void TurnOffISp(void)
{
	// no-op without inputsprockets
}




















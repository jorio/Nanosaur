/****************************/
/*   	HIGHSCORES.C    	*/
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"

#include <QD3D.h>
#include <QD3DGroup.h>
#include <QD3DMath.h>
#include <math.h>

#include "objects.h"
#include "mobjtypes.h"
#include "misc.h"
#include "highscores.h"
#include "qd3d_support.h"
#include "3dmf.h"
#include "sound2.h"
#include "title.h"
#include "input.h"
#include "windows_nano.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	float			gFramesPerSecond,gFramesPerSecondFrac;
extern	WindowPtr		gCoverWindow;
extern	short			gStreamCount;
extern	TQ3Object		gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID;
extern	FSSpec	gDataSpec;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupHighScoresScreen(void);
static void AddNewScore(unsigned long newScore);
static void EnterPlayerName(unsigned long newScore);
static void MoveCursor(ObjNode *theNode);
static void TypeNewKey(char c);
static void UpdateNameAndCursor(Boolean doCursor, float x, float y, float z);
static void SaveHighScores(void);
static void LoadHighScores(void);
static void ClearHighScores(void);
static void PrepHighScoresShow(void);

/***************************/
/*    CONSTANTS            */
/***************************/

#define	NUM_SCORES		8
#define	MAX_NAME_LENGTH	11

#define	LETTER_SEPARATION	18
#define	LEFT_EDGE			-100

typedef struct
{
	char			name[MAX_NAME_LENGTH];
	unsigned long	score;
}HighScoreType;



/***************************/
/*    VARIABLES            */
/***************************/

HighScoreType	gNewName;	
ObjNode	*gNewNameObj[MAX_NAME_LENGTH];

Byte	gCursorPosition;
ObjNode	*gCursorObj,*gSpiralObj;


HighScoreType	gHighScores[NUM_SCORES];	


/************** SHOW HIGHSCORES SCREEN *******************/
//
// INPUT: 	newScore = score to try to add, 0 == just show me
//

void ShowHighScoresScreen(unsigned long newScore)
{
TQ3Vector3D	camDelta = {0,0,0};

			/* INIT */
			
	SetupHighScoresScreen();
	
	MakeFadeEvent(true);
	
	
		/* ADD NEW SCORE */
			
	AddNewScore(newScore);
	

		/* PREP HIGH SCORES SHOW */
		
	PrepHighScoresShow();
		
		
			/* SHOW THE SCORES */

	do
	{
		QD3D_CalcFramesPerSecond();					
		MoveObjects();
		
				/* ROTATE SPIRAL */
				
		gSpiralObj->Rot.x += gFramesPerSecondFrac * 1.5;
		UpdateObjectTransforms(gSpiralObj);
		
				/* MOVE CAMERA */
				
		camDelta.x = gFramesPerSecondFrac * 70;
		QD3D_MoveCameraFromTo(gGameViewInfoPtr, &camDelta, &camDelta);			// update camera position
		
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);	

		ReadKeyboard();
		if (GetNewKeyState_Real(KEY_SPACE))
			break;
			
	}while(gGameViewInfoPtr->currentCameraCoords.x < 2200);
	
	
			/* CLEANUP */
				
	DeleteAllObjects();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
	Free3DMFGroup(MODEL_GROUP_HIGHSCORES);
	
}


/********************* SETUP HIGHSCORES SCREEN ******************************/

static void SetupHighScoresScreen(void)
{
TQ3Point3D			cameraFrom = { 0, 00, 160.0 };
TQ3Point3D			cameraTo = { 0.0, 0, 0.0 };
TQ3Vector3D			cameraUp = { 0.0, 1.0, 0.0 };
TQ3ColorARGB		clearColor = {1,0,0,0};
TQ3ColorRGB			ambientColor = { 1.0, 1.0, 1.0 };
TQ3Vector3D			fillDirection1 = { .7, -.1, -0.3 };
TQ3Vector3D			fillDirection2 = { -1, -.3, -.4 };
FSSpec				file;
QD3DSetupInputType		viewDef;
	
	DeleteAllObjects();
	
			/* SET QD3D PARAMETERS */
			
	QD3D_NewViewDef(&viewDef, gCoverWindow);			
	viewDef.view.clearColor 		= clearColor;
		
#if TWO_MEG_VERSION
	viewDef.view.paneClip.left 		+= 0;  
	viewDef.view.paneClip.right 	+= 0;  
	viewDef.view.paneClip.top		+= 80;  
	viewDef.view.paneClip.bottom 	+= 80;  
#endif	
		
	viewDef.camera.from 			= cameraFrom;
	viewDef.camera.to 				= cameraTo;
	viewDef.camera.up 				= cameraUp;
	viewDef.camera.hither 			= 5;
	viewDef.camera.yon 				= 2000;
	viewDef.camera.fov 				= 1.1;

	viewDef.lights.ambientBrightness = 0.2;
	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.numFillLights 	= 2;
	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[0] 	= ambientColor;
	viewDef.lights.fillColor[1] 	= ambientColor;
	viewDef.lights.fillBrightness[0] = 1.0;
	viewDef.lights.fillBrightness[1] = .4;

	viewDef.lights.useFog = true;

	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);


		/* LOAD MODELS */
		
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:HighScores.3dmf", &file);		
	LoadGrouped3DMF(&file, MODEL_GROUP_HIGHSCORES);

	
		/***********************/
		/* LOAD CURRENT SCORES */
		/***********************/

	LoadHighScores();	
}


/*********************** LOAD HIGH SCORES ********************************/

static void LoadHighScores(void)
{
OSErr				iErr;
short				refNum;
FSSpec				file;
long				count;

				/* OPEN FILE */
					
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":Nanosaur:HighScores", &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);	
	if (iErr == fnfErr)
		ClearHighScores();
	else
	if (iErr)
		DoFatalAlert("Error opening High Scores file!");
	else
	{
		count = sizeof(HighScoreType) * NUM_SCORES;
		iErr = FSRead(refNum, &count,  (Ptr)&gHighScores[0]);								// read data from file
		if (iErr)
		{
			FSClose(refNum);			
			FSpDelete(&file);												// file is corrupt, so delete
//			DoAlert("LoadHighScores: FSRead failed!");
//			ShowSystemErr(iErr);
			return;
		}
		FSClose(refNum);			
	}	
}


/************************ SAVE HIGH SCORES ******************************/

static void SaveHighScores(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

				/* CREATE BLANK FILE */
				
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":Nanosaur:HighScores", &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, 'NanO', 'Skor', smSystemScript);					// create blank file
	if (iErr)
		goto err;


				/* OPEN FILE */
					
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":Nanosaur:HighScores", &file);
	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
err:	
		DoAlert("Unable to Save High Scores file!");
		return;
	}

				/* WRITE DATA */
				
	count = sizeof(HighScoreType) * NUM_SCORES;
	FSWrite(refNum, &count, (Ptr)&gHighScores[0]);	
	FSClose(refNum);			

}


/**************** CLEAR HIGH SCORES **********************/

static void ClearHighScores(void)
{
short				i,j;
char				blank[] = "               ";

			/* INIT SCORES */
			
	for (i=0; i < NUM_SCORES; i++)
	{
		for (j=0; j < MAX_NAME_LENGTH; j++)
			gHighScores[i].name[j] = blank[j];
		gHighScores[i].score = 0;
	}

	SaveHighScores();		
}




/*************************** ADD NEW SCORE ****************************/

static void AddNewScore(unsigned long newScore)
{
short	slot,i;

			/* FIND INSERT SLOT */
	
	for (slot=0; slot < NUM_SCORES; slot++)
	{
		if (newScore > gHighScores[slot].score)
			goto	got_slot;
	}
	return;
	
	
got_slot:
			/* GET PLAYER'S NAME */
			
	EnterPlayerName(newScore);
	
			/* INSERT INTO LIST */

	for (i = NUM_SCORES-1; i > slot; i--)						// make hole
		gHighScores[i] = gHighScores[i-1];
	gHighScores[slot] = gNewName;								// insert new name 'n stuff
	
			/* UPDATE THE FILE */
			
	SaveHighScores();
}


/******************** ENTER PLAYER NAME **************************/

static void EnterPlayerName(unsigned long newScore)
{
ObjNode		*frameObj;
TQ3Point3D	camPt = gGameViewInfoPtr->currentCameraCoords;
float		camWobble = 0;
short		i;

				/*********/
				/* SETUP */
				/*********/
			
			/* MAKE NAME FRAME */
			
	gNewObjectDefinition.group = MODEL_GROUP_HIGHSCORES;
	gNewObjectDefinition.type = SCORES_ObjType_NameFrame;
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = 10;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1;
	frameObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* MAKE CURSOR */
			
	gNewObjectDefinition.type = SCORES_ObjType_Cursor;
	gNewObjectDefinition.coord.x = LEFT_EDGE;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.moveCall = MoveCursor;
	gCursorObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	MakeObjectKeepBackfaces(gCursorObj);
	
	for (i=0; i < MAX_NAME_LENGTH; i++)					// init name to blank
	{
		gNewName.name[i] = ' ';
		gNewNameObj[i] = nil;
	}
	gNewName.score = newScore;							// set new score
	gCursorPosition = 0;
			
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);		// use event manager to read keyboard, so start fresh
	
		
	
			/*************/
			/* MAIN LOOP */
			/*************/
		
	
	QD3D_CalcFramesPerSecond();					
		
	do
	{
		QD3D_CalcFramesPerSecond();					
		MoveObjects();
		
				/* CHECK FOR KEY */
				
		ReadKeyboard();
#if 1
		TODO2("key down event");
		break;
#else
		EventRecord	theEvent;
		if (GetNextEvent(keyDownMask, &theEvent))		// see if keydown
		{
			TypeNewKey(theEvent.message & charCodeMask);
			UpdateNameAndCursor(true,LEFT_EDGE,0,0);
		}
#endif
		
				/* MOVE CAMERA */
				
		camWobble += gFramesPerSecondFrac;
		camPt.x = sin(camWobble) * 50;
		QD3D_UpdateCameraFrom(gGameViewInfoPtr, &camPt);	// update camera position
		
				/* DRAW SCENE */
				
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);	
	}while(!GetNewKeyState_Real(KEY_RETURN));

			/* CLEANUP */

	DeleteAllObjects();
}


/****************** TYPE NEW KEY **********************/
//
// When player types new letter for name, this enters it.
//

static void TypeNewKey(char c)
{
short	i;
			/*******************/
			/* SEE IF EDIT KEY */
			/*******************/

	if (c == CHAR_RETURN)
		return;

				/* DELETE */
				
	if (c == CHAR_DELETE)
	{
		if (gCursorPosition > 0)
		{
			for (i = gCursorPosition-1; i < (MAX_NAME_LENGTH-1); i++)
				gNewName.name[i] = gNewName.name[i+1];
			gNewName.name[MAX_NAME_LENGTH-1] = ' ';
			gCursorPosition--;
		}
		return;
	}
				/* LEFT ARROW */
		
	if (c == CHAR_LEFT)
	{
		if (gCursorPosition > 0)
			gCursorPosition--;
		return;
	}
				/* RIGHT ARROW */
	
	if (c == CHAR_RIGHT)
	{
		if (gCursorPosition < (MAX_NAME_LENGTH-1))
			gCursorPosition++;
		return;
	}

			/* ADD NEW LETTER TO NAME */
			
	gNewName.name[gCursorPosition] = c;							// add to string
	if (gCursorPosition < (MAX_NAME_LENGTH-1))
		gCursorPosition++;
		
	PlayEffect(EFFECT_SELECT);				
}



/*************** MOVE CURSOR *************************/

static void MoveCursor(ObjNode *theNode)
{

			/* MAKE BLINK */
			
	theNode->SpecialF[0] -= gFramesPerSecondFrac;
	if (theNode->SpecialF[0] <= 0)
	{
		theNode->StatusBits ^= STATUS_BIT_HIDDEN;
		theNode->SpecialF[0] = 0.2;
	}
}


/******************** UPDATE NAME AND CURSOR ************************/
//
// INPUT: 	doCursor == false if just do name, not cursor.  Also, won't delete old name.
//			x,y,z = coord offsets for name
//

static void UpdateNameAndCursor(Boolean doCursor, float x, float y, float z)
{
short	i;
char	c;

	if (doCursor)
	{
				/* UPDATE CURSOR COORD */
				
		gCursorObj->Coord.x = LEFT_EDGE + (gCursorPosition*LETTER_SEPARATION);
		UpdateObjectTransforms(gCursorObj);


				/* DELETE OLD NAME */
				
		for (i=0; i < MAX_NAME_LENGTH; i++)								// init name to blank
		{
			if (gNewNameObj[i])
				Nano_DeleteObject(gNewNameObj[i]);
			gNewNameObj[i] = nil;
		}
	}

			/* MAKE NEW NAME */
			
	for (i=0; i < MAX_NAME_LENGTH; i++)								// init name to blank
	{
		c = gNewName.name[i];
		if (c != ' ')
		{
			if ((c >= '0') && (c <= '9'))							// see if char is a number
				gNewObjectDefinition.type = SCORES_ObjType_0 + (c - '0');
			else
			if ((c >= 'A') && (c <= 'Z'))							// see if char is a letter
				gNewObjectDefinition.type = SCORES_ObjType_A + (c - 'A');
			else
			if ((c >= 'a') && (c <= 'z'))							// see if char is a (lowercase) letter
				gNewObjectDefinition.type = SCORES_ObjType_A + (c - 'a');
			else
			switch(c)
			{
				case	'#':
						gNewObjectDefinition.type = SCORES_ObjType_Pound;
						break;
				case	'!':
						gNewObjectDefinition.type = SCORES_ObjType_Exclamation;
						break;
				case	'?':
						gNewObjectDefinition.type = SCORES_ObjType_Question;
						break;
				case	CHAR_APOSTROPHE:
						gNewObjectDefinition.type = SCORES_ObjType_Apostrophe;
						break;
				case	'.':
						gNewObjectDefinition.type = SCORES_ObjType_Period;
						break;
				case	':':
						gNewObjectDefinition.type = SCORES_ObjType_Colon;
						break;
				case	'-':
						gNewObjectDefinition.type = SCORES_ObjType_Dash;
						break;
				default:
						continue;
			}


			gNewObjectDefinition.group = MODEL_GROUP_HIGHSCORES;
			gNewObjectDefinition.coord.x = x + (i*LETTER_SEPARATION);
			gNewObjectDefinition.coord.y = y;
			gNewObjectDefinition.coord.z = z;
			gNewObjectDefinition.flags = 0;
			gNewObjectDefinition.slot = 10;
			gNewObjectDefinition.moveCall = nil;
			gNewObjectDefinition.rot = 0;
			gNewObjectDefinition.scale = 1;
			gNewNameObj[i] = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		}
	}
}


/******************************* PREP HIGH SCORES SHOW *******************************************/
//
// Creates all the objects for the "show highscores".
//

static void PrepHighScoresShow(void)
{
short	slot,place;
ObjNode	*newObj;
unsigned long score,digit;
TQ3Point3D	camPt = {-110,-30,90};
float		x;

			/* INIT CAMERA */
			
	QD3D_UpdateCameraFrom(gGameViewInfoPtr, &camPt);
	
	for (slot = 0; slot < NUM_SCORES; slot++)
	{
		x = LETTER_SEPARATION *  (MAX_NAME_LENGTH + 3) * slot;
		x += 200;
	
				/***************/
				/* CREATE NAME */
				/***************/

		gNewName = gHighScores[slot];
		UpdateNameAndCursor(false,x,0,0);
		
				/***************/
				/* CREATE SCORE*/
				/***************/
			
		score = gNewName.score;
		place = 0;
				
		x += 75;
		while((score > 0) || (place < 4))
		{
			digit = score % 10;											// get digit @ end of #
			score /= 10;												// shift down
			
			gNewObjectDefinition.type = SCORES_ObjType_0 + digit;
			gNewObjectDefinition.group = MODEL_GROUP_HIGHSCORES;
			gNewObjectDefinition.coord.x = x - (place*(LETTER_SEPARATION));
			gNewObjectDefinition.coord.y = -25;
			gNewObjectDefinition.coord.z = 0;
			gNewObjectDefinition.flags = 0;
			gNewObjectDefinition.slot = 10;
			gNewObjectDefinition.moveCall = nil;
			gNewObjectDefinition.rot = 0;
			gNewObjectDefinition.scale = 1.0;
			newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
			
			place++;
		}
	}
				/*****************/
				/* CREATE SPIRAL */
				/*****************/
				
	gNewObjectDefinition.type = SCORES_ObjType_Spiral;
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.flags = STATUS_BIT_HIGHFILTER;
	gNewObjectDefinition.slot = 10;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 4.0;
	gSpiralObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

}








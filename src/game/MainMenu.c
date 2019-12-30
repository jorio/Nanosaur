/****************************/
/*   	MAINMENU.C		    */
/* (c)1998 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

/****************************/
/*    EXTERNALS             */
/****************************/

#include <InputSprocket.h>
#include <QD3D.h>
#include <QD3DTransform.h>
#include <QD3DMath.h>
#include 	<DrawSprocket.h>
#include <math.h>
#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "mobjtypes.h"
#include "mainmenu.h"
#include "3dmf.h"
#include "input.h"
#include "windows_nano.h"
#include "SkeletonObj.h"
#include "sound2.h"
#include "title.h"
#include "file.h"
#include "environmentmap.h"
#include 	"selfrundemo.h"
#include "highscores.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	TQ3Point3D			gCoord;
extern	WindowPtr			gCoverWindow;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	KeyMap gNewKeys;
extern	Boolean				gSongPlayingFlag,gResetSong;
extern	ObjNode	*gFirstNodePtr;
extern	PrefsType	gGamePrefs;
extern	FSSpec		gDataSpec;

/****************************/
/*    PROTOTYPES             */
/****************************/

static void MakeMainMenuModels(void);
static void SpinToPreviousMainMenuIcon(void);
static void SpinToNextMainMenuIcon(void);
static void MoveFallingEgg(ObjNode *theNode);
static void GenerateFallingEgg(void);
static void DoQualityDialog(void);
static void MoveMenuBG(ObjNode *theNode);

/****************************/
/*    CONSTANTS             */
/****************************/

#define WHEEL_SEPARATION	310
#define SPIN_SPEED			2.5
#define	MENU_RING_Y			0

#define	NUM_MAINMENU_ICONS	5

/****************************/
/*    VARIABLES             */
/****************************/

static float gEggTimer = 0;



/******************* MENU INTERFACE ITEMS *************************/

enum
{
	MENU_ObjType_Quit,
	MENU_ObjType_Options,
	MENU_ObjType_Info,
	MENU_ObjType_HighScores,
	MENU_ObjType_Egg,
	MENU_ObjType_Background
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	XWarp	SpecialF[0]
#define	YWarp	SpecialF[1]
#define	ZWarp	SpecialF[2]

ObjNode	*gTitleLogoObj;
ObjNode	*gMainMenuIcons[NUM_MAINMENU_ICONS];

Byte	gCurrentSelection;

float	gTimeToDemo;

static	float		gMainMenuWheelRot;
static	float		gWheelCenterZ;


/************************* DO MAIN MENU ****************************/

void DoMainMenu(void)
{
FSSpec	file;
QD3DSetupInputType	viewDef;
TQ3Point3D			cameraFrom = { 0, 00, 600.0 };
float				timer;
Size				size;

	MaxMem(&size);


	MakeFadeEvent(true);

do_again:
			/**************/
			/* INITIALIZE */
			/**************/

			/* MAKE VIEW */

	QD3D_NewViewDef(&viewDef, gCoverWindow);
	viewDef.camera.hither 			= 50;
	viewDef.camera.yon 				= 1000;
	viewDef.camera.fov 				= 1.0;
	viewDef.camera.from 	= cameraFrom;

	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b 		= 0;
	
#if TWO_MEG_VERSION
	viewDef.view.paneClip.left 		+= 0;  
	viewDef.view.paneClip.right 	+= 0;  
	viewDef.view.paneClip.top		+= 80;  
	viewDef.view.paneClip.bottom 	+= 80;  
#else
	viewDef.view.paneClip.bottom 	+= 11;  
#endif	

	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);

	{
		int w;
		Str255	s = "Use the Arrow Keys to change the Selection.  Press the Spacebar to make a Selection.";
		
		SetPort(gCoverWindow);
		w = TextWidth(s, 0, s[0]);
		MoveTo(320-(w/2), 478);
		ForeColor(whiteColor);
		DrawString(s);
	}


			/* LOAD ART & SOUND */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:MenuInterface.3dmf", &file);
	LoadGrouped3DMF(&file, MODEL_GROUP_MENU);
	LoadASkeleton(SKELETON_TYPE_DEINON);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Menu.sounds", &file);
	LoadSoundBank(&file, SOUND_BANK_MENU);


			/* BUILD SCENE */
			
	MakeMainMenuModels();


			/* INIT DEMO TIMER */
			
	QD3D_CalcFramesPerSecond();
	timer = 0;

				/*************/
				/* MAIN LOOP */
				/*************/
				
	do
	{
				/* UPDATE FRAME */
				
		GenerateFallingEgg();
		MoveObjects();
		CalcEnvironmentMappingCoords(&gGameViewInfoPtr->currentCameraCoords);		
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);	
		QD3D_CalcFramesPerSecond();					
		
				/* CHECK FOR KEY INPUT */

		ReadKeyboard();
				
		if (GetNewKeyState_Real(KEY_SPACE) || GetNewKeyState_Real(KEY_RETURN))	// see if select
			break;

		if (GetNewKeyState_Real(KEY_LEFT))									// spin left
		{
			if (gCurrentSelection == 0)
				gCurrentSelection = NUM_MAINMENU_ICONS-1;
			else
				gCurrentSelection--;
			SpinToPreviousMainMenuIcon();
			timer = 0;
		}
		else	
		if (GetNewKeyState_Real(KEY_RIGHT))									// spin right
		{
			if (++gCurrentSelection >= NUM_MAINMENU_ICONS)
				gCurrentSelection = 0;
			SpinToNextMainMenuIcon();
			timer = 0;
		}		
	
				/* SEE IF DEMO TIME */
				
//		timer += gFramesPerSecondFrac;
//		if (timer > 10)
//		{
//			InitDemoPlayback();
//			break;
//		}
							
	}while(true);

			/* SEE IF RECORD DEMO */
			
	if (GetKeyState_Real(KEY_R))
		StartRecordingDemo();

	QD3D_CalcFramesPerSecond();		// call this to prime data for demo playback/record


		/***********/
		/* CLEANUP */
		/***********/
		
	DeleteAllObjects();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);		
	DeleteAll3DMFGroups();
	FreeAllSkeletonFiles(-1);
	DisposeSoundBank(SOUND_BANK_MENU);


			/* HANDLE SELECTION */
	
	switch(gCurrentSelection)
	{
		case	0:							// play
				GammaFadeOut();
				return;
				
		case	1:							// high scores	
				DoQualityDialog();
				goto do_again;
		
		case	2:							// HELP
				GammaFadeOut();
				ShowHelp();
				MakeFadeEvent(true);				
				goto do_again;
						
		case	3:							// quit
				CleanQuit();


		case	4:							// high scores
				GammaFadeOut();
				ShowHighScoresScreen(0);
				GammaFadeOut();
				MakeFadeEvent(true);				
				goto do_again;
	}	
}


/********************** MAKE MAIN MENU MODELS ****************************/

static void MakeMainMenuModels(void)
{
ObjNode	*newObj;
short	i;
float	r;

			/* MAKE SKELETON */
		
	gNewObjectDefinition.type 	= SKELETON_TYPE_DEINON;
	gNewObjectDefinition.animNum = 1;
	gNewObjectDefinition.scale = .8;
	gNewObjectDefinition.coord.x = -350;
	gNewObjectDefinition.coord.y = 00;
	gNewObjectDefinition.coord.z = 630;
	gNewObjectDefinition.slot = 10;
	gNewObjectDefinition.flags = STATUS_BIT_HIGHFILTER|STATUS_BIT_DONTCULL;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot = gMainMenuWheelRot + (PI2 / NUM_MAINMENU_ICONS * 0);
	gMainMenuIcons[0] = MakeNewSkeletonObject(&gNewObjectDefinition);	
	gMainMenuIcons[0]->Skeleton->AnimSpeed = .8;

			/* MAKE OPTIONS ICON */
				
	gNewObjectDefinition.group = MODEL_GROUP_MENU;
	gNewObjectDefinition.type = MENU_ObjType_Options;
	gNewObjectDefinition.coord.x += 225;
	gNewObjectDefinition.scale = 1.0;
	gNewObjectDefinition.rot = gMainMenuWheelRot + (PI2 / NUM_MAINMENU_ICONS * 1);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->XWarp = RandomFloat()*PI2;		
	newObj->YWarp = RandomFloat()*PI2;		
	newObj->ZWarp = RandomFloat()*PI2;		
	newObj->Flag[0] = 3;
	gMainMenuIcons[1] = newObj;

			/* MAKE INFO ICON */
				
	gNewObjectDefinition.type = MENU_ObjType_Info;
	gNewObjectDefinition.coord.x += 225;
	gNewObjectDefinition.rot = gMainMenuWheelRot + (PI2 / NUM_MAINMENU_ICONS * 2);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->XWarp = RandomFloat()*PI2;		
	newObj->YWarp = RandomFloat()*PI2;		
	newObj->ZWarp = RandomFloat()*PI2;		
	newObj->Flag[0] = 4;
	gMainMenuIcons[2] = newObj;

			/* MAKE QUIT ICON */
				
	gNewObjectDefinition.type = MENU_ObjType_Quit;
	gNewObjectDefinition.coord.x += 225;
	gNewObjectDefinition.rot = gMainMenuWheelRot + (PI2 / NUM_MAINMENU_ICONS * 3);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->XWarp = RandomFloat()*PI2;		
	newObj->YWarp = RandomFloat()*PI2;		
	newObj->ZWarp = RandomFloat()*PI2;		
	newObj->Flag[0] = 5;
	gMainMenuIcons[3] = newObj;


			/* MAKE HIGHSCORES ICON */
				
	gNewObjectDefinition.type = MENU_ObjType_HighScores;
	gNewObjectDefinition.coord.x += 225;
	gNewObjectDefinition.rot = gMainMenuWheelRot + (PI2 / NUM_MAINMENU_ICONS * 4);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->XWarp = RandomFloat()*PI2;		
	newObj->YWarp = RandomFloat()*PI2;		
	newObj->ZWarp = RandomFloat()*PI2;		
	newObj->Flag[0] = 6;
	gMainMenuIcons[4] = newObj;


			/* MAKE BACKGROUND */
				
	gNewObjectDefinition.type = MENU_ObjType_Background;
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 5;
	gNewObjectDefinition.slot = 50;
	gNewObjectDefinition.flags = STATUS_BIT_REFLECTIONMAP;
	gNewObjectDefinition.moveCall = MoveMenuBG;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->Scale.y *= .5;

			/* RECALC */

	for (i = 0; i < NUM_MAINMENU_ICONS; i++)
	{
		r = gMainMenuWheelRot + (PI2 / NUM_MAINMENU_ICONS * i);
	
		gMainMenuIcons[i]->Coord.x = sin(r) * WHEEL_SEPARATION;
		gMainMenuIcons[i]->Coord.z = gWheelCenterZ + (cos(r) * WHEEL_SEPARATION - 5);
		gMainMenuIcons[i]->Coord.y = MENU_RING_Y;
			if (i == 0)								// offset skeleton rot by 90 degrees
				r += PI/2;
		gMainMenuIcons[i]->Rot.y = r;
		UpdateObjectTransforms(gMainMenuIcons[i]);
	}
}


/****************** SPIN TO PREVIOUS MAIN MENU ICON ************************/
//
// Spins all the icons counter clockwise until they're in the right position.
//

static void SpinToPreviousMainMenuIcon(void)
{
float	targetRot,r;
Byte	i;

	PlayEffect(EFFECT_MENUCHANGE);										// play sound


	targetRot = gMainMenuWheelRot + (PI2/ NUM_MAINMENU_ICONS);			// calc target rotation

	do
	{
		gMainMenuWheelRot += SPIN_SPEED * gFramesPerSecondFrac;


		for (i = 0; i < NUM_MAINMENU_ICONS; i++)
		{
			r = gMainMenuWheelRot + (PI2 / NUM_MAINMENU_ICONS * i);
		
			gMainMenuIcons[i]->Coord.x = sin(r) * WHEEL_SEPARATION;
			gMainMenuIcons[i]->Coord.z = gWheelCenterZ + (cos(r) * WHEEL_SEPARATION - 5);
			gMainMenuIcons[i]->Coord.y = MENU_RING_Y;	
			
			if (i == 0)										// offset skeleton rot by 90 degrees
				r += PI/2;
				
			gMainMenuIcons[i]->Rot.y = r;
			UpdateObjectTransforms(gMainMenuIcons[i]);
		}
	
				/* UPDATE FRAME */
				
		QD3D_CalcFramesPerSecond();					
		MoveObjects();
		CalcEnvironmentMappingCoords(&gGameViewInfoPtr->currentCameraCoords);		
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);
		
		
		ReadKeyboard();
		
	}while(gMainMenuWheelRot < targetRot);

	gMainMenuWheelRot = targetRot;


}


/****************** SPIN TO NEXT MAIN MENU ICON ************************/
//
// Spins all the icons clockwise until they're in the right position.
//

static void SpinToNextMainMenuIcon(void)
{
float	targetRot,r;
Byte	i;

	PlayEffect(EFFECT_MENUCHANGE);										// play sound

	targetRot = gMainMenuWheelRot - (PI2/ NUM_MAINMENU_ICONS);				// calc target rotation

	do
	{
		gMainMenuWheelRot -= SPIN_SPEED * gFramesPerSecondFrac;


		for (i = 0; i < NUM_MAINMENU_ICONS; i++)
		{
			r = gMainMenuWheelRot + (PI2 / NUM_MAINMENU_ICONS * i);
		
			gMainMenuIcons[i]->Coord.x = sin(r) * WHEEL_SEPARATION;
			gMainMenuIcons[i]->Coord.z = gWheelCenterZ + (cos(r) * WHEEL_SEPARATION - 5);
			gMainMenuIcons[i]->Coord.y = MENU_RING_Y; // - cos(r)*100;	
			if (i == 0)										// offset skeleton rot by 90 degrees
				r += PI/2;
			gMainMenuIcons[i]->Rot.y = r;
			UpdateObjectTransforms(gMainMenuIcons[i]);
		}
	
				/* UPDATE FRAME */
				
		QD3D_CalcFramesPerSecond();					
		MoveObjects();
		CalcEnvironmentMappingCoords(&gGameViewInfoPtr->currentCameraCoords);		
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);		
		ReadKeyboard();
		
	}while(gMainMenuWheelRot > targetRot);

	gMainMenuWheelRot = targetRot;
}

/********************* GENERATE FALLING EGG ***********************/

static void GenerateFallingEgg(void)
{
ObjNode	*newObj;

	gEggTimer += gFramesPerSecondFrac;
	if (gEggTimer > .2)
	{
		gEggTimer = 0;

				/* MAKE HIGHSCORES ICON */
					
		gNewObjectDefinition.group = MODEL_GROUP_MENU;
		gNewObjectDefinition.type = MENU_ObjType_Egg;
		gNewObjectDefinition.coord.x = (RandomFloat()-.5) * 700;
		gNewObjectDefinition.coord.z = ((RandomFloat()-.5) * 700) + 150;
		gNewObjectDefinition.coord.y = 400;
		gNewObjectDefinition.scale = 1.0;
		gNewObjectDefinition.rot = RandomFloat() * PI;
		gNewObjectDefinition.slot = 50;
		gNewObjectDefinition.flags = 0;
		gNewObjectDefinition.moveCall = MoveFallingEgg;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		
		newObj->Rot.x = RandomFloat()*PI;		
		newObj->Rot.z = RandomFloat()*PI;		
	}
}


/********************* MOVE FALLING EGG **********************/

static void MoveFallingEgg(ObjNode *theNode)
{
	theNode->Rot.x += gFramesPerSecondFrac;
	theNode->Rot.y += gFramesPerSecondFrac;
	theNode->Rot.z += gFramesPerSecondFrac;

	theNode->Coord.y -= gFramesPerSecondFrac * 70;
	if (theNode->Coord.y < -250)
	{
		Nano_DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);
}


/********************* MOVE MENU BG **********************/

static void MoveMenuBG(ObjNode *theNode)
{
	theNode->Rot.y += gFramesPerSecondFrac;

	UpdateObjectTransforms(theNode);
}



/**************** DO QUALITY DIALOG *********************/

static void DoQualityDialog(void)
{
DialogPtr 		myDialog;
short			itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;
Boolean			dialogDone,cancelled;

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);

	SetPort(gCoverWindow);					// set this so DialogManager knows where to put dialog
	InitCursor();

	myDialog = GetNewDialog(129,nil,MOVE_TO_FRONT);

	DSpContext_FadeGamma(nil,100,nil);

	
				/* SET CONTROL VALUES */
					
	GetDialogItem(myDialog,2,&itemType,(Handle *)&itemHandle,&itemRect);
	SetControlValue((ControlHandle)itemHandle,gGamePrefs.highQualityTextures);

	GetDialogItem(myDialog,3,&itemType,(Handle *)&itemHandle,&itemRect);
	SetControlValue((ControlHandle)itemHandle, gGamePrefs.canDoFog);

	GetDialogItem(myDialog,4,&itemType,(Handle *)&itemHandle,&itemRect);
	SetControlValue((ControlHandle)itemHandle, gGamePrefs.shadows);

	GetDialogItem(myDialog,5,&itemType,(Handle *)&itemHandle,&itemRect);
	SetControlValue((ControlHandle)itemHandle, gGamePrefs.dust);
		
				/* DO IT */
				
	dialogDone = cancelled = false;
	while(!dialogDone)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case 	1:									// hit ok
					dialogDone = true;
					break;
					
			case	2:
					gGamePrefs.highQualityTextures = !gGamePrefs.highQualityTextures;
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.highQualityTextures);					
					break;
							
			case	3:
					gGamePrefs.canDoFog = !gGamePrefs.canDoFog;
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.canDoFog);					
					break;

			case	4:
					gGamePrefs.shadows = !gGamePrefs.shadows;
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.shadows);					
					break;

			case	5:
					gGamePrefs.dust = !gGamePrefs.dust;
					GetDialogItem(myDialog,itemHit,&itemType,(Handle *)&itemHandle,&itemRect);
					SetControlValue((ControlHandle)itemHandle,gGamePrefs.dust);					
					break;
					
			case	7:
					DoKeyConfigDialog();
					goto done;
		}
	}	
	
done:	
	DisposeDialog(myDialog);
	
	HideCursor();	
	SavePrefs(&gGamePrefs);	
}













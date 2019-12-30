/****************************/
/*   		TITLE.C		    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
#include <QD3DTransform.h>
#include <QD3DMath.h>
#include <math.h>

#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "title.h"
#include "3dmf.h"
#include "mobjtypes.h"
#include "windows_nano.h"
#include "input.h"
#include "file.h"
#include "SkeletonObj.h"
#include "environmentmap.h"
#include "sound2.h"
#include 	"selfrundemo.h"
#include "skeletonanim.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	TQ3Point3D			gCoord;
extern	WindowPtr			gCoverWindow;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	KeyMap gNewKeys_Real;
extern	Boolean		gSongPlayingFlag,gResetSong,gDisableAnimSounds,gSongPlayingFlag;
extern	FSSpec		gDataSpec;
extern	PrefsType	gGamePrefs;

/****************************/
/*    PROTOTYPES            */
/****************************/


static void MoveTitleText(ObjNode *theNode);
static void MovePangeaLogoPart(ObjNode *theNode);
static void DoPangeaLogo(void);
static void MoveTitleBG(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/


/********************** DO TITLE SCREEN *************************/

void DoTitleScreen(void)
{
QD3DSetupInputType		viewDef;
ObjNode			*dinoObj;
TQ3Point3D		cameraFrom = { 110, 90, 190.0 };
TQ3Point3D		cameraFrom2 = { 110, 90, 190.0 };

			/************************/
			/* DO PANGEA LOGO INTRO */
			/************************/
			//
			// note: this also loads the required art for the title sequence
			//
			
	DoPangeaLogo();



			/* LOAD ADDITIONAL ART */
			
	LoadASkeleton(SKELETON_TYPE_REX);



			/* MAKE VIEW */

	QD3D_NewViewDef(&viewDef, gCoverWindow);
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 500;
	viewDef.camera.fov 				= 1.0;
	viewDef.lights.fogHither		= .3;
	viewDef.styles.usePhong = false;
#if TWO_MEG_VERSION
	viewDef.view.paneClip.left 		+= 64;  
	viewDef.view.paneClip.right 	+= 64;  
	viewDef.view.paneClip.top		+= 50;  
	viewDef.view.paneClip.bottom 	+= 50;  
	viewDef.camera.from 			= cameraFrom2;
#else
	viewDef.camera.from 			= cameraFrom;
#endif
	
	if (!gGamePrefs.canDoFog)		// if no fog possible, then bg is black
	{
		viewDef.view.clearColor.r = 0;
		viewDef.view.clearColor.g = 0;
		viewDef.view.clearColor.b = 0;
	}
	else							// set fog since I think it'll work
	{
		viewDef.view.clearColor.r = 
		viewDef.view.clearColor.g = 
		viewDef.view.clearColor.b = 1;
	}
	
	
	
	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);

	if (!gSongPlayingFlag)						// make sure music is going
		PlaySong(1,true);


			/* MAKE SKELETON 1 */
		
	gNewObjectDefinition.type 	= SKELETON_TYPE_REX;
	gNewObjectDefinition.animNum = 1;
	gNewObjectDefinition.scale = .5;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.x = 10;
	gNewObjectDefinition.coord.z = 70;
	gNewObjectDefinition.slot = PLAYER_SLOT;
	gNewObjectDefinition.flags = STATUS_BIT_HIGHFILTER;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot = -PI/2;
	dinoObj = MakeNewSkeletonObject(&gNewObjectDefinition);	
	dinoObj->Skeleton->AnimSpeed = .8;
	UpdateObjectTransforms(dinoObj);



			/* MAKE TITLE NAME */
			
	gNewObjectDefinition.group = MODEL_GROUP_TITLE;	
	gNewObjectDefinition.type = TITLE_MObjType_GameName;	
	gNewObjectDefinition.coord.x = 60;
	gNewObjectDefinition.coord.y = 15;
	gNewObjectDefinition.coord.z = 100;
	gNewObjectDefinition.flags = STATUS_BIT_REFLECTIONMAP;
	gNewObjectDefinition.moveCall = MoveTitleText;
	gNewObjectDefinition.rot = .9;
	gNewObjectDefinition.scale = .4;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* MAKE BACKGROUND */
			
	gNewObjectDefinition.group = MODEL_GROUP_TITLE;	
	gNewObjectDefinition.type = TITLE_MObjType_Background;	
	gNewObjectDefinition.coord.x = -400;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = -40;
	gNewObjectDefinition.flags = STATUS_BIT_DONTCULL;
	gNewObjectDefinition.moveCall = MoveTitleBG;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 2.5;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);
	
	gNewObjectDefinition.coord.x += 300.0f * gNewObjectDefinition.scale;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);




			/*****************/
			/* ANIMATE TITLE */
			/*****************/
			
	gDisableAnimSounds = true;
			
	MakeFadeEvent(true);								// start fade-in		
			
	QD3D_CalcFramesPerSecond();
	
	while(!Button())
	{
		MoveObjects();
		CalcEnvironmentMappingCoords(&gGameViewInfoPtr->currentCameraCoords);
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);
		QD3D_CalcFramesPerSecond();
		ReadKeyboard();									// keys get us out
		if (gNewKeys_Real[0] || gNewKeys_Real[1] ||  gNewKeys_Real[2] || gNewKeys_Real[3])
			break;
	}
	
			/* CLEANUP */

	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DeleteAll3DMFGroups();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);		
	GammaFadeOut();
	gDisableAnimSounds = false;
}



/**************** MOVE TITLE BG ********************/

static void MoveTitleBG(ObjNode *theNode)
{
	GetObjectInfo(theNode);

	gCoord.x -= gFramesPerSecondFrac * 65.0f;
	if (gCoord.x < (-480.0f * theNode->Scale.x))
	{
		gCoord.x += 300.0f * theNode->Scale.x * 2.0f;
	}
	
	UpdateObject(theNode);

}


/******************* MOVE TITLE TEXT *************************/

static void MoveTitleText(ObjNode *theNode)
{
	theNode->SpecialF[0] += gFramesPerSecondFrac*1.8;
	
	theNode->Rot.y = .3 + (sin(theNode->SpecialF[0])+1.0) * .3;
	theNode->Rot.x = -.3;

	UpdateObjectTransforms(theNode);
}

/********************** DO PANGEA LOGO **********************************/

static void DoPangeaLogo(void)
{
ObjNode		*backObj;
TQ3Point3D			cameraFrom = { 0, 0, 70.0 };
QD3DSetupInputType		viewDef;
TQ3ColorRGB		c1 = { 1.0, 1, 1 };
TQ3ColorRGB		c2 = { 1, .9, .6 };
FSSpec			spec;

			/* MAKE VIEW */

	QD3D_NewViewDef(&viewDef, gCoverWindow);
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 350;
	viewDef.camera.fov 				= 1.0;
	viewDef.camera.from				= cameraFrom;
	viewDef.lights.fillColor[0] 	= c1;
	viewDef.lights.fillColor[1] 	= c2;
	viewDef.view.clearColor.r = 
	viewDef.view.clearColor.g = 
	viewDef.view.clearColor.b = 0;
	viewDef.styles.usePhong = false;
	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);


			/* LOAD ART */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:Title.3dmf", &spec);		// load other models
	LoadGrouped3DMF(&spec,MODEL_GROUP_TITLE);	




			/***************/
			/* MAKE MODELS */
			/***************/
				
			/* BACKGROUND */
			
	gNewObjectDefinition.group = MODEL_GROUP_TITLE;	
	gNewObjectDefinition.type = TITLE_MObjType_Pangea;
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = -300;	
	gNewObjectDefinition.flags = STATUS_BIT_REFLECTIONMAP;
	gNewObjectDefinition.slot = 50;
	gNewObjectDefinition.moveCall = MovePangeaLogoPart;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = .2;
	backObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	backObj->Rot.y = -PI/2;
	backObj->Rot.x = -PI/2;
	
	
	MakeFadeEvent(true);
	
	
			/*************/
			/* MAIN LOOP */
			/*************/
			
	PlaySong(2,false);			

	while((!gResetSong) && gSongPlayingFlag)					// wait until song stops
	{
		ReadKeyboard();
		if (gNewKeys_Real[0] || gNewKeys_Real[1] ||  gNewKeys_Real[2] || gNewKeys_Real[3])
			break;

		MoveObjects();
		CalcEnvironmentMappingCoords(&gGameViewInfoPtr->currentCameraCoords);
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);	
		QD3D_CalcFramesPerSecond();					
	}
	GammaFadeOut();


			/***********/
			/* CLEANUP */
			/***********/
			
abort:	
	DeleteAllObjects();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);		
	KillSong();
}


/********************* MOVE PANGEA LOGO PART ***************************/

static void MovePangeaLogoPart(ObjNode *theNode)
{
	theNode->Coord.z += gFramesPerSecondFrac * 45;		
	theNode->Rot.y += gFramesPerSecondFrac * PI/9;		

	theNode->Rot.x = sin(theNode->SpecialF[0] += gFramesPerSecondFrac*1.5) * .3;
						
	UpdateObjectTransforms(theNode);
}


/*************** SHOW CHARITY **********************/
//
// OEM non-charity version
//

void ShowCharity(void)
{
//PicHandle	pic;
//Rect		r;
FSSpec	spec;

			/* DO PAGE 1 */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:Boot1.pict", &spec);
    DrawPictureToScreen(&spec, 0,0);

//	pic = LoadAPict(&spec);
//	SetPort(gCoverWindow);
//	SetRect(&r,0,0,GAME_VIEW_WIDTH,GAME_VIEW_HEIGHT);
//	DrawPicture(pic, &r);	
//	DisposeHandle((Handle)pic);
	ReadKeyboard();
		
	do
	{
		ReadKeyboard();
		DoSoundMaintenance();
	}while(!(gNewKeys_Real[0] || gNewKeys_Real[1] ||  gNewKeys_Real[2] || gNewKeys_Real[3]));

			/* DO PAGE 2 */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:Boot2.pict", &spec);		// load next page
    DrawPictureToScreen(&spec, 0,0);
//	pic = LoadAPict(&spec);
//	DrawPicture(pic, &r);	
//	DisposeHandle((Handle)pic);
	ReadKeyboard();
		
	do
	{
		ReadKeyboard();
		DoSoundMaintenance();
	}while(!(gNewKeys_Real[0] || gNewKeys_Real[1] ||  gNewKeys_Real[2] || gNewKeys_Real[3]));

	GammaFadeOut();
}


/*************** SHOW HELP **********************/

void ShowHelp(void)
{
//PicHandle	pic;
//Rect		r;
FSSpec	spec;

			/* DO PAGE 1 */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:Help1.pict", &spec);
    DrawPictureToScreen(&spec, 0,0);
	
//	pic = LoadAPict(&spec);
//	SetPort(gCoverWindow);
//	SetRect(&r,0,0,GAME_VIEW_WIDTH,GAME_VIEW_HEIGHT);
//	DrawPicture(pic, &r);	
	GammaFadeIn();
//	DisposeHandle((Handle)pic);
	ReadKeyboard();
	
	
	
	do
	{
		ReadKeyboard();
		DoSoundMaintenance();
	}while(!(gNewKeys_Real[0] || gNewKeys_Real[1] ||  gNewKeys_Real[2] || gNewKeys_Real[3]));

			/* DO PAGE 2 */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:Help2.pict", &spec);		// load next page
    DrawPictureToScreen(&spec, 0,0);
//	pic = LoadAPict(&spec);
//	DrawPicture(pic, &r);	
//	DisposeHandle((Handle)pic);
	ReadKeyboard();
	
	do
	{
		ReadKeyboard();
		DoSoundMaintenance();
	}while(!(gNewKeys_Real[0] || gNewKeys_Real[1] ||  gNewKeys_Real[2] || gNewKeys_Real[3]));

	GammaFadeOut();
}





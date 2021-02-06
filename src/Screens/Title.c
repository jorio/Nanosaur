/****************************/
/*   		TITLE.C		    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
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
#include "skeletonobj.h"
#include "environmentmap.h"
#include "sound2.h"
#include "skeletonanim.h"

#include "GamePatches.h"
#include "version.h"
#include <string.h>
#include <stdlib.h>

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	TQ3Point3D			gCoord;
extern	WindowPtr			gCoverWindow;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	KeyMap gNewKeys_Real;
extern	Boolean		gSongPlayingFlag,gResetSong,gDisableAnimSounds;
extern	FSSpec		gDataSpec;
extern	PrefsType	gGamePrefs;
extern	const int	PRO_MODE;

/****************************/
/*    PROTOTYPES            */
/****************************/


static void MoveTitleText(ObjNode *theNode);
static void MovePangeaLogoPart(ObjNode *theNode);
static void MoveTitleBG(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

// Source port addition.
// These values have been tweaked so that the intro screen looks good
// in widescreen without any gaps appearing in the background.
static const int	kBackgroundRepeat			= 3;		// was 2
static const float	kBackgroundScale			= 2.6f;		// was 2.5
static const float	kBackgroundLeftmostX		= -600.0f;	// was -480
static const float	kBackgroundLength			= 300.0f;


/*********************/
/*    VARIABLES      */
/*********************/


/********************** DO TITLE SCREEN *************************/

void DoTitleScreen(void)
{
FSSpec spec;
QD3DSetupInputType		viewDef;
ObjNode			*dinoObj;
TQ3Point3D		cameraFrom = { 110, 90, 190.0 };



			/* MAKE VIEW */

	QD3D_NewViewDef(&viewDef);
	viewDef.camera.hither 			= 10;
	viewDef.camera.yon				= gGamePrefs.canDoFog? 500: 700;		// Source port change from 500 to look good in widescreen without fog
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



			/* LOAD ART */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Title.3dmf", &spec);		// load other models
	LoadGrouped3DMF(&spec,MODEL_GROUP_TITLE);

	LoadASkeleton(SKELETON_TYPE_REX);




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
	gNewObjectDefinition.coord.x = kBackgroundLeftmostX * kBackgroundScale;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = -40;
	gNewObjectDefinition.flags = STATUS_BIT_DONTCULL;
	gNewObjectDefinition.moveCall = MoveTitleBG;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = kBackgroundScale;

	for (int i = 0; i < kBackgroundRepeat; i++)
	{
		MakeNewDisplayGroupObject(&gNewObjectDefinition);
		gNewObjectDefinition.coord.x += kBackgroundLength * kBackgroundScale;
	}



			/*****************/
			/* ANIMATE TITLE */
			/*****************/
			
	gDisableAnimSounds = true;
			
	MakeFadeEvent(true);								// start fade-in		
			
	QD3D_CalcFramesPerSecond();
	
	while(!Button())
	{
		MoveObjects();
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
	if (gCoord.x < (kBackgroundLeftmostX * kBackgroundScale))
	{
		gCoord.x += kBackgroundLength * kBackgroundScale * kBackgroundRepeat;
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

void DoPangeaLogo(void)
{
ObjNode		*backObj;
TQ3Point3D			cameraFrom = { 0, 0, 70.0 };
QD3DSetupInputType		viewDef;
TQ3ColorRGB		c1 = { 1.0, 1, 1 };
TQ3ColorRGB		c2 = { 1, .9, .6 };
FSSpec			spec;

			/* MAKE VIEW */

	QD3D_NewViewDef(&viewDef);
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
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Title.3dmf", &spec);		// load other models
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
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);
		QD3D_CalcFramesPerSecond();					
	}
	GammaFadeOut();


			/***********/
			/* CLEANUP */
			/***********/
			
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


/*************** SLIDESHOW (source port refactor) **********************/

enum SlideshowEntryOpcode
{
	SLIDESHOW_STOP,
	SLIDESHOW_FILE,
	SLIDESHOW_RESOURCE
};

struct SlideshowEntry
{
	int opcode;
	const char* imagePath;
	short pictResource;
	void (*postDrawCallback)(void);
};

static void Slideshow(const struct SlideshowEntry* slides)
{
	FSSpec spec;

	ExclusiveOpenGLMode_Begin();

	for (int i = 0; slides[i].opcode != SLIDESHOW_STOP; i++)
	{
		const struct SlideshowEntry* slide = &slides[i];

		if (slide->opcode == SLIDESHOW_RESOURCE)
		{
			PicHandle picHandle = GetPicture(slide->pictResource);
			if (!picHandle)
				continue;
			DrawPicture(picHandle, &(**picHandle).picFrame);
			ReleaseResource((Handle)picHandle);
		}
		else if (slide->opcode == SLIDESHOW_FILE)
		{
			OSErr result;
			result = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, slide->imagePath, &spec);
			if (result != noErr)
				continue;
			DrawPictureToScreen(&spec, 0, 0);
		}
		else
		{
			DoAlert("unsupported slideshow opcode");
			continue;
		}

		if (slide->postDrawCallback)
		{
			slide->postDrawCallback();
		}

		if (i == 0)
		{
			RenderBackdropQuad(BACKDROP_FIT);
			GammaFadeIn();
		}
		ReadKeyboard();

		float slideAge = 0;
		bool promptShownYet = false;

		do
		{
			slideAge += gFramesPerSecondFrac;

			if (!promptShownYet && slideAge > 2)
			{
				MoveTo(490, 480);
				RGBBackColor2(0);
				RGBForeColor2(0xFFFFFF);
				DrawStringC(" Hit SPACE to continue ");
				promptShownYet = true;
			}

			ReadKeyboard();
			DoSoundMaintenance();
			RenderBackdropQuad(BACKDROP_FIT);
			QD3D_CalcFramesPerSecond(); // required for DoSDLMaintenance to properly cap the framerate
			DoSDLMaintenance();
		} while (!GetNewKeyState_Real(kVK_Return) && !GetNewKeyState_Real(kVK_Escape) && !GetNewKeyState_Real(kVK_Space));
	}

	ExclusiveOpenGLMode_End();

	GammaFadeOut();
}


/*************** SHOW CHARITY **********************/
//
// OEM non-charity version
//

static void ShowCharity_SourcePortVersionOverlay(void)
{
    RGBBackColor2(0xA5A5A5);
    RGBForeColor2(0x000000);
    MoveTo(8, 16);
    DrawStringC(PROJECT_VERSION);
}

static void ShowCharity_SourcePortCreditOverlay(void)
{
	RGBBackColor2(0xA5A5A5);
	MoveTo(8, 480-8-14-14);
	RGBForeColor2(0xA50808);
	DrawStringC("SOURCE PORT:");
	ForeColor(blackColor);
	MoveTo(8, 480-8-14);
	DrawStringC("Iliyas Jorio (2020)");
	MoveTo(8, 480-8);
	DrawStringC("https://github.com/jorio/nanosaur");
}

void ShowCharity(void)
#if 1	// TODO noquesa
{ printf("TODO noquesa: %s\n", __func__); }
#else
{
	const char* firstImage = PRO_MODE ? ":images:Boot1Pro.pict" : ":images:Boot1.pict";

	const struct SlideshowEntry slides[] = {
			{ SLIDESHOW_FILE, firstImage, 0, ShowCharity_SourcePortVersionOverlay },
			{ SLIDESHOW_FILE, ":images:Boot2.pict", 0, ShowCharity_SourcePortCreditOverlay },
			{ SLIDESHOW_STOP, NULL, 0, NULL },
	};
	Slideshow(slides);
}
#endif


/*************** SHOW HELP **********************/

void ShowHelp(void)
{
	const struct SlideshowEntry slides[] = {
			{ SLIDESHOW_FILE, ":images:Help1.pict", 0, NULL },
			{ SLIDESHOW_FILE, ":images:Help2.pict", 0, NULL },
			{ SLIDESHOW_STOP, NULL, 0, NULL },
	};
	Slideshow(slides);
}



/******************* SHOW BUGDOM AD *************************/

void ShowBugdomAd(void)
{
	const struct SlideshowEntry slides[] = {
			{ SLIDESHOW_RESOURCE, NULL, 130, NULL },
			{ SLIDESHOW_STOP, NULL, 0, NULL },
	};
	Slideshow(slides);
}

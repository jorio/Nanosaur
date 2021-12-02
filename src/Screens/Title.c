/****************************/
/*   		TITLE.C		    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


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


	// Subdivide triangles in background mesh so per-vertex fog looks better on it
	// on systems that don't support per-pixel fog.
	Q3TriMeshData_SubdivideTriangles(gObjectGroupList[MODEL_GROUP_TITLE][TITLE_MObjType_Background].meshes[0]);
	Q3TriMeshData_SubdivideTriangles(gObjectGroupList[MODEL_GROUP_TITLE][TITLE_MObjType_Background].meshes[0]);


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
	
	while (1)
	{
		MoveObjects();
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);
		QD3D_CalcFramesPerSecond();
		UpdateInput();									// keys get us out
		if (UserWantsOut())
			break;
	}
	
			/* CLEANUP */

	Render_FreezeFrameFadeOut();

	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DeleteAll3DMFGroups();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);		
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
		UpdateInput();
		if (UserWantsOut())
			break;

		MoveObjects();
		QD3D_DrawScene(gGameViewInfoPtr,DrawObjects);
		QD3D_CalcFramesPerSecond();					
	}

	Render_FreezeFrameFadeOut();


			/***********/
			/* CLEANUP */
			/***********/
			
	DeleteAllObjects();
	DeleteAll3DMFGroups();
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

struct SlideshowEntry
{
	const char* imagePath;
	void (*postDrawCallback)(void);
};

static void Slideshow(const struct SlideshowEntry* slides)
{
	FSSpec spec;

	SDL_GLContext glContext = SDL_GL_CreateContext(gSDLWindow);
	GAME_ASSERT(glContext);
	
	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);
	
	Render_InitState();
	Render_Alloc2DCover(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT);

	for (int i = 0; slides[i].imagePath != NULL; i++)
	{
		const struct SlideshowEntry* slide = &slides[i];
		
		OSErr result = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, slide->imagePath, &spec);
		GAME_ASSERT(result == noErr);

		PicHandle picHandle = GetPictureFromTGA(&spec);
		GAME_ASSERT(picHandle);
		GAME_ASSERT(*picHandle);

		DrawPicture(picHandle, &(**picHandle).picFrame);
		DisposeHandle((Handle)picHandle);

		if (slide->postDrawCallback)
		{
			slide->postDrawCallback();
		}

		uint8_t* clearColor = (uint8_t*)gCoverWindowPixPtr;
		glClearColor(clearColor[1]/255.0f, clearColor[2]/255.0f, clearColor[3]/255.0f, 1.0f);

		UpdateInput();

		float slideAge = 0;
		bool promptShownYet = false;

		bool wantOut = false;

		do
		{
			float gamma = 100;
#if ALLOW_FADE
			if (i == 0)
			{
				gamma = 100.0f * slideAge / 1.0f;
				if (gamma > 100)
					gamma = 100;
			}
			Render_SetWindowGamma(gamma);
#endif

			slideAge += gFramesPerSecondFrac;

			if (!promptShownYet && slideAge > 2)
			{
				MoveTo(490, 480-4);
				RGBBackColor2(0);
				RGBForeColor2(0xFFFFFF);
				DrawStringC(" Hit SPACE to continue ");
				promptShownYet = true;
			}

			UpdateInput();
			DoSoundMaintenance();

			Render_StartFrame();
			Render_Draw2DCover(kCoverQuadFit);

			QD3D_CalcFramesPerSecond(); // required to properly cap the framerate

			Render_EndFrame();
			SDL_GL_SwapWindow(gSDLWindow);

			if (gamma < 100)
				wantOut = false;
			else
				wantOut = UserWantsOut();
		} while (!wantOut);
	}

	Render_FreezeFrameFadeOut();

	Render_Dispose2DCover();
	SDL_GL_DeleteContext(glContext);
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
	MoveTo(8, 480-4-14);
	RGBForeColor2(0xA50808);
	DrawStringC("ENHANCED UPDATE:   ");
	ForeColor(blackColor);
	DrawStringC("Iliyas Jorio");
	MoveTo(8, 480-4);
	RGBForeColor2(0x606060);
	DrawStringC("https://github.com/jorio/nanosaur");
}

void ShowCharity(void)
{
	const char* firstImage = PRO_MODE ? ":images:Boot1Pro.tga" : ":images:Boot1.tga";

	const struct SlideshowEntry slides[] = {
			{ firstImage, ShowCharity_SourcePortVersionOverlay },
			{ ":images:Boot2.tga", ShowCharity_SourcePortCreditOverlay },
			{ NULL, NULL },
	};
	Slideshow(slides);
}


/*************** SHOW HELP **********************/

void ShowHelp(void)
{
	const struct SlideshowEntry slides[] = {
			{ ":images:Help1.tga", NULL },
			{ ":images:Help2.tga", NULL },
			{ NULL, NULL },
	};
	Slideshow(slides);
}


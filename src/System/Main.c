/****************************/
/*    NANOSAUR - MAIN 	*/
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

static void InitLevel(void);
static void CleanupLevel(void);
static void PlayLevel(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	KILL_DELAY	4


/****************************/
/*    VARIABLES             */
/****************************/


Boolean		gGameOverFlag;
Boolean		gPlayerGotKilledFlag,gWonGameFlag;

QD3DSetupOutputType		*gGameViewInfoPtr = nil;

PrefsType	gGamePrefs;

FSSpec		gDataSpec;


//======================================================================================
//======================================================================================
//======================================================================================


void InitDefaultPrefs(void)
{
	memset(&gGamePrefs, 0, sizeof(gGamePrefs));
	snprintf(gGamePrefs.magic, sizeof(gGamePrefs.magic), "%s", PREFS_MAGIC);

	gGamePrefs.highQualityTextures = true;			// set the defaults
	gGamePrefs.canDoFog = true;
	gGamePrefs.shadows = true;
	gGamePrefs.dust = true;
	gGamePrefs.fullscreen = true;
	gGamePrefs.preferredDisplay = 0;
	gGamePrefs.antialiasingLevel = 0;
	gGamePrefs.vsync = true;
	gGamePrefs.mainMenuHelp = true;
	gGamePrefs.extreme = false;
	gGamePrefs.music = true;
	gGamePrefs.ambientSounds = true;
	gGamePrefs.nanosaurTeethFix = true;
	gGamePrefs.whiteSky = true;

	memcpy(gGamePrefs.keys, kDefaultKeyBindings, sizeof(gGamePrefs.keys));
	_Static_assert(sizeof(kDefaultKeyBindings) == sizeof(gGamePrefs.keys), "size mismatch: default keybindings / prefs keybinings");
}

/*****************/
/* TOOLBOX INIT  */
/*****************/

void ToolBoxInit(void)
{
OSErr		iErr;

//	gMainAppRezFile = CurResFile();


			/* MAKE FSSPEC FOR DATA FOLDER */

	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons", &gDataSpec);
	if (iErr)
		DoFatalAlert("Cannot locate the Nanosaur Data folder.");



			/* LOAD QD3D & QT */
			
	QD3D_Boot();


			/* INIT PREFERENCES */

	InitDefaultPrefs();
	LoadPrefs(&gGamePrefs);							// attempt to read from prefs file

	SetFullscreenMode(true);
	SetProModeSettings(gGamePrefs.extreme);



			/* INIT SOME OF MY STUFF */

	InitSkeletonManager();
	InitSoundTools();
	Init3DMFManager();
	InitObjectManager();
	InitSpriteManager();
}


/***************** INIT LEVEL ************************/

static void InitLevel(void)
{
QD3DSetupInputType		viewDef;
TQ3ColorRGB		c1 = { 1.0, 1, 1 };
TQ3ColorRGB		c2 = { 1, .9, .6 };

	PlaySong(0,true);

			/* MAKE VIEW */

	QD3D_NewViewDef(&viewDef);
	
#if TWO_MEG_VERSION
	viewDef.view.paneClip.left 		+= 144;  
	viewDef.view.paneClip.right 	+= 32;  
	viewDef.view.paneClip.top		+= 40;  
	viewDef.view.paneClip.bottom 	+= 140;  
	viewDef.lights.ambientBrightness = 0.3;
#else	
	viewDef.view.paneClip.left 		+= 118;  
	viewDef.view.paneClip.right 	+= 12;  
	viewDef.view.paneClip.top		+= 9;  
	viewDef.view.paneClip.bottom 	+= 110;  
	viewDef.lights.ambientBrightness = 0.2;
#endif	
	viewDef.camera.hither 			= HITHER_DISTANCE;
	viewDef.camera.yon 				= YON_DISTANCE;
	viewDef.camera.fov 				= 1.0;
	viewDef.lights.fillColor[0] 	= c1;
	viewDef.lights.fillColor[1] 	= c2;
	viewDef.lights.fillBrightness[0] = 1.2;
	viewDef.lights.fillBrightness[1] = .4;

	viewDef.view.keepBackdropAspectRatio = gGamePrefs.force4x3;

	if (!gGamePrefs.whiteSky)
	{
		viewDef.view.clearColor.r = 0;
		viewDef.view.clearColor.g = 0;
		viewDef.view.clearColor.b = 0;
	}
	else
	{
		viewDef.view.clearColor.r = .95;
		viewDef.view.clearColor.g = .95;
		viewDef.view.clearColor.b = .75;
	}
	
	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);
	QD3D_DrawScene(gGameViewInfoPtr, nil);


				/* INIT FLAGS */

	gGameOverFlag = false;
	gPlayerGotKilledFlag = false;
	gWonGameFlag = false;
	gMyHealth = 1.0;

			/* LOAD ART */

	MakeShadowTexture();

	LoadLevelArt(LEVEL_NUM_0);

	QD3D_InitShards();	
	InitWeaponManager();
	InitItemsManager();
	InitMyInventory();	
	InitInfobar();

		
		/* INIT THE PLAYER */
			
	InitMyGuy();
	InitEnemyManager();	
					
			/* INIT CAMERA */
			
	MakeCameraEvent();
	
			/* PREP THE TERRAIN */

	InitTerrainManager();
	PrimeInitialTerrain();
	InitTimePortals();
	
	StartAmbientEffect();
}


/**************** CLEANUP LEVEL **********************/

static void CleanupLevel(void)
{
	StopAllEffectChannels();
	Render_FreezeFrameFadeOut();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeTerrain();
	DisposeSpriteGroup(0);
	QD3D_DisposeShards();
	DeleteAll3DMFGroups();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
}


/**************** PLAY LEVEL ************************/

static void PlayLevel(void)
{
float killDelay = KILL_DELAY;						// time to wait after I'm dead before fading out
float	fps;
FSSpec	spec;

			/* INIT LEVEL */
			
	InitLevel();

	gGameOverFlag = false;

	MakeFadeEvent(true);
	QD3D_CalcFramesPerSecond();
		

		/******************/
		/* MAIN GAME LOOP */
		/******************/
	
	while(true)
	{
		fps = gFramesPerSecondFrac;
		
		UpdateInput();


				/* MOVE OBJECTS */
				
		CalcPlayerKeyControls();
		MoveObjects();
		QD3D_MoveShards();

				/* SPECIFIC MAINTENANCE */
				
		UpdateLavaTextureAnimation();
		UpdateWaterTextureAnimation();
		DecAsteroidTimer();
	
	
				/* DRAW OBJECTS & TERRAIN */
					
		DoMyTerrainUpdate();
		UpdateInfobar();
		QD3D_DrawScene(gGameViewInfoPtr,DrawTerrain);
		QD3D_CalcFramesPerSecond();

				
			/* SEE IF PAUSE GAME */

		if (GetNewNeedState(kNeed_UIPause)						// see if pause/abort
			|| IsCmdQPressed())
		{
			DoPaused();
		}


			/* CHECK CHEAT KEYS */
			
		if (GetSDLKeyState(SDL_SCANCODE_F15) || GetSDLKeyState(SDL_SCANCODE_F12))
		{
			if (GetNewSDLKeyState(SDL_SCANCODE_F1))				// get health
				GetHealth(1);
			else
			if (GetNewSDLKeyState(SDL_SCANCODE_F2))				// get shield
				StartMyShield(gPlayerObj);
			else
			if (GetNewSDLKeyState(SDL_SCANCODE_F3))				// get full weaponry
				GetCheatWeapons();
			else
			if (GetNewSDLKeyState(SDL_SCANCODE_F4))				// get all eggs
				GetAllEggsCheat();
			else
			if (GetNewSDLKeyState(SDL_SCANCODE_F5))				// get fuel
			{
				gFuel = MAX_FUEL_CAPACITY;
				gInfobarUpdateBits |= UPDATE_FUEL;
			}
				
		}

			/* SEE IF GAME ENDED */				
		
		if (gGameOverFlag)
			break;
			
			
			/* SEE IF GOT KILLED */
				
		if (gPlayerGotKilledFlag)				// if got killed, then hang around for a few seconds before resetting player
		{
			killDelay -= fps;					
			if (killDelay < 0.0f)				// see if time to reset player
			{
				ResetPlayer();					// reset player
				killDelay = KILL_DELAY;			// reset kill timer for next death
			}
		}	
	}

			/* CLEANUP */
	CleanupLevel();


			/* PLAY WIN MOVIE */
	
	if (gWonGameFlag)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":movies:Win.mov", &spec);
		PlayAMovie(&spec);
	}
	
			/* PLAY LOSE MOVIE */
	else
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":movies:Lose.mov", &spec);
		PlayAMovie(&spec);
	}


			/* DO HIGH SCORES */
		
	if (!gSongPlayingFlag)						// make sure music is going
		PlaySong(1,true);		
		
	ShowHighScoresScreen(gScore);
}



/************************************************************/
/******************** PROGRAM MAIN ENTRY  *******************/
/************************************************************/


void GameMain(void)
{
unsigned long	someLong;

				/**************/
				/* BOOT STUFF */
				/**************/
				
	ToolBoxInit();
 	 		 	 		
 	InitInput();

	GetDateTime ((unsigned long *)(&someLong));		// init random seed
	SetMyRandomSeed(someLong);
	
	ShowCharity();

	LoadSoundBank();								// load sound bank for entire game

	while(true)
	{
		DoPangeaLogo();
		DoTitleScreen();
		DoMainMenu();
		PlayLevel();
	}
}




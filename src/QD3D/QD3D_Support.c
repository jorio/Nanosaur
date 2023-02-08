/****************************/
/*   	QD3D SUPPORT.C	    */
/* By Brian Greenstone      */
/* (c)1997 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void CreateLights(QD3DLightDefType *lightDefPtr);
static TQ3Area GetAdjustedPane(Rect paneClip);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

SDL_GLContext					gGLContext = NULL;
RenderStats						gRenderStats;

GLuint 							gShadowGLTextureName = 0;

float	gFramesPerSecond = MIN_FPS;				// this is used to maintain a constant timing velocity as frame rates differ
float	gFramesPerSecondFrac = 1.0f/MIN_FPS;

float	gAdditionalClipping = 0;

int		gWindowWidth		= GAME_VIEW_WIDTH;
int		gWindowHeight		= GAME_VIEW_HEIGHT;

static const uint32_t	gDebugTextUpdateInterval = 50;
static uint32_t			gDebugTextFrameAccumulator = 0;
static uint32_t			gDebugTextLastUpdatedAt = 0;
static char				gDebugTextBuffer[1024];


/******************** QD3D: BOOT ******************************/

void QD3D_Boot(void)
{
				/* LET 'ER RIP! */

	GAME_ASSERT_MESSAGE(gSDLWindow, "Gotta have a window to create a context");
	GAME_ASSERT_MESSAGE(!gGLContext, "GL context already created!");

	gGLContext = SDL_GL_CreateContext(gSDLWindow);									// also makes it current
	GAME_ASSERT_MESSAGE(gGLContext, SDL_GetError());
}


/******************** QD3D: SHUTDOWN ***************************/

void QD3D_Shutdown(void)
{
	if (gGLContext)
	{
		SDL_GL_DeleteContext(gGLContext);
		gGLContext = NULL;
	}
}


//=======================================================================================================
//=============================== VIEW WINDOW SETUP STUFF ===============================================
//=======================================================================================================


/*********************** QD3D: NEW VIEW DEF **********************/
//
// fills a view def structure with default values.
//

void QD3D_NewViewDef(QD3DSetupInputType *viewDef)
{
TQ3ColorRGBA		clearColor = {0,0,0,1};
TQ3Point3D			cameraFrom = { 0, 40, 200.0 };
TQ3Point3D			cameraTo = { 0, 0, 0 };
TQ3Vector3D			cameraUp = { 0.0, 1.0, 0.0 };
TQ3ColorRGB			ambientColor = { 1.0, 1.0, 1.0 };
TQ3Vector3D			fillDirection1 = { 1, -.7, -1 };
TQ3Vector3D			fillDirection2 = { -1, -1, .2 };

	Q3Vector3D_Normalize(&fillDirection1,&fillDirection1);
	Q3Vector3D_Normalize(&fillDirection2,&fillDirection2);

	viewDef->view.clearColor 		= clearColor;
	viewDef->view.paneClip.left 	= 0;
	viewDef->view.paneClip.right 	= 0;
	viewDef->view.paneClip.top 		= 0;
	viewDef->view.paneClip.bottom 	= 0;
	viewDef->view.keepBackdropAspectRatio = true;

	viewDef->styles.interpolation 	= kQ3InterpolationStyleVertex;
	viewDef->styles.backfacing 		= kQ3BackfacingStyleRemove; 
	viewDef->styles.fill			= kQ3FillStyleFilled; 
	viewDef->styles.usePhong 		= false; 

	viewDef->camera.from 			= cameraFrom;
	viewDef->camera.to 				= cameraTo;
	viewDef->camera.up 				= cameraUp;
	viewDef->camera.hither 			= 10;
	viewDef->camera.yon 			= 3000;
	viewDef->camera.fov 			= .9;

	viewDef->lights.ambientBrightness = 0.25;
	viewDef->lights.ambientColor 	= ambientColor;
	viewDef->lights.numFillLights 	= 1;
	viewDef->lights.fillDirection[0] = fillDirection1;
	viewDef->lights.fillDirection[1] = fillDirection2;
	viewDef->lights.fillColor[0] 	= ambientColor;
	viewDef->lights.fillColor[1] 	= ambientColor;
	viewDef->lights.fillBrightness[0] = 1.3;
	viewDef->lights.fillBrightness[1] = .3;
	
	viewDef->lights.useFog = true;
	viewDef->lights.fogHither = .4;
	viewDef->lights.fogYon = 1.0;
	viewDef->lights.fogMode = kQ3FogModePlaneBasedLinear;

}

/************** SETUP QD3D WINDOW *******************/

void QD3D_SetupWindow(QD3DSetupInputType *setupDefPtr, QD3DSetupOutputType **outputHandle)
{
QD3DSetupOutputType	*outputPtr;

			/* ALLOC MEMORY FOR OUTPUT DATA */

	*outputHandle = (QD3DSetupOutputType *)AllocPtr(sizeof(QD3DSetupOutputType));
	GAME_ASSERT(*outputHandle);
	outputPtr = *outputHandle;

			/* CREATE & SET DRAW CONTEXT */

	GAME_ASSERT(gGLContext);

				/* PASS BACK INFO */

	outputPtr->paneClip = setupDefPtr->view.paneClip;
	outputPtr->needScissorTest = setupDefPtr->view.paneClip.left != 0 || setupDefPtr->view.paneClip.right != 0
								 || setupDefPtr->view.paneClip.bottom != 0 || setupDefPtr->view.paneClip.top != 0;
	outputPtr->keepBackdropAspectRatio = setupDefPtr->view.keepBackdropAspectRatio;
	outputPtr->hither = setupDefPtr->camera.hither;				// remember hither/yon
	outputPtr->yon = setupDefPtr->camera.yon;
	outputPtr->fov = setupDefPtr->camera.fov;
	outputPtr->viewportAspectRatio = 1;		// filled in later
	outputPtr->clearColor = setupDefPtr->view.clearColor;

	outputPtr->cameraPlacement.upVector				= setupDefPtr->camera.up;
	outputPtr->cameraPlacement.pointOfInterest		= setupDefPtr->camera.to;
	outputPtr->cameraPlacement.cameraLocation		= setupDefPtr->camera.from;

	outputPtr->lights = setupDefPtr->lights;

	outputPtr->isActive = true;							// it's now an active structure

	TQ3Vector3D v = {0, 0, 0};
	QD3D_MoveCameraFromTo(outputPtr,&v,&v);				// call this to set outputPtr->currentCameraCoords & camera matrix

			/* SET UP OPENGL RENDERER PROPERTIES NOW THAT WE HAVE A CONTEXT */

	Render_InitState();									// set up default GL state

	CreateLights(&setupDefPtr->lights);

	if (setupDefPtr->lights.useFog && gGamePrefs.canDoFog)
	{
		float camHither = setupDefPtr->camera.hither;
		float camYon	= setupDefPtr->camera.yon;
		float fogHither	= setupDefPtr->lights.fogHither;
		float fogYon	= setupDefPtr->lights.fogYon;
		glEnable(GL_FOG);
		glHint(GL_FOG_HINT,		GL_NICEST);
		glFogi(GL_FOG_MODE,		GL_LINEAR);
		glFogf(GL_FOG_START,	camHither + fogHither * (camYon - camHither));
		glFogf(GL_FOG_END,		camHither + fogYon    * (camYon - camHither));
		glFogfv(GL_FOG_COLOR,	&setupDefPtr->view.clearColor.r);
		//glFogf(GL_FOG_DENSITY,	0.5f);
	}
	else
		glDisable(GL_FOG);

	Render_AllocBackdrop(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT);	// creates GL texture for backdrop
	Render_ClearBackdrop(0);									// avoid residual junk from previous backdrop
}


/***************** QD3D_DisposeWindowSetup ***********************/
//
// Disposes of all data created by QD3D_SetupWindow
//
// Make sure all GL textures have been freed before calling this, as this will destroy the GL context.
//

void QD3D_DisposeWindowSetup(QD3DSetupOutputType **dataHandle)
{
QD3DSetupOutputType	*data;

	data = *dataHandle;
	GAME_ASSERT(data);										// see if this setup exists

	Render_DisposeBackdrop();								// deletes GL texture for backdrop

	if (gShadowGLTextureName != 0)
	{
		glDeleteTextures(1, &gShadowGLTextureName);
		gShadowGLTextureName = 0;
	}

	data->isActive = false;									// now inactive
	
		/* FREE MEMORY & NIL POINTER */
		
	DisposePtr((Ptr)data);
	*dataHandle = nil;
}



/**************** SET STYLES ****************/
//
// Creates style objects which define how the scene is to be rendered.
// It also sets the shader object.
//

/********************* CREATE LIGHTS ************************/

static void CreateLights(QD3DLightDefType *lightDefPtr)
{
//	glEnable(GL_LIGHTING);	// controlled by renderer.c

			/************************/
			/* CREATE AMBIENT LIGHT */
			/************************/

	if (lightDefPtr->ambientBrightness != 0)						// see if ambient exists
	{
		GLfloat ambient[4] =
		{
			lightDefPtr->ambientBrightness * lightDefPtr->ambientColor.r,
			lightDefPtr->ambientBrightness * lightDefPtr->ambientColor.g,
			lightDefPtr->ambientBrightness * lightDefPtr->ambientColor.b,
			1
		};
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	}

			/**********************/
			/* CREATE FILL LIGHTS */
			/**********************/

	for (int i = 0; i < lightDefPtr->numFillLights; i++)
	{
		static GLfloat lightamb[4] = { 0.0, 0.0, 0.0, 1.0 };
		GLfloat lightVec[4];
		GLfloat	diffuse[4];

					/* SET FILL DIRECTION */

		Q3Vector3D_Normalize(&lightDefPtr->fillDirection[i], &lightDefPtr->fillDirection[i]);
		lightVec[0] = -lightDefPtr->fillDirection[i].x;		// negate vector because OGL is stupid
		lightVec[1] = -lightDefPtr->fillDirection[i].y;
		lightVec[2] = -lightDefPtr->fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);

					/* SET COLOR */

		glLightfv(GL_LIGHT0+i, GL_AMBIENT, lightamb);

		diffuse[0] = lightDefPtr->fillColor[i].r * lightDefPtr->fillBrightness[i];
		diffuse[1] = lightDefPtr->fillColor[i].g * lightDefPtr->fillBrightness[i];
		diffuse[2] = lightDefPtr->fillColor[i].b * lightDefPtr->fillBrightness[i];
		diffuse[3] = 1;

		glLightfv(GL_LIGHT0+i, GL_DIFFUSE, diffuse);

		glEnable(GL_LIGHT0+i);								// enable the light
	}

			/* KILL OTHER LIGHTS */

	for (int i = lightDefPtr->numFillLights; i < MAX_FILL_LIGHTS; i++)
	{
		glDisable(GL_LIGHT0+i);
	}
}





/******************* QD3D DRAW SCENE *********************/

void QD3D_DrawScene(QD3DSetupOutputType *setupInfo, void (*drawRoutine)(QD3DSetupOutputType *))
{
	GAME_ASSERT(setupInfo);
	GAME_ASSERT(setupInfo->isActive);							// make sure it's legit
	GAME_ASSERT(gGLContext);

			/* CALC VIEWPORT DIMENSIONS */

	SDL_GL_GetDrawableSize(gSDLWindow, &gWindowWidth, &gWindowHeight);
	TQ3Area viewportPane = GetAdjustedPane(setupInfo->paneClip);
	float viewportWidth = viewportPane.max.x - viewportPane.min.x;
	float viewportHeight = viewportPane.max.y - viewportPane.min.y;
	setupInfo->viewportAspectRatio = (viewportHeight < 1) ? (1.0f) : (viewportWidth / viewportHeight);

			/* UPDATE CAMERA MATRICES */

	CalcCameraMatrixInfo(setupInfo);

			/* START RENDERING */

	Render_StartFrame();

			/* DRAW BACKDROP */

	if (setupInfo->needScissorTest)
	{
		Render_DrawBackdrop(setupInfo->keepBackdropAspectRatio);
	}
	else if (gGamePrefs.force4x3 && (viewportPane.min.x != 0 || viewportPane.min.y != 0))
	{
		Render_DrawBackdrop(true);		// Forces clearing pillarbox/letterbox zones
	}

			/* ENTER 3D VIEWPORT */

	Render_SetViewportClearColor(setupInfo->clearColor);
	Render_SetViewport(viewportPane);

			/* PREPARE FRUSTUM CULLING PLANES */

	UpdateFrustumPlanes();

			/* 3D SCENE RENDER LOOP */

	if (drawRoutine)
		drawRoutine(setupInfo);

			/* DONE RENDERING */

	Render_EndFrame();

	SDL_GL_SwapWindow(gSDLWindow);
}


//=======================================================================================================
//=============================== CAMERA STUFF ==========================================================
//=======================================================================================================

#pragma mark ---------- camera -------------

/*************** QD3D_UpdateCameraFromTo ***************/

void QD3D_UpdateCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Point3D *from, TQ3Point3D *to)
{
	setupInfo->cameraPlacement.pointOfInterest = *to;					// set camera look at
	setupInfo->cameraPlacement.cameraLocation = *from;					// set camera coords
	CalcCameraMatrixInfo(setupInfo);									// update matrices
}


/*************** QD3D_UpdateCameraFrom ***************/

void QD3D_UpdateCameraFrom(QD3DSetupOutputType *setupInfo, TQ3Point3D *from)
{
	setupInfo->cameraPlacement.cameraLocation = *from;					// set camera coords
	CalcCameraMatrixInfo(setupInfo);									// update matrices
}


/*************** QD3D_MoveCameraFromTo ***************/

void QD3D_MoveCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Vector3D *moveVector, TQ3Vector3D *lookAtVector)
{
	setupInfo->cameraPlacement.cameraLocation.x += moveVector->x;		// set camera coords
	setupInfo->cameraPlacement.cameraLocation.y += moveVector->y;
	setupInfo->cameraPlacement.cameraLocation.z += moveVector->z;

	setupInfo->cameraPlacement.pointOfInterest.x += lookAtVector->x;	// set camera look at
	setupInfo->cameraPlacement.pointOfInterest.y += lookAtVector->y;
	setupInfo->cameraPlacement.pointOfInterest.z += lookAtVector->z;

	CalcCameraMatrixInfo(setupInfo);									// update matrices
}


//=======================================================================================================
//=============================== MISC ==================================================================
//=======================================================================================================

/************** QD3D REFRESH WINDOW SIZE *****************/

void QD3D_OnWindowResized(void)
{
	SDL_GL_GetDrawableSize(gSDLWindow, &gWindowWidth, &gWindowHeight);
}

/************** QD3D CALC FRAMES PER SECOND *****************/

void QD3D_CalcFramesPerSecond(void)
{
	static uint64_t performanceFrequency = 0;
	static uint64_t prevTime = 0;
	uint64_t currTime;

	if (performanceFrequency == 0)
	{
		performanceFrequency = SDL_GetPerformanceFrequency();
	}

	slow_down:
	currTime = SDL_GetPerformanceCounter();
	uint64_t deltaTime = currTime - prevTime;

	if (deltaTime <= 0)
	{
		gFramesPerSecond = MIN_FPS;						// avoid divide by 0
	}
	else
	{
		gFramesPerSecond = performanceFrequency / (float)(deltaTime);

		if (gFramesPerSecond > MAX_FPS)					// keep from cooking the GPU
		{
			if (gFramesPerSecond - MAX_FPS > 1000)		// try to sneak in some sleep if we have 1 ms to spare
			{
				SDL_Delay(1);
			}
			goto slow_down;
		}

		if (gFramesPerSecond < MIN_FPS)					// (avoid divide by 0's later)
		{
			gFramesPerSecond = MIN_FPS;
		}
	}

	// In debug builds, speed up with KP_PLUS
#if _DEBUG
	if (GetSDLKeyState(SDL_SCANCODE_KP_PLUS))
#else
	if (GetSDLKeyState(SDL_SCANCODE_GRAVE) && GetSDLKeyState(SDL_SCANCODE_KP_PLUS))
#endif
	{
		gFramesPerSecond = MIN_FPS;
	}

	gFramesPerSecondFrac = 1.0f / gFramesPerSecond;		// calc fractional for multiplication

	prevTime = currTime;								// reset for next time interval


			/* UPDATE DEBUG TEXT */

	if (gGamePrefs.debugInfoInTitleBar)
	{
		uint32_t ticksNow = SDL_GetTicks();
		uint32_t ticksElapsed = ticksNow - gDebugTextLastUpdatedAt;
		if (ticksElapsed >= gDebugTextUpdateInterval)
		{
			float fps = 1000 * gDebugTextFrameAccumulator / (float)ticksElapsed;
			snprintf(
					gDebugTextBuffer, sizeof(gDebugTextBuffer),
					"%s%s - %dfps %dt %dm %dn %dp %dK x:%.0f y:%.0f z:%.0f",
					PRO_MODE ? "NanoExtreme" : "Nanosaur",
					PROJECT_VERSION,
					(int)round(fps),
					gRenderStats.trianglesDrawn,
					gRenderStats.meshQueueSize,
					gObjNodePool? Pool_Size(gObjNodePool): 0,
					(int)Pomme_GetNumAllocs(),
					(int)(Pomme_GetHeapSize()/1024),
					gMyCoord.x,
					gMyCoord.y,
					gMyCoord.z
			);
			SDL_SetWindowTitle(gSDLWindow, gDebugTextBuffer);
			gDebugTextFrameAccumulator = 0;
			gDebugTextLastUpdatedAt = ticksNow;
		}
		gDebugTextFrameAccumulator++;
	}
}



/*********************** MAKE SHADOW TEXTURE ***************************/
//
// The shadow is acutally just an alpha map, but we load in the PICT resource, then
// convert it to a texture, then convert the texture to an alpha channel with a texture of all white.
//

void MakeShadowTexture(void)
{
	if (gShadowGLTextureName != 0)			// already allocated
		return;

			/* LOAD IMAGE FROM RESOURCE */

	FSSpec spec;
	OSErr err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:Shadow.tga", &spec);
	GAME_ASSERT(!err);

	PicHandle picHandle = GetPictureFromTGA(&spec);
	GAME_ASSERT(picHandle);
	GAME_ASSERT(*picHandle);


		/* GET QD3D PIXMAP DATA INFO */

	int16_t width = (**picHandle).picFrame.right - (**picHandle).picFrame.left;
	int16_t height = (**picHandle).picFrame.bottom - (**picHandle).picFrame.top;
	uint32_t *const pixelData =  (uint32_t*) (**picHandle).__pomme_pixelsARGB32;


			/* REMAP THE ALPHA */

	uint32_t* pixelPtr = pixelData;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			// put Blue into Alpha & leave map white
			unsigned char* argb = (unsigned char *)(&pixelPtr[x]);
			argb[0] = argb[3];	// put blue into alpha
			argb[1] = 255;
			argb[2] = 255;
			argb[3] = 255;
		}

		pixelPtr += width;
	}


		/* UPDATE THE MAP */

	GAME_ASSERT_MESSAGE(gShadowGLTextureName == 0, "shadow texture already allocated");

	gShadowGLTextureName = Render_LoadTexture(
			GL_RGBA,
			width,
			height,
			GL_BGRA,
			GL_UNSIGNED_INT_8_8_8_8,
			pixelData,
			0
			);

	DisposeHandle((Handle) picHandle);
}


#pragma mark -

static TQ3Area GetAdjustedPane(Rect paneClip)
{
	TQ3Area pane;
	TQ3Vector2D scale;
	TQ3Vector2D offset;

	pane.min.x = paneClip.left;					// set bounds?
	pane.max.x = GAME_VIEW_WIDTH - paneClip.right;
	pane.min.y = paneClip.bottom;							// bottom is min for OpenGL viewport
	pane.max.y = GAME_VIEW_HEIGHT - paneClip.top;			// top is max for OpenGL viewport

	pane.min.x += gAdditionalClipping;						// offset bounds by user clipping
	pane.max.x -= gAdditionalClipping;
	pane.min.y += gAdditionalClipping*.75f;
	pane.max.y -= gAdditionalClipping*.75f;

	if (gGamePrefs.force4x3)
	{
		TQ3Vector2D fit = FitRectKeepAR(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT, gWindowWidth, gWindowHeight);
		scale.x = fit.x / (float) GAME_VIEW_WIDTH;
		scale.y = fit.y / (float) GAME_VIEW_HEIGHT;
		offset.x = (gWindowWidth  - fit.x) / 2;
		offset.y = (gWindowHeight - fit.y) / 2;
	}
	else
	{
		scale.x = gWindowWidth	/ (float) GAME_VIEW_WIDTH;
		scale.y = gWindowHeight	/ (float) GAME_VIEW_HEIGHT;
		offset.x = 0;
		offset.y = 0;
	}

	pane.min.x *= scale.x;	// scale clip pane to window size
	pane.max.x *= scale.x;
	pane.min.y *= scale.y;
	pane.max.y *= scale.y;

	pane.min.x += offset.x;
	pane.max.x += offset.x;
	pane.min.y += offset.y;
	pane.max.y += offset.y;

	return pane;
}

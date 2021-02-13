/****************************/
/*   	QD3D SUPPORT.C	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <SDL.h>
#include <QD3D.h>
#include <QD3DMath.h>

#if __APPLE__
	#include <OpenGL/glu.h>		// gluPerspective
#else
	#include <GL/glu.h>			// gluPerspective
#endif

#include <frustumculling.h>
#include <stdio.h>

#include "globals.h"
#include "misc.h"
#include "qd3d_support.h"
#include "input.h"
#include "windows_nano.h"
#include "camera.h"
#include "3dmath.h"
#include "environmentmap.h"

#include "renderer.h"

extern	SDL_Window	*gSDLWindow;
extern	long		gScreenXOffset,gScreenYOffset;
extern	PrefsType	gGamePrefs;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;


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

SDL_GLContext					gGLContext;
RenderStats						gRenderStats;

GLuint 							gShadowGLTextureName = 0;

float	gFramesPerSecond = DEFAULT_FPS;				// this is used to maintain a constant timing velocity as frame rates differ
float	gFramesPerSecondFrac = 1/DEFAULT_FPS;

float	gAdditionalClipping = 0;

int		gWindowWidth		= GAME_VIEW_WIDTH;
int		gWindowHeight		= GAME_VIEW_HEIGHT;


/******************** QD3D: BOOT ******************************/

void QD3D_Boot(void)
{
				/* LET 'ER RIP! */
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

#if 0	// NOQUESA
	if (theWindow == nil)
		viewDef->view.useWindow 	=	false;							// assume going to pixmap
	else
		viewDef->view.useWindow 	=	true;							// assume going to window
	viewDef->view.displayWindow 	= theWindow;
	viewDef->view.rendererType      = kQ3RendererTypeOpenGL;//kQ3RendererTypeInteractive;
#endif
	viewDef->view.clearColor 		= clearColor;
	viewDef->view.paneClip.left 	= 0;
	viewDef->view.paneClip.right 	= 0;
	viewDef->view.paneClip.top 		= 0;
	viewDef->view.paneClip.bottom 	= 0;
	viewDef->view.backdropFit		= kCoverQuadFill;

	viewDef->styles.interpolation 	= gGamePrefs.interpolationStyle? kQ3InterpolationStyleVertex: kQ3InterpolationStyleNone; 
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

	gGLContext = SDL_GL_CreateContext(gSDLWindow);									// also makes it current
	GAME_ASSERT(gGLContext);

				/* PASS BACK INFO */

	outputPtr->paneClip = setupDefPtr->view.paneClip;
	outputPtr->needScissorTest = setupDefPtr->view.paneClip.left != 0 || setupDefPtr->view.paneClip.right != 0
								 || setupDefPtr->view.paneClip.bottom != 0 || setupDefPtr->view.paneClip.top != 0;
	outputPtr->backdropFit = setupDefPtr->view.backdropFit;
	outputPtr->hither = setupDefPtr->camera.hither;				// remember hither/yon
	outputPtr->yon = setupDefPtr->camera.yon;
	outputPtr->fov = setupDefPtr->camera.fov;

	outputPtr->cameraPlacement.upVector				= setupDefPtr->camera.up;
	outputPtr->cameraPlacement.pointOfInterest		= setupDefPtr->camera.to;
	outputPtr->cameraPlacement.cameraLocation		= setupDefPtr->camera.from;

	outputPtr->lights = setupDefPtr->lights;

	outputPtr->isActive = true;							// it's now an active structure

	TQ3Vector3D v = {0, 0, 0};
	QD3D_MoveCameraFromTo(outputPtr,&v,&v);				// call this to set outputPtr->currentCameraCoords & camera matrix

			/* SET UP OPENGL RENDERER PROPERTIES NOW THAT WE HAVE A CONTEXT */

	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);

	CreateLights(&setupDefPtr->lights);

	if (setupDefPtr->lights.useFog)
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
		glFogfv(GL_FOG_COLOR,	(float *)&setupDefPtr->view.clearColor);
		//glFogf(GL_FOG_DENSITY,	0.5f);
	}
	else
		glDisable(GL_FOG);

	glAlphaFunc(GL_GREATER, 0.4999f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Normalize normal vectors. Required so lighting looks correct on scaled meshes.
	glEnable(GL_NORMALIZE);

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);									// CCW is front face

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	Render_InitState();
	Render_Alloc2DCover(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT);

	glClearColor(setupDefPtr->view.clearColor.r, setupDefPtr->view.clearColor.g, setupDefPtr->view.clearColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CHECK_GL_ERROR();
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

	Render_Dispose2DCover(); // Source port addition - release backdrop GL texture

	if (gShadowGLTextureName != 0)
	{
		glDeleteTextures(1, &gShadowGLTextureName);
		gShadowGLTextureName = 0;
	}

	SDL_GL_DeleteContext(gGLContext);						// dispose GL context
	gGLContext = nil;

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
}





/******************* QD3D DRAW SCENE *********************/

void QD3D_DrawScene(QD3DSetupOutputType *setupInfo, void (*drawRoutine)(QD3DSetupOutputType *))
{
	GAME_ASSERT(setupInfo);
	GAME_ASSERT(setupInfo->isActive);							// make sure it's legit


			/* START RENDERING */

	int mkc = SDL_GL_MakeCurrent(gSDLWindow, gGLContext);
	GAME_ASSERT_MESSAGE(mkc == 0, SDL_GetError());

	Render_StartFrame();

	// Clip pane
	if (setupInfo->needScissorTest)
	{
		// Render backdrop
		Render_Draw2DCover(setupInfo->backdropFit);

		// Set scissor
		TQ3Area pane	= GetAdjustedPane(setupInfo->paneClip);
		int paneWidth	= pane.max.x-pane.min.x;
		int paneHeight	= pane.max.y-pane.min.y;
		glEnable(GL_SCISSOR_TEST);
		glScissor	(pane.min.x, pane.min.y, paneWidth, paneHeight);
		glViewport	(pane.min.x, pane.min.y, paneWidth, paneHeight);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	else
	{
		glViewport(0, 0, gWindowWidth, gWindowHeight);
	}

			/* PREPARE FRUSTUM PLANES FOR SPHERE VISIBILITY CHECKS */
			// (Source port addition)

	UpdateFrustumPlanes();


			/***************/
			/* RENDER LOOP */
			/***************/

	if (drawRoutine)
		drawRoutine(setupInfo);

			/******************/
			/* DONE RENDERING */
			/*****************/

	if (setupInfo->needScissorTest)
	{
		glDisable(GL_SCISSOR_TEST);
	}

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

/************** QD3D CALC FRAMES PER SECOND *****************/

void	QD3D_CalcFramesPerSecond(void)
{
UnsignedWide	wide;
unsigned long	now;
static	unsigned long then = 0;

			/* DO REGULAR CALCULATION */
			
	Microseconds(&wide);
	now = wide.lo;
	if (then != 0)
	{
		gFramesPerSecond = 1000000.0f/(float)(now-then);
		if (gFramesPerSecond < DEFAULT_FPS)			// (avoid divide by 0's later)
			gFramesPerSecond = DEFAULT_FPS;
	}
	else
		gFramesPerSecond = DEFAULT_FPS;
		
//	gFramesPerSecondFrac = 1/gFramesPerSecond;		// calc fractional for multiplication
	gFramesPerSecondFrac = __fres(gFramesPerSecond);	
	
	then = now;										// remember time	
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

	PicHandle picHandle = GetPicture(129);
	GAME_ASSERT(picHandle);


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


#pragma mark ---------- source port additions -------------

static TQ3Area GetAdjustedPane(Rect paneClip)
{
	TQ3Area pane;

	pane.min.x = paneClip.left;					// set bounds?
	pane.max.x = GAME_VIEW_WIDTH - paneClip.right;
	pane.min.y = paneClip.bottom;							// bottom is min for OpenGL viewport
	pane.max.y = GAME_VIEW_HEIGHT - paneClip.top;			// top is max for OpenGL viewport

	pane.min.x += gAdditionalClipping;						// offset bounds by user clipping
	pane.max.x -= gAdditionalClipping;
	pane.min.y += gAdditionalClipping*.75f;
	pane.max.y -= gAdditionalClipping*.75f;

	pane.min.x *= gWindowWidth	/ (float)(GAME_VIEW_WIDTH);	// scale clip pane to window size
	pane.max.x *= gWindowWidth	/ (float)(GAME_VIEW_WIDTH);
	pane.min.y *= gWindowHeight	/ (float)(GAME_VIEW_HEIGHT);
	pane.max.y *= gWindowHeight	/ (float)(GAME_VIEW_HEIGHT);

	return pane;
}

// Called when the game window gets resized.
// Adjusts the clipping pane and camera aspect ratio.
void QD3D_OnWindowResized(int windowWidth, int windowHeight)
{
	gWindowWidth	= windowWidth;
	gWindowHeight	= windowHeight;

	if (!gGameViewInfoPtr)
		return;

	CalcCameraMatrixInfo(gGameViewInfoPtr);
}



#pragma mark -

float QD3D_GetCurrentViewportAspectRatio(const QD3DSetupOutputType *setupInfo)
{
	int	t,b,l,r;
	t = setupInfo->paneClip.top;
	b = setupInfo->paneClip.bottom;
	l = setupInfo->paneClip.left;
	r = setupInfo->paneClip.right;

	int w = gWindowWidth	- l		- r;
	int h = gWindowHeight	- t		- b;

	if (h == 0)
		return 1;
	else
		return (float)w / (float)h;
}

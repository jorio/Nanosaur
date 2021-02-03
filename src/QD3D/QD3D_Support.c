/****************************/
/*   	QD3D SUPPORT.C	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

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

#include "GamePatches.h" // Source port addition - for backdrop quad

extern	SDL_Window	*gSDLWindow;
extern	long		gScreenXOffset,gScreenYOffset;
extern	PrefsType	gGamePrefs;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	long		gNodesDrawn;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void CreateDrawContext(QD3DViewDefType *viewDefPtr);
static void SetStyles(QD3DStyleDefType *styleDefPtr);
static void CreateCamera(QD3DSetupInputType *setupDefPtr);
static void CreateLights(QD3DLightDefType *lightDefPtr);
static void CreateView(QD3DSetupInputType *setupDefPtr);
#if 0 // TODO noquesa
static void DrawPICTIntoMipmap(PicHandle pict,long width, long height, TQ3Mipmap *mipmap);
static void Data16ToMipmap(Ptr data, short width, short height, TQ3Mipmap *mipmap);
#endif
static void MakeShadowTexture(void);
static TQ3Area GetAdjustedPane(int windowWidth, int windowHeight, Rect paneClip);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

#if 0	// NOQUESA
static TQ3CameraObject			gQD3D_CameraObject;
TQ3GroupObject			gQD3D_LightGroup;
static TQ3ViewObject			gQD3D_ViewObject;
static TQ3RendererObject		gQD3D_RendererObject;
static TQ3ShaderObject			gQD3D_ShaderObject,gQD3D_NullShaderObject;
static	TQ3StyleObject			gQD3D_BackfacingStyle;
static	TQ3StyleObject			gQD3D_FillStyle;
static	TQ3StyleObject			gQD3D_InterpolationStyle;
#endif
SDL_GLContext					gGLContext;

TQ3ShaderObject					gQD3D_gShadowTexture = nil;

float	gFramesPerSecond = DEFAULT_FPS;				// this is used to maintain a constant timing velocity as frame rates differ
float	gFramesPerSecondFrac = 1/DEFAULT_FPS;

float	gAdditionalClipping = 0;

short			gFogMode;
static TQ3FogStyleData			gQD3D_FogStyleData;

// Source port addition: this is a Quesa feature, enabled by default,
// that renders translucent materials more accurately at an angle.
// However, it looks "off" in the game -- shadow quads, shield spheres,
// water patches all appear darker than they would on original hardware.
static TQ3Boolean gQD3D_AngleAffectsAlpha = kQ3False;

static Boolean gQD3D_FreshDrawContext = false;


/******************** QD3D: BOOT ******************************/

void QD3D_Boot(void)
{
				/* LET 'ER RIP! */

			/* MAKE THAT SHADOW THING */

	MakeShadowTexture();
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
TQ3ColorARGB		clearColor = {0,0,0,0};
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
#if 0	// TODO noquesa
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3Vector3D	v = {0,0,0};
TQ3Status	status;
QD3DSetupOutputType	*outputPtr;

			/* ALLOC MEMORY FOR OUTPUT DATA */

	*outputHandle = (QD3DSetupOutputType *)AllocPtr(sizeof(QD3DSetupOutputType));
	if (*outputHandle == nil)
		DoFatalAlert("QD3D_SetupWindow: AllocPtr failed");
	outputPtr = *outputHandle;

				/* SETUP */

	CreateView(setupDefPtr);
	
	CreateCamera(setupDefPtr);										// create new CAMERA object
	CreateLights(&setupDefPtr->lights);
	SetStyles(&setupDefPtr->styles);	
	

				/* DISPOSE OF EXTRA REFERENCES */

#if 1
	printf("TODO noquesa %s():%d\n", __func__, __LINE__);
#else
	status = Q3Object_Dispose(gQD3D_RendererObject);				// (is contained w/in gQD3D_ViewObject)
	if (status == kQ3Failure)
		DoFatalAlert("QD3D_SetupWindow: Q3Object_Dispose failed!");
#endif
	

	
				/* PASS BACK INFO */
#if 1
	printf("TODO noquesa %s():%d\n", __func__, __LINE__);
#else
	outputPtr->viewObject = gQD3D_ViewObject;
	outputPtr->interpolationStyle = gQD3D_InterpolationStyle;
	outputPtr->fillStyle = gQD3D_FillStyle;
	outputPtr->backfacingStyle = gQD3D_BackfacingStyle;
	outputPtr->shaderObject = gQD3D_ShaderObject;
	outputPtr->nullShaderObject = gQD3D_NullShaderObject;
	outputPtr->cameraObject = gQD3D_CameraObject;
	outputPtr->lightGroup = gQD3D_LightGroup;
	outputPtr->drawContext = gQD3D_DrawContext;
	outputPtr->window = setupDefPtr->view.displayWindow;		// remember which window
#endif
	outputPtr->paneClip = setupDefPtr->view.paneClip;
	outputPtr->hither = setupDefPtr->camera.hither;				// remember hither/yon
	outputPtr->yon = setupDefPtr->camera.yon;
	outputPtr->fov = setupDefPtr->camera.fov;

	outputPtr->cameraPlacement.upVector				= setupDefPtr->camera.up;
	outputPtr->cameraPlacement.pointOfInterest		= setupDefPtr->camera.to;
	outputPtr->cameraPlacement.cameraLocation		= setupDefPtr->camera.from;

	outputPtr->isActive = true;							// it's now an active structure

	QD3D_MoveCameraFromTo(outputPtr,&v,&v);				// call this to set outputPtr->currentCameraCoords & camera matrix

#if 0	// TODO noquesa
	Q3DrawContext_SetClearImageColor(gQD3D_DrawContext, &setupDefPtr->view.clearColor); // (source port fix)
#endif

			/* FOG */
			
	if (setupDefPtr->lights.useFog)
	{
		QD3D_SetRaveFog(outputPtr,setupDefPtr->lights.fogHither,setupDefPtr->lights.fogYon,
						&setupDefPtr->view.clearColor,setupDefPtr->lights.fogMode);
	}


			/* SET UP SOME OPENGL RENDERER PROPERTIES */

	glEnable(GL_DEPTH_TEST);

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CHECK_GL_ERROR();

	SDL_GL_SwapWindow(gSDLWindow);
}
#endif


/***************** QD3D_DisposeWindowSetup ***********************/
//
// Disposes of all data created by QD3D_SetupWindow
//

void QD3D_DisposeWindowSetup(QD3DSetupOutputType **dataHandle)
{
QD3DSetupOutputType	*data;

	data = *dataHandle;
	if (data == nil)												// see if this setup exists
		DoFatalAlert("QD3D_DisposeWindowSetup: data == nil");

	DisposeBackdropTexture(); // Source port addition - release backdrop GL texture

	gNodesDrawn = 0;  // Source port addition - reset debug "nodes drawn" counter

#if 1	// NOQUESA
	printf("TODO noquesa: %s\n", __func__);
#else
	Q3Object_Dispose(data->viewObject);
	Q3Object_Dispose(data->interpolationStyle);
	Q3Object_Dispose(data->backfacingStyle);
	Q3Object_Dispose(data->fillStyle);
	Q3Object_Dispose(data->cameraObject);
	Q3Object_Dispose(data->lightGroup);
	Q3Object_Dispose(data->drawContext);
	Q3Object_Dispose(data->shaderObject);
	Q3Object_Dispose(data->nullShaderObject);
#endif
		
	data->isActive = false;									// now inactive
	
		/* FREE MEMORY & NIL POINTER */
		
	DisposePtr((Ptr)data);
	*dataHandle = nil;
}


/******************* CREATE GAME VIEW *************************/

static void CreateView(QD3DSetupInputType *setupDefPtr)
#if 0	// TODO noquesa
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3Status	status;
#if 0 // Source port removal
TQ3Uns32	hints;
#endif

#if 0	// TODO noquesa
				/* CREATE NEW VIEW OBJECT */
				
	gQD3D_ViewObject = Q3View_New();
	if (gQD3D_ViewObject == nil)
		DoFatalAlert("Q3View_New failed!");
#endif


			/* CREATE & SET DRAW CONTEXT */

#if 1	// TODO noquesa
	gGLContext = SDL_GL_CreateContext(gSDLWindow);									// also makes it current
	GAME_ASSERT(gGLContext);

	gQD3D_FreshDrawContext = true;
#else
	CreateDrawContext(&setupDefPtr->view); 											// init draw context

	status = Q3View_SetDrawContext(gQD3D_ViewObject, gQD3D_DrawContext);			// assign context to view
	if (status == kQ3Failure)
		DoFatalAlert("Q3View_SetDrawContext Failed!");
#endif


#if 0	// TODO noquesa
			/* CREATE & SET RENDERER */


	gQD3D_RendererObject = Q3Renderer_NewFromType(setupDefPtr->view.rendererType);	// create new RENDERER object
	if (gQD3D_RendererObject == nil)
	{
		DoFatalAlert("Q3Renderer_NewFromType Failed!");
	}

#if 0 // Source port removal - deprecated by Quesa
	status = Q3InteractiveRenderer_SetPreferences(gQD3D_RendererObject, kQAVendor_BestChoice, kQAEngine_AppleHW);
	if (status == kQ3Failure)
		DoFatalAlert("Q3InteractiveRenderer_SetPreferences Failed!");
#endif
	
	status = Q3View_SetRenderer(gQD3D_ViewObject, gQD3D_RendererObject);				// assign renderer to view
	if (status == kQ3Failure)
		DoFatalAlert("Q3View_SetRenderer Failed!");
		
		
		/* SET RENDERER FEATURES */
		
#if 0 // Source port note: not needed with modern Quesa. Except for 16-bit dithering if we want perfect accuracy.
	Q3InteractiveRenderer_GetRAVEContextHints(gQD3D_RendererObject, &hints);
	hints &= ~kQAContext_NoZBuffer; 				// Z buffer is on 
	hints &= ~kQAContext_DeepZ; 					// shallow z
	hints &= ~kQAContext_NoDither; 					// yes-dither
	Q3InteractiveRenderer_SetRAVEContextHints(gQD3D_RendererObject, hints);	
#endif

	// Source port addition: turn off Quesa's angle affect on alpha to preserve the original look of shadows, water, shields etc.
	Q3Object_SetProperty(gQD3D_RendererObject, kQ3RendererPropertyAngleAffectsAlpha,
	                     sizeof(gQD3D_AngleAffectsAlpha), &gQD3D_AngleAffectsAlpha);
	
	Q3InteractiveRenderer_SetRAVETextureFilter(gQD3D_RendererObject,kQATextureFilter_Fast);	// texturing
#if 0 // Source port removal - deprecated by Quesa
	Q3InteractiveRenderer_SetDoubleBufferBypass(gQD3D_RendererObject,kQ3True);
#endif

#endif
}
#endif


/**************** CREATE DRAW CONTEXT *********************/

static void CreateDrawContext(QD3DViewDefType *viewDefPtr)
#if 1	// TODO noquesa
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3DrawContextData		drawContexData;
TQ3SDLDrawContextData	myMacDrawContextData;
extern SDL_Window*		gSDLWindow;
	int ww, wh;
	SDL_GL_GetDrawableSize(gSDLWindow, &ww, &wh);


			/* FILL IN DRAW CONTEXT DATA */

	drawContexData.clearImageMethod = kQ3ClearMethodWithColor;				// how to clear
	drawContexData.clearImageColor = viewDefPtr->clearColor;				// color to clear to
	drawContexData.pane = GetAdjustedPane(ww, wh, viewDefPtr->paneClip);
	
	
	drawContexData.paneState = kQ3True;										// use bounds?
	drawContexData.maskState = kQ3False;									// no mask
	drawContexData.doubleBufferState = kQ3True;								// double buffering

	myMacDrawContextData.drawContextData = drawContexData;					// set MAC specifics
	myMacDrawContextData.sdlWindow = gSDLWindow;							// assign window to draw to


			/* CREATE DRAW CONTEXT */

	gQD3D_DrawContext = Q3SDLDrawContext_New(&myMacDrawContextData);
	if (gQD3D_DrawContext == nil)
		DoFatalAlert("Q3MacDrawContext_New Failed!");


	gQD3D_FreshDrawContext = true;
}
#endif




/**************** SET STYLES ****************/
//
// Creates style objects which define how the scene is to be rendered.
// It also sets the shader object.
//

static void SetStyles(QD3DStyleDefType *styleDefPtr)
#if 1	// TODO noquesa
{ printf("TODO noquesa: %s\n", __func__); }
#else
{

				/* SET INTERPOLATION (FOR SHADING) */
					
	gQD3D_InterpolationStyle = Q3InterpolationStyle_New(styleDefPtr->interpolation);
	if (gQD3D_InterpolationStyle == nil)
		DoFatalAlert("Q3InterpolationStyle_New Failed!");

					/* SET BACKFACING */

	gQD3D_BackfacingStyle = Q3BackfacingStyle_New(styleDefPtr->backfacing);
	if (gQD3D_BackfacingStyle == nil )
		DoFatalAlert("Q3BackfacingStyle_New Failed!");


				/* SET POLYGON FILL STYLE */
						
	gQD3D_FillStyle = Q3FillStyle_New(styleDefPtr->fill);
	if ( gQD3D_FillStyle == nil )
		DoFatalAlert(" Q3FillStyle_New Failed!");


					/* SET THE SHADER TO USE */

	if (styleDefPtr->usePhong)
	{
		gQD3D_ShaderObject = Q3PhongIllumination_New();
		if ( gQD3D_ShaderObject == nil )
			DoFatalAlert(" Q3PhongIllumination_New Failed!");
	}
	else
	{
		gQD3D_ShaderObject = Q3LambertIllumination_New();
		if ( gQD3D_ShaderObject == nil )
			DoFatalAlert(" Q3LambertIllumination_New Failed!");
	}


			/* ALSO MAKE NULL SHADER FOR SPECIAL PURPOSES */
			
	gQD3D_NullShaderObject = Q3NULLIllumination_New();

}
#endif



/****************** CREATE CAMERA *********************/

static void CreateCamera(QD3DSetupInputType *setupDefPtr)
#if 1	// TODO noquesa
{ printf("TODO noquesa: %s\n", __func__); }
#else
{
TQ3CameraData					myCameraData;
TQ3ViewAngleAspectCameraData	myViewAngleCameraData;
TQ3Area							pane;
TQ3Status						status;
TQ3Status	myErr;
QD3DCameraDefType 				*cameraDefPtr;

	cameraDefPtr = &setupDefPtr->camera;

		/* GET PANE */
		//
		// Note: Q3DrawContext_GetPane seems to return garbage on pixmaps so, rig it.
		//
		
	if (setupDefPtr->view.useWindow)
	{
		status = Q3DrawContext_GetPane(gQD3D_DrawContext,&pane);				// get window pane info
		if (status == kQ3Failure)
			DoFatalAlert("Q3DrawContext_GetPane Failed!");
	}
	else
	{
		DoFatalAlert("CreateCamera: offscreen view unsupported");
#if 0
		pane.max.x = setupDefPtr->view.gworld->portRect.right;
		pane.max.y = setupDefPtr->view.gworld->portRect.bottom;
#endif
	}


				/* FILL IN CAMERA DATA */
				
	myCameraData.placement.cameraLocation = cameraDefPtr->from;			// set camera coords
	myCameraData.placement.pointOfInterest = cameraDefPtr->to;			// set target coords
	myCameraData.placement.upVector = cameraDefPtr->up;					// set a vector that's "up"
	myCameraData.range.hither = cameraDefPtr->hither;					// set frontmost Z dist
	myCameraData.range.yon = cameraDefPtr->yon;							// set farthest Z dist
	myCameraData.viewPort.origin.x = -1.0;								// set view origins?
	myCameraData.viewPort.origin.y = 1.0;
	myCameraData.viewPort.width = 2.0;
	myCameraData.viewPort.height = 2.0;

	myViewAngleCameraData.cameraData = myCameraData;
	myViewAngleCameraData.fov = cameraDefPtr->fov;						// larger = more fisheyed
	myViewAngleCameraData.aspectRatioXToY =
				(pane.max.x-pane.min.x)/(pane.max.y-pane.min.y);

	gQD3D_CameraObject = Q3ViewAngleAspectCamera_New(&myViewAngleCameraData);	 // create new camera
	if (gQD3D_CameraObject == nil)
		DoFatalAlert("Q3ViewAngleAspectCamera_New failed!");
		
	myErr = Q3View_SetCamera(gQD3D_ViewObject, gQD3D_CameraObject);		// assign camera to view
	if (myErr == kQ3Failure)
		DoFatalAlert("Q3View_SetCamera Failed!");
}
#endif


/********************* CREATE LIGHTS ************************/

static void CreateLights(QD3DLightDefType *lightDefPtr)
{
#if 0	// NOQUESA
TQ3GroupPosition		myGroupPosition;
TQ3LightData			myLightData;
TQ3DirectionalLightData	myDirectionalLightData;
TQ3LightObject			myLight;
short					i;
TQ3Status	myErr;


			/* CREATE NEW LIGHT GROUP */
			
	gQD3D_LightGroup = Q3LightGroup_New();						// make new light group
	if ( gQD3D_LightGroup == nil )
		DoFatalAlert(" Q3LightGroup_New Failed!");


	myLightData.isOn = kQ3True;									// light is ON
#endif
	glEnable(GL_LIGHTING);

			/************************/
			/* CREATE AMBIENT LIGHT */
			/************************/

	if (lightDefPtr->ambientBrightness != 0)						// see if ambient exists
	{
#if 1	// NOQUESA
		GLfloat ambient[4] =
		{
			lightDefPtr->ambientBrightness * lightDefPtr->ambientColor.r,
			lightDefPtr->ambientBrightness * lightDefPtr->ambientColor.g,
			lightDefPtr->ambientBrightness * lightDefPtr->ambientColor.b,
			1
		};
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
#else
		myLightData.color = lightDefPtr->ambientColor;				// set color of light
		myLightData.brightness = lightDefPtr->ambientBrightness;	// set brightness value
		myLight = Q3AmbientLight_New(&myLightData);					// make it
		if ( myLight == nil )
			DoFatalAlert("Q3AmbientLight_New Failed!");

		myGroupPosition = Q3Group_AddObject(gQD3D_LightGroup, myLight);	// add to group
		if ( myGroupPosition == 0 )
			DoFatalAlert(" Q3Group_AddObject Failed!");

		Q3Object_Dispose(myLight);									// dispose of light
#endif
	}

			/**********************/
			/* CREATE FILL LIGHTS */
			/**********************/
			
	for (int i = 0; i < lightDefPtr->numFillLights; i++)
	{
#if 1	// NOQUESA
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

		diffuse[0] = lightDefPtr->fillColor[i].r;
		diffuse[1] = lightDefPtr->fillColor[i].g;
		diffuse[2] = lightDefPtr->fillColor[i].b;
		diffuse[3] = 1;

		glLightfv(GL_LIGHT0+i, GL_DIFFUSE, diffuse);


		glEnable(GL_LIGHT0+i);								// enable the light
#else
		myLightData.color = lightDefPtr->fillColor[i];						// set color of light
		myLightData.brightness = lightDefPtr->fillBrightness[i];			// set brightness
		myDirectionalLightData.lightData = myLightData;						// refer to general light info
		myDirectionalLightData.castsShadows = kQ3True;						// shadows
		myDirectionalLightData.direction =  lightDefPtr->fillDirection[i];	// set fill vector
		myLight = Q3DirectionalLight_New(&myDirectionalLightData);			// make it
		if ( myLight == nil )
			DoFatalAlert(" Q3DirectionalLight_New Failed!");

		myGroupPosition = Q3Group_AddObject(gQD3D_LightGroup, myLight);		// add to group
		if ( myGroupPosition == 0 )
			DoFatalAlert(" Q3Group_AddObject Failed!");

		Q3Object_Dispose(myLight);											// dispose of light
#endif
	}

#if 0	// NOQUESA
			/* ASSIGN LIGHT GROUP TO VIEW */
			
	myErr = Q3View_SetLightGroup(gQD3D_ViewObject, gQD3D_LightGroup);		// assign light group to view
	if (myErr == kQ3Failure)
		DoFatalAlert("Q3View_SetLightGroup Failed!");		
#endif
}





/******************* QD3D DRAW SCENE *********************/

void QD3D_DrawScene(QD3DSetupOutputType *setupInfo, void (*drawRoutine)(QD3DSetupOutputType *))
{
#if 0	// NOQUESA
TQ3Status				myStatus;
TQ3ViewStatus			myViewStatus;
#endif

	GAME_ASSERT(setupInfo);
	GAME_ASSERT(setupInfo->isActive);							// make sure it's legit


			/* START RENDERING */

#if 1	// NOQUESA
	int mkc = SDL_GL_MakeCurrent(gSDLWindow, gGLContext);
	GAME_ASSERT_MESSAGE(mkc == 0, SDL_GetError());

	int windowWidth, windowHeight;
	SDL_GetWindowSize(gSDLWindow, &windowWidth, &windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);


	glClearColor(0, 0.5f, 0.5f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#else
	myStatus = Q3View_StartRendering(setupInfo->viewObject);
	if ( myStatus == kQ3Failure )
	{
		DoAlert("ERROR: Q3View_StartRendering Failed!");
		QD3D_ShowRecentError();
	}
#endif


	if (gQD3D_FreshDrawContext)
	{
		SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);

		AllocBackdropTexture(); // Source port addition - alloc GL backdrop texture
		// (must be after StartRendering so we have a valid GL context)

		gQD3D_FreshDrawContext = false;
	}



#if 1	// NOQUESA
	printf("TODO noquesa: %s: update frustum planes\n", __func__);
#else
			/* PREPARE FRUSTUM PLANES FOR SPHERE VISIBILITY CHECKS */
			// (Source port addition)

	UpdateFrustumPlanes(setupInfo->viewObject);
#endif


			/***************/
			/* RENDER LOOP */
			/***************/

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_CULL_FACE);

#if 1	// NOQUESA
	if (drawRoutine)
		drawRoutine(setupInfo);

	SDL_GL_SwapWindow(gSDLWindow);
#else
	do
	{
				/* DRAW STYLES */
				
		// Source port addition
		myStatus = Q3FogStyle_Submit(&gQD3D_FogStyleData, setupInfo->viewObject);
		if ( myStatus == kQ3Failure )
			DoFatalAlert(" Q3FogStyle_Submit Failed!");
		
		myStatus = Q3Style_Submit(setupInfo->interpolationStyle,setupInfo->viewObject);
		if ( myStatus == kQ3Failure )
			DoFatalAlert(" Q3Style_Submit Failed!");
			
		myStatus = Q3Style_Submit(setupInfo->backfacingStyle,setupInfo->viewObject);
		if ( myStatus == kQ3Failure )
			DoFatalAlert(" Q3Style_Submit Failed!");
			
		myStatus = Q3Style_Submit(setupInfo->fillStyle, setupInfo->viewObject);
		if ( myStatus == kQ3Failure )
			DoFatalAlert(" Q3Style_Submit Failed!");

		myStatus = Q3Shader_Submit(setupInfo->shaderObject, setupInfo->viewObject);
		if ( myStatus == kQ3Failure )
			DoFatalAlert(" Q3Shader_Submit Failed!");


			/* CALL INPUT DRAW FUNCTION */

		if (drawRoutine != nil)
			drawRoutine(setupInfo);

		myViewStatus = Q3View_EndRendering(setupInfo->viewObject);
		if ( myViewStatus == kQ3ViewStatusError)
			DoFatalAlert("QD3D_DrawScene: Q3View_EndRendering failed!");
		else if (myViewStatus == kQ3ViewStatusRetraverse)
			DoFatalAlert("QD3D_DrawScene: we need to retraverse!");
		
	} while ( myViewStatus == kQ3ViewStatusRetraverse );
#endif
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
//=============================== LIGHTS STUFF ==========================================================
//=======================================================================================================

#pragma mark ---------- lights -------------


/********************* QD3D ADD POINT LIGHT ************************/

TQ3GroupPosition QD3D_AddPointLight(QD3DSetupOutputType *setupInfo,TQ3Point3D *point, TQ3ColorRGB *color, float brightness)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); return nil; }
#else
{
TQ3GroupPosition		myGroupPosition;
TQ3LightData			myLightData;
TQ3PointLightData		myPointLightData;
TQ3LightObject			myLight;


	myLightData.isOn = kQ3True;											// light is ON
	
	myLightData.color = *color;											// set color of light
	myLightData.brightness = brightness;								// set brightness
	myPointLightData.lightData = myLightData;							// refer to general light info
	myPointLightData.castsShadows = kQ3False;							// no shadows
	myPointLightData.location = *point;									// set coords
	
	myPointLightData.attenuation = kQ3AttenuationTypeNone;// kQ3AttenuationTypeInverseDistance;	// set attenuation
	myLight = Q3PointLight_New(&myPointLightData);				// make it
	if ( myLight == nil )
		DoFatalAlert(" Q3DirectionalLight_New Failed!");

	myGroupPosition = Q3Group_AddObject(setupInfo->lightGroup, myLight);		// add to light group
	if ( myGroupPosition == 0 )
		DoFatalAlert(" Q3Group_AddObject Failed!");

	Q3Object_Dispose(myLight);											// dispose of light

	return(myGroupPosition);

}
#endif


/****************** QD3D SET POINT LIGHT COORDS ********************/

void QD3D_SetPointLightCoords(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition, TQ3Point3D *point)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3PointLightData	pointLightData;
TQ3LightObject		light;
TQ3Status			status;

	status = Q3Group_GetPositionObject(setupInfo->lightGroup, lightPosition, &light);	// get point light object from light group
	if (status == kQ3Failure)
		DoFatalAlert("Q3Group_GetPositionObject Failed!");


	status =  Q3PointLight_GetData(light, &pointLightData);				// get light data
	if (status == kQ3Failure)
		DoFatalAlert("Q3PointLight_GetData Failed!");

	pointLightData.location = *point;									// set coords

	status = Q3PointLight_SetData(light, &pointLightData);				// update light data
	if (status == kQ3Failure)
		DoFatalAlert("Q3PointLight_SetData Failed!");
		
	Q3Object_Dispose(light);
}
#endif


/****************** QD3D SET POINT LIGHT BRIGHTNESS ********************/

void QD3D_SetPointLightBrightness(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition, float bright)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3LightObject		light;
TQ3Status			status;

	status = Q3Group_GetPositionObject(setupInfo->lightGroup, lightPosition, &light);	// get point light object from light group
	if (status == kQ3Failure)
		DoFatalAlert("Q3Group_GetPositionObject Failed!");

	status = Q3Light_SetBrightness(light, bright);
	if (status == kQ3Failure)
		DoFatalAlert("Q3Light_SetBrightness Failed!");

	Q3Object_Dispose(light);
}
#endif



/********************* QD3D ADD FILL LIGHT ************************/

TQ3GroupPosition QD3D_AddFillLight(QD3DSetupOutputType *setupInfo,TQ3Vector3D *fillVector, TQ3ColorRGB *color, float brightness)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); return nil; }
#else
{
TQ3GroupPosition		myGroupPosition;
TQ3LightData			myLightData;
TQ3LightObject			myLight;
TQ3DirectionalLightData	myDirectionalLightData;


	myLightData.isOn = kQ3True;									// light is ON
	
	myLightData.color = *color;									// set color of light
	myLightData.brightness = brightness;						// set brightness
	myDirectionalLightData.lightData = myLightData;				// refer to general light info
	myDirectionalLightData.castsShadows = kQ3False;				// no shadows
	myDirectionalLightData.direction = *fillVector;				// set vector
	
	myLight = Q3DirectionalLight_New(&myDirectionalLightData);	// make it
	if ( myLight == nil )
		DoFatalAlert(" Q3DirectionalLight_New Failed!");
	
	myGroupPosition = Q3Group_AddObject(setupInfo->lightGroup, myLight);	// add to light group
	if ( myGroupPosition == 0 )
		DoFatalAlert(" Q3Group_AddObject Failed!");

	Q3Object_Dispose(myLight);												// dispose of light
	return(myGroupPosition);
}
#endif

/********************* QD3D ADD AMBIENT LIGHT ************************/

TQ3GroupPosition QD3D_AddAmbientLight(QD3DSetupOutputType *setupInfo, TQ3ColorRGB *color, float brightness)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); return nil; }
#else
{
TQ3GroupPosition		myGroupPosition;
TQ3LightData			myLightData;
TQ3LightObject			myLight;



	myLightData.isOn = kQ3True;									// light is ON
	myLightData.color = *color;									// set color of light
	myLightData.brightness = brightness;						// set brightness
	
	myLight = Q3AmbientLight_New(&myLightData);					// make it
	if ( myLight == nil )
		DoFatalAlert("Q3AmbientLight_New Failed!");

	myGroupPosition = Q3Group_AddObject(setupInfo->lightGroup, myLight);		// add to light group
	if ( myGroupPosition == 0 )
		DoFatalAlert(" Q3Group_AddObject Failed!");

	Q3Object_Dispose(myLight);									// dispose of light
	
	return(myGroupPosition);
}
#endif




/****************** QD3D DELETE LIGHT ********************/

void QD3D_DeleteLight(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3LightObject		light;

	light = Q3Group_RemovePosition(setupInfo->lightGroup, lightPosition);
	if (light == nil)
		DoFatalAlert("Q3Group_RemovePosition Failed!");

	Q3Object_Dispose(light);
}
#endif


/****************** QD3D DELETE ALL LIGHTS ********************/
//
// Deletes ALL lights from the light group, including the ambient light.
//

void QD3D_DeleteAllLights(QD3DSetupOutputType *setupInfo)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3Status				status;

	status = Q3Group_EmptyObjects(setupInfo->lightGroup);
	if (status == kQ3Failure)
		DoFatalAlert("QD3D_DeleteAllLights: Q3Group_EmptyObjects Failed!");

}
#endif




//=======================================================================================================
//=============================== TEXTURE MAP STUFF =====================================================
//=======================================================================================================

#pragma mark ---------- textures -------------

/**************** QD3D GET TEXTURE MAP ***********************/
//
// Loads a PICT resource and returns a shader object which is
// based on the PICT converted to a texture map.
//
// INPUT: textureRezID = resource ID of texture PICT to get.
//			myFSSpec != nil if want to load PICT from file instead
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.  nil == error.
//

TQ3SurfaceShaderObject	QD3D_GetTextureMap(long	textureRezID, FSSpec *myFSSpec)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); return nil; }
#else
{
PicHandle			picture;
TQ3SurfaceShaderObject		shader;
long				pictSize,headerSize;
OSErr				iErr;
short				fRefNum;
char				pictHeader[PICT_HEADER_SIZE];

	if (myFSSpec == nil)
	{
					/* LOAD PICT REZ */
		
		picture = GetPicture (textureRezID);
		if (picture == nil)
			DoFatalAlert("Unable to load texture PICT resource");
	}
	else
	{
				/* LOAD PICT FROM FILE */
	
		iErr = FSpOpenDF(myFSSpec,fsCurPerm,&fRefNum);
		if (iErr)
		{
			DoAlert("Sorry, can open that PICT file!");
			return(nil);
		}

		if	(GetEOF(fRefNum,&pictSize) != noErr)		// get size of file		
			goto err;
				
		headerSize = PICT_HEADER_SIZE;					// check the header					
		if (FSRead(fRefNum,&headerSize,pictHeader) != noErr)
			goto err;

		if ((pictSize -= PICT_HEADER_SIZE) <= 0)
			goto err;
			
		if ((picture = (PicHandle)AllocHandle(pictSize)) == nil)
		{
			DoAlert("Sorry, not enough memory to read PICT file!");
			return(nil);
		}
		HLock((Handle)picture);
			
		if (FSRead(fRefNum,&pictSize,(Ptr)*picture) != noErr)
		{
			DisposeHandle((Handle)picture);
			goto err;
		}
			
		FSClose(fRefNum);		
	}
	
	
	shader = QD3D_PICTToTexture(picture);
		
	if (myFSSpec == nil)
		ReleaseResource ((Handle) picture);
	else
		DisposeHandle((Handle)picture);

	return(shader);	
	
err:
	DoAlert("Sorry, error reading PICT file!");
	return(nil);
}
#endif


/**************** QD3D PICT TO TEXTURE ***********************/
//
//
// INPUT: picture = handle to PICT.
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.
//

TQ3SurfaceShaderObject	QD3D_PICTToTexture(PicHandle picture)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); return nil; }
#else
{
TQ3Mipmap 				mipmap;
TQ3TextureObject		texture;
TQ3SurfaceShaderObject	shader;
long					width,height;


			/* MAKE INTO STORAGE MIPMAP */

	width = (**picture).picFrame.right  - (**picture).picFrame.left;		// calc dimensions of mipmap
	height = (**picture).picFrame.bottom - (**picture).picFrame.top;
		
	DrawPICTIntoMipmap (picture, width, height, &mipmap);
		

			/* MAKE NEW PIXMAP TEXTURE */
			
	texture = Q3MipmapTexture_New(&mipmap);							// make new mipmap	
	if (texture == nil)
		DoFatalAlert("QD3D_PICTToTexture: Q3MipmapTexture_New failed!");
		
	shader = Q3TextureShader_New (texture);
	if (shader == nil)
		DoFatalAlert("Error calling Q3TextureShader_New!");

	Q3Object_Dispose (texture);
	Q3Object_Dispose (mipmap.image);			// disposes of extra reference to storage obj

	return(shader);	
}
#endif


/**************** QD3D GWORLD TO TEXTURE ***********************/
//
// INPUT: picture = handle to PICT.
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.
//

TQ3SurfaceShaderObject	QD3D_GWorldToTexture(GWorldPtr theGWorld, Boolean pointToGWorld)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); return nil; }
#else
{
TQ3Mipmap 					mipmap;
TQ3TextureObject			texture;
TQ3SurfaceShaderObject		shader;

			/* CREATE MIPMAP */
			
	QD3D_GWorldToMipMap(theGWorld,&mipmap, pointToGWorld);


			/* MAKE NEW MIPMAP TEXTURE */
			
	texture = Q3MipmapTexture_New(&mipmap);							// make new mipmap	
	if (texture == nil)
		DoFatalAlert("QD3D_GWorldToTexture: Q3MipmapTexture_New failed!");
			
	shader = Q3TextureShader_New(texture);
	if (shader == nil)
		DoFatalAlert("Error calling Q3TextureShader_New!");

	Q3Object_Dispose (texture);
	Q3Object_Dispose (mipmap.image);					// dispose of extra ref to storage object

	return(shader);	
}
#endif


/******************** DRAW PICT INTO MIPMAP ********************/
//
// OUTPUT: mipmap = new mipmap holding texture image
//

#if 0	// TODO noquesa
static void DrawPICTIntoMipmap(PicHandle pict, long width, long height, TQ3Mipmap* mipmap)
{
#if 1
short					depth;

	depth = 32;
	if (width  != (**pict).picFrame.right  - (**pict).picFrame.left) DoFatalAlert("DrawPICTIntoMipmap: unexpected width");
	if (height != (**pict).picFrame.bottom - (**pict).picFrame.top)  DoFatalAlert("DrawPICTIntoMipmap: unexpected height");
	Ptr pictMapAddr = (**pict).__pomme_pixelsARGB32;
	long pictRowBytes = width * (depth / 8);

	/*
	if (pointToGWorld)
	{
		LockPixels(hPixMap);										// we don't want this to move on us
		mipmap->image = Q3MemoryStorage_NewBuffer((unsigned char*)pictMapAddr, pictRowBytes * height, pictRowBytes * height);
	}
	else
	*/
		mipmap->image = Q3MemoryStorage_New((unsigned char*)pictMapAddr, pictRowBytes * height);
	
	if (mipmap->image == nil)
		DoFatalAlert("Q3MemoryStorage_New Failed!");

	mipmap->useMipmapping = kQ3False;							// not actually using mipmaps (just 1 srcmap)
	if (depth == 16)
		mipmap->pixelType = kQ3PixelTypeRGB16;
	else
		mipmap->pixelType = kQ3PixelTypeARGB32;					// if 32bit, assume alpha

	mipmap->bitOrder = kQ3EndianBig;
	mipmap->byteOrder = kQ3EndianBig;
	mipmap->reserved = 0;
	mipmap->mipmaps[0].width = width;
	mipmap->mipmaps[0].height = height;
	mipmap->mipmaps[0].rowBytes = pictRowBytes;
	mipmap->mipmaps[0].offset = 0;
#else
Rect 					rectGW;
GWorldPtr 				pGWorld;
PixMapHandle 			hPixMap;
OSErr					myErr;
GDHandle				oldGD;
GWorldPtr				oldGW;
long					bytesNeeded;
PictInfo				thePictInfo;
short					depth;

	GetGWorld(&oldGW, &oldGD);										// save current port


				/* GET PICT INFO TO FIND COLOR DEPTH */
				
	myErr = GetPictInfo(pict, &thePictInfo, 0, 0, systemMethod, 0);
	if (myErr)
		depth = 16;
	else
	{
		depth = thePictInfo.depth;
	
		if (thePictInfo.commentHandle)									// free pict info
			DisposeHandle((Handle)thePictInfo.commentHandle);
		if (thePictInfo.fontHandle)
			DisposeHandle((Handle)thePictInfo.fontHandle);
		if (thePictInfo.fontNamesHandle)
			DisposeHandle((Handle)thePictInfo.fontNamesHandle);
		if (thePictInfo.theColorTable)
			DisposeCTable(thePictInfo.theColorTable);
	}


				/* CREATE A GWORLD TO DRAW INTO */

	SetRect(&rectGW, 0, 0, width, height);							// set dimensions
	bytesNeeded = width * height * 2;
	myErr = NewGWorld(&pGWorld, depth, &rectGW, 0, 0, 0L);			// make gworld
	if (myErr)
		DoFatalAlert("DrawPICTIntoMipmap: NewGWorld failed!");
	
	hPixMap = GetGWorldPixMap(pGWorld);								// get gworld's pixmap


			/* DRAW PICTURE INTO GWORLD */
			
	SetGWorld(pGWorld, nil);	
	LockPixels(hPixMap);
	EraseRect(&rectGW);
	DrawPicture(pict, &rectGW);


			/* MAKE A MIPMAP FROM GWORLD */
			
	QD3D_GWorldToMipMap(pGWorld,mipmap,false);
	
	SetGWorld (oldGW, oldGD);
	UnlockPixels (hPixMap);
	DisposeGWorld (pGWorld);
#endif
}
#endif


/******************** GWORLD TO MIPMAP ********************/
//
// Creates a mipmap from an existing GWorld
//
// NOTE: Assumes that GWorld is 16bit!!!!
//
// OUTPUT: mipmap = new mipmap holding texture image
//

#if 0	// TODO noquesa
void QD3D_GWorldToMipMap(GWorldPtr pGWorld, TQ3Mipmap *mipmap, Boolean pointToGWorld)
{
Ptr			 			pictMapAddr;
PixMapHandle 			hPixMap;
unsigned long 			pictRowBytes;
long					width, height;
short					depth;
	
				/* GET GWORLD INFO */
				
	hPixMap = GetGWorldPixMap(pGWorld);								// calc addr & rowbytes
	
	depth = (**hPixMap).pixelSize;									// get gworld depth
	
	pictMapAddr = GetPixBaseAddr(hPixMap);
	width = ((**hPixMap).bounds.right - (**hPixMap).bounds.left);
	height = ((**hPixMap).bounds.bottom - (**hPixMap).bounds.top);
#if 0
	pictRowBytes = (unsigned long)(**hPixMap).rowBytes & 0x3fff;
#else
	pictRowBytes = width * (depth / 8);
#endif

			/* MAKE MIPMAP */

	if (pointToGWorld)	
	{
		LockPixels(hPixMap);										// we don't want this to move on us
		mipmap->image = Q3MemoryStorage_NewBuffer ((unsigned char *) pictMapAddr, pictRowBytes * height,pictRowBytes * height);
	}
	else
		mipmap->image = Q3MemoryStorage_New ((unsigned char *) pictMapAddr, pictRowBytes * height);
		
	if (mipmap->image == nil)
		DoFatalAlert("Q3MemoryStorage_New Failed!");

	mipmap->useMipmapping = kQ3False;							// not actually using mipmaps (just 1 srcmap)
	if (depth == 16)
		mipmap->pixelType = kQ3PixelTypeRGB16;						
	else
		mipmap->pixelType = kQ3PixelTypeARGB32;					// if 32bit, assume alpha
	
	mipmap->bitOrder = kQ3EndianBig;
	mipmap->byteOrder = kQ3EndianBig;
	mipmap->reserved = 0;
	mipmap->mipmaps[0].width = width;
	mipmap->mipmaps[0].height = height;
	mipmap->mipmaps[0].rowBytes = pictRowBytes;
	mipmap->mipmaps[0].offset = 0;
}
#endif



//=======================================================================================================
//=============================== STYLE STUFF =====================================================
//=======================================================================================================


/****************** QD3D:  SET BACKFACE STYLE ***********************/

void SetBackFaceStyle(QD3DSetupOutputType *setupInfo, TQ3BackfacingStyle style)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3Status status;
TQ3BackfacingStyle	backfacingStyle;

	status = Q3BackfacingStyle_Get(setupInfo->backfacingStyle, &backfacingStyle);
	if (status == kQ3Failure)
		DoFatalAlert("Q3BackfacingStyle_Get Failed!");

	if (style == backfacingStyle)							// see if already set to that
		return;
		
	status = Q3BackfacingStyle_Set(setupInfo->backfacingStyle, style);
	if (status == kQ3Failure)
		DoFatalAlert("Q3BackfacingStyle_Set Failed!");

}
#endif


/****************** QD3D:  SET FILL STYLE ***********************/

void SetFillStyle(QD3DSetupOutputType *setupInfo, TQ3FillStyle style)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3Status 		status;
TQ3FillStyle	fillStyle;

	status = Q3FillStyle_Get(setupInfo->fillStyle, &fillStyle);
	if (status == kQ3Failure)
		DoFatalAlert("Q3FillStyle_Get Failed!");

	if (style == fillStyle)							// see if already set to that
		return;
		
	status = Q3FillStyle_Set(setupInfo->fillStyle, style);
	if (status == kQ3Failure)
		DoFatalAlert("Q3FillStyle_Set Failed!");

}
#endif


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


#pragma mark ---------- errors -------------

/************ QD3D: SHOW RECENT ERROR *******************/

void QD3D_ShowRecentError(void)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3Error	q3Err;
Str255		s;
	
	q3Err = Q3Error_Get(nil);
	if (q3Err == kQ3ErrorOutOfMemory)
		QD3D_DoMemoryError();
	else
	if (q3Err == kQ3ErrorMacintoshError)
		DoFatalAlert("kQ3ErrorMacintoshError");
	else
	if (q3Err == kQ3ErrorNotInitialized)
		DoFatalAlert("kQ3ErrorNotInitialized");
	else
	if (q3Err == kQ3ErrorReadLessThanSize)
		DoFatalAlert("kQ3ErrorReadLessThanSize");
	else
	if (q3Err == kQ3ErrorViewNotStarted)
		DoFatalAlert("kQ3ErrorViewNotStarted");
	else
	if (q3Err != 0)
	{
		NumToStringC(q3Err,s);
		DoFatalAlert2("QD3D Error", s);
	}
}
#endif

/***************** QD3D: DO MEMORY ERROR **********************/

void QD3D_DoMemoryError(void)
{
	DoFatalAlert("QD3D Memory Error!");
}











/**************** QD3D: DATA16 TO TEXTURE_NOMIP ***********************/
//
// Converts input data to non mipmapped texture
//
// INPUT: .
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.
//

TQ3SurfaceShaderObject	QD3D_Data16ToTexture_NoMip(Ptr data, short width, short height)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); return nil; }
#else
{
TQ3Mipmap 					mipmap;
TQ3TextureObject			texture;
TQ3SurfaceShaderObject		shader;

			/* CREATE MIPMAP */
			
	Data16ToMipmap(data,width,height,&mipmap);


			/* MAKE NEW MIPMAP TEXTURE */
			
	texture = Q3MipmapTexture_New(&mipmap);							// make new mipmap	
	if (texture == nil)
		DoFatalAlert("QD3D_GWorldToTexture: Q3MipmapTexture_New failed!");
			
	shader = Q3TextureShader_New(texture);
	if (shader == nil)
		DoFatalAlert("Error calling Q3TextureShader_New!");

	Q3Object_Dispose (texture);
	Q3Object_Dispose (mipmap.image);					// dispose of extra ref to storage object

	return(shader);	
}
#endif


/******************** DATA16 TO MIPMAP ********************/
//
// Creates a mipmap from an existing 16bit data buffer
//
// NOTE: Assumes that GWorld is 16bit!!!!
//
// OUTPUT: mipmap = new mipmap holding texture image
//

#if 0	// TODO noquesa
static void Data16ToMipmap(Ptr data, short width, short height, TQ3Mipmap *mipmap)
{
			/* MAKE 16bit MIPMAP */

	mipmap->image = Q3MemoryStorage_New ((unsigned char *) data, width * height * 2);
	if (mipmap->image == nil)
		DoFatalAlert("Data16ToMipmap: Q3MemoryStorage_New Failed!");


	mipmap->useMipmapping = kQ3False;							// not actually using mipmaps (just 1 srcmap)
	mipmap->pixelType = kQ3PixelTypeRGB16;						
	mipmap->bitOrder = kQ3EndianBig;
	mipmap->byteOrder = kQ3EndianBig;
	mipmap->reserved = 0;
	mipmap->mipmaps[0].width = width;
	mipmap->mipmaps[0].height = height;
	mipmap->mipmaps[0].rowBytes = width*2;
	mipmap->mipmaps[0].offset = 0;
}
#endif


/**************** QD3D: GET MIPMAP STORAGE OBJECT FROM ATTRIB **************************/

#if 0	// TODO noquesa
TQ3StorageObject QD3D_GetMipmapStorageObjectFromAttrib(TQ3AttributeSet attribSet)
{
TQ3Status	status;
TQ3SurfaceShaderObject	surfaceShader;
TQ3TextureObject		texture;
TQ3Mipmap 				mipmap;
TQ3StorageObject		storage;

			/* GET SHADER FROM ATTRIB */
			
	status = Q3AttributeSet_Get(attribSet, kQ3AttributeTypeSurfaceShader, &surfaceShader);
	if (status == kQ3Failure)
		DoFatalAlert("QD3D_GetMipmapStorageObjectFromAttrib: Q3AttributeSet_Get failed!");

			/* GET TEXTURE */
			
	status = Q3TextureShader_GetTexture(surfaceShader, &texture);
	if (status == kQ3Failure)
		DoFatalAlert("QD3D_GetMipmapStorageObjectFromAttrib: Q3TextureShader_GetTexture failed!");

			/* GET MIPMAP */
			
	status = Q3MipmapTexture_GetMipmap(texture,&mipmap);
	if (status == kQ3Failure)
		DoFatalAlert("QD3D_GetMipmapStorageObjectFromAttrib: Q3MipmapTexture_GetMipmap failed!");

		/* GET A LEGAL REF TO STORAGE OBJ */
			
	storage = mipmap.image;
	
			/* DISPOSE REFS */
			
	Q3Object_Dispose(surfaceShader);
	Q3Object_Dispose(texture);	
	return(storage);
}
#endif


/*********************** MAKE SHADOW TEXTURE ***************************/
//
// The shadow is acutally just an alpha map, but we load in the PICT resource, then
// convert it to a texture, then convert the texture to an alpha channel with a texture of all black.
//

static void MakeShadowTexture(void)
#if 1
{ printf("TODO noquesa: %s\n", __func__); }
#else
{
TQ3Status	status;
TQ3TextureObject	texture;
TQ3Mipmap			mipmap;
short		width,height,x,y;
UInt32		*buffer,*pixelPtr,pixmapRowbytes,size,sizeRead;

	if (gQD3D_gShadowTexture)
		return;

			/* LOAD IMAGE FROM RESOURCE */
				
	gQD3D_gShadowTexture = QD3D_GetTextureMap(129, nil);			
		
		
		/* GET QD3D PIXMAP DATA INFO */


	status = Q3TextureShader_GetTexture(gQD3D_gShadowTexture, &texture);		// get texture from shader
	if (status != kQ3Success)
		DoFatalAlert("MakeShadowTexture: Q3TextureShader_GetTexture failed!");

	status = Q3MipmapTexture_GetMipmap(texture, &mipmap);						// get texture's mipmap
	if (status != kQ3Success)
		DoFatalAlert("MakeShadowTexture: Q3MipmapTexture_GetMipmap failed!");
		
		
		
	height = mipmap.mipmaps[0].height;
	width = mipmap.mipmaps[0].width;

	status = Q3Storage_GetSize(mipmap.image, &size);						// get size of data
	if (status == kQ3Failure)
		DoFatalAlert("MakeShadowTexture: Q3Storage_GetSize failed!");

	buffer = (UInt32 *)AllocPtr(size);										// alloc buffer for pixel data
	if (buffer == nil)
		DoFatalAlert("MakeShadowTexture: AllocPtr failed!");		

	status = Q3Storage_GetData(mipmap.image, 0, size, (unsigned char *)buffer, &sizeRead);	// get pixel data
	if (status == kQ3Failure)
		DoFatalAlert("MakeShadowTexture: Q3Storage_GetData failed!");

	pixmapRowbytes = mipmap.mipmaps[0].rowBytes/4;
		


			/* REMAP THE ALPHA */
					
	pixelPtr = buffer;
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			// put Blue into Alpha & leave map black
			unsigned char* argb = (unsigned char *)(pixelPtr + x);
			argb[0] = argb[3];
			argb[1] = 0;
			argb[2] = 0;
			argb[3] = 0;
		}
			
		pixelPtr += pixmapRowbytes;
	}
		
		
		/* UPDATE THE MAP */
				
	status = Q3Storage_SetData(mipmap.image, 0, size, (unsigned char *)buffer, &sizeRead);		
	if (status == kQ3Failure)
		DoFatalAlert("MakeShadowTexture: Q3Storage_SetData failed!");

	DisposePtr((Ptr)buffer);
	Q3Object_Dispose(texture);
	Q3Object_Dispose(mipmap.image);												// nuke old image storage
}
#endif

#pragma mark ---------- fog & settings -------------

/********************** SET RAVE FOG ******************************/

// Source port change: this function was originally dependent on RAVE extensions for ATI Rage Pro cards.
void QD3D_SetRaveFog(QD3DSetupOutputType *setupInfo, float fogHither, float fogYon, TQ3ColorARGB *fogColor, TQ3FogMode fogMode)
{
	gQD3D_FogStyleData.state        = gGamePrefs.canDoFog? kQ3On: kQ3Off;
	gQD3D_FogStyleData.mode         = kQ3FogModePlaneBasedLinear;
	gQD3D_FogStyleData.color        = *fogColor;
	gQD3D_FogStyleData.fogStart     = setupInfo->hither + fogHither * (setupInfo->yon - setupInfo->hither);
	gQD3D_FogStyleData.fogEnd       = setupInfo->hither + fogYon    * (setupInfo->yon - setupInfo->hither);
	gQD3D_FogStyleData.density      = 0.5f;  // Ignored for linear fog
}



/************************ SET TEXTURE FILTER **************************/

void QD3D_SetTextureFilter(unsigned long textureMode)
#if 1
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
	if (!gGamePrefs.highQualityTextures)		// see if allow high quality
		return;

#if 1
	Q3InteractiveRenderer_SetRAVETextureFilter(gQD3D_RendererObject, (TQ3TextureFilter)textureMode);
#else
	if (!gRaveDrawContext)
		return;

	QASetInt(gRaveDrawContext, (TQATagInt)kQATag_TextureFilter, textureMode);
#endif
}
#endif



#if 0 // Source port removal - unsupported
/************************ SET TRIANGLE CACHE MODE *****************************/
//
// For ATI driver, sets triangle caching flag for xparent triangles
//

void QD3D_SetTriangleCacheMode(Boolean isOn)
{
	if (!gRaveDrawContext)
		return;

	QASetInt(gRaveDrawContext, (TQATagInt)kATITriCache, isOn);
}	
#endif
				

#if 0 // Source port removal - use Q3Shader_SetUBoundary instead
/************************ SET TEXTURE WRAP MODE ************************/
//
// INPUT: mode = kQAGL_Clamp or kQAGL_Repeat
//

void QD3D_SetTextureWrapMode(int mode)
{
	if (!gRaveDrawContext)
		return;

	QASetInt(gRaveDrawContext, kQATagGL_TextureWrapU, mode);
	QASetInt(gRaveDrawContext, kQATagGL_TextureWrapV, mode);
}
#endif


/************************ SET BLENDING MODE ************************/

#if 0 // Source port removal - not required anymore (was used to toggle between interpolate/premultiply)
void QD3D_SetBlendingMode(int mode)
{
	if (gATIis431)							// only do this for ATI driver version 4.30 or newer
	{
		if (!gRaveDrawContext)
			return;

		QASetInt(gRaveDrawContext,kQATag_Blend, mode);
	}
}
#endif






#pragma mark ---------- source port additions -------------

static TQ3Area GetAdjustedPane(int windowWidth, int windowHeight, Rect paneClip)
{
	TQ3Area pane;

	pane.min.x = paneClip.left;					// set bounds?
	pane.max.x = GAME_VIEW_WIDTH - paneClip.right;
	pane.min.y = paneClip.top;
	pane.max.y = GAME_VIEW_HEIGHT - paneClip.bottom;

	pane.min.x += gAdditionalClipping;						// offset bounds by user clipping
	pane.max.x -= gAdditionalClipping;
	pane.min.y += gAdditionalClipping*.75;
	pane.max.y -= gAdditionalClipping*.75;

	// Source port addition
	pane.min.x *= windowWidth / (float)(GAME_VIEW_WIDTH);					// scale clip pane to window size
	pane.max.x *= windowWidth / (float)(GAME_VIEW_WIDTH);
	pane.min.y *= windowHeight / (float)(GAME_VIEW_HEIGHT);
	pane.max.y *= windowHeight / (float)(GAME_VIEW_HEIGHT);

	return pane;
}

// Called when the game window gets resized.
// Adjusts the clipping pane and camera aspect ratio.
void QD3D_OnWindowResized(int windowWidth, int windowHeight)
#if 1
{ printf("TODO noquesa: %s\n", __func__); }
#else
{
	if (!gGameViewInfoPtr)
		return;

	TQ3Area pane = GetAdjustedPane(windowWidth, windowHeight, gGameViewInfoPtr->paneClip);
	Q3DrawContext_SetPane(gGameViewInfoPtr->drawContext, &pane);

	float aspectRatioXToY = (pane.max.x-pane.min.x)/(pane.max.y-pane.min.y);

	Q3ViewAngleAspectCamera_SetAspectRatio(gGameViewInfoPtr->cameraObject, aspectRatioXToY);
}
#endif



#pragma mark -

void QD3D_GetCurrentViewport(const QD3DSetupOutputType *setupInfo, int *x, int *y, int *w, int *h)
{
	int	t,b,l,r;
	int gameWindowWidth, gameWindowHeight;

	SDL_GetWindowSize(gSDLWindow, &gameWindowWidth, &gameWindowHeight);

	t = setupInfo->paneClip.top;
	b = setupInfo->paneClip.bottom;
	l = setupInfo->paneClip.left;
	r = setupInfo->paneClip.right;

	*x = l;
	*y = t;
	*w = gameWindowWidth-l-r;
	*h = gameWindowHeight-t-b;
}

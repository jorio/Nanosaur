/****************************/
/*   	QD3D SUPPORT.C	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
#include <QD3DGroup.h>
#include <QD3DLight.h>
#include <QD3DTransform.h>
#include <QD3DStorage.h>
#include <QD3DMath.h>
#include <QD3DErrors.h>

#include "globals.h"
#include "misc.h"
#include "qd3d_support.h"
#include "input.h"
#include "windows_nano.h"
#include "camera.h"
#include "3dmath.h"
#include "selfrundemo.h"

extern	WindowPtr			gCoverWindow;
extern	long		gScreenXOffset,gScreenYOffset;
extern	Byte		gDemoMode;
extern	PrefsType	gGamePrefs;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void CreateDrawContext(QD3DViewDefType *viewDefPtr);
static void CreateDrawContext_Pixmap(QD3DViewDefType *viewDefPtr);
static void SetStyles(QD3DStyleDefType *styleDefPtr);
static void CreateCamera(QD3DSetupInputType *setupDefPtr);
static void CreateLights(QD3DLightDefType *lightDefPtr);
static void CreateView(QD3DSetupInputType *setupDefPtr);
static void DrawPICTIntoMipmap(PicHandle pict,long width, long height, TQ3Mipmap *mipmap);
static void PreTransformGeometry_Recurse(TQ3Object obj, Boolean reverseFaces);
static void PreTransformAttribute(TQ3AttributeSet theAttribute);
static void PreTransformMesh(TQ3GeometryObject theMesh);
static void PreTransformTriMesh(TQ3GeometryObject theMesh);
static void ReverseMeshVertexOrder(TQ3GeometryObject theMesh);
static void Data16ToMipmap(Ptr data, short width, short height, TQ3Mipmap *mipmap);
static void MakeShadowTexture(void);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static TQ3CameraObject			gQD3D_CameraObject;
TQ3GroupObject			gQD3D_LightGroup;
static TQ3ViewObject			gQD3D_ViewObject;
static TQ3DrawContextObject		gQD3D_DrawContext;
static TQ3RendererObject		gQD3D_RendererObject;
static TQ3ShaderObject			gQD3D_ShaderObject,gQD3D_NullShaderObject;
static	TQ3StyleObject			gQD3D_BackfacingStyle;
static	TQ3StyleObject			gQD3D_FillStyle;
static	TQ3StyleObject			gQD3D_InterpolationStyle;

TQ3ShaderObject					gQD3D_gShadowTexture = nil;

float	gFramesPerSecond = DEFAULT_FPS;				// this is used to maintain a constant timing velocity as frame rates differ
float	gFramesPerSecondFrac = 1/DEFAULT_FPS;

float	gAdditionalClipping = 0;

TQADrawContext	*gRaveDrawContext = nil;
short			gFogMode;

Boolean			gQD3DInitialized = false;


/******************** QD3D: BOOT ******************************/
//
// NOTE: The QuickDraw3D libraries should be included in the project as a "WEAK LINK" so that I can
// 		get an error if the library can't load.  Otherwise, the Finder reports a useless error to the user.
//

void QD3D_Boot(void)
{
TQ3Status	myStatus;


				/* LET 'ER RIP! */
				
	myStatus = Q3Initialize();
	if ( myStatus == kQ3Failure )
		DoFatalAlert("Q3Initialize returned failure.");	

	gQD3DInitialized = true;


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

void QD3D_NewViewDef(QD3DSetupInputType *viewDef, WindowPtr theWindow)
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

	if (theWindow == nil)
		viewDef->view.useWindow 	=	false;							// assume going to pixmap
	else
		viewDef->view.useWindow 	=	true;							// assume going to window
	viewDef->view.displayWindow 	= theWindow;
	viewDef->view.rendererType 		= kQ3RendererTypeInteractive;
	viewDef->view.clearColor 		= clearColor;
	viewDef->view.paneClip.left 	= 0;
	viewDef->view.paneClip.right 	= 0;
	viewDef->view.paneClip.top 		= 0;
	viewDef->view.paneClip.bottom 	= 0;

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
	viewDef->lights.fogMode = kQATIFogLinear;

}

/************** SETUP QD3D WINDOW *******************/

void QD3D_SetupWindow(QD3DSetupInputType *setupDefPtr, QD3DSetupOutputType **outputHandle)
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
				
	status = Q3Object_Dispose(gQD3D_RendererObject);				// (is contained w/in gQD3D_ViewObject)
	if (status == kQ3Failure)
		DoFatalAlert("QD3D_SetupWindow: Q3Object_Dispose failed!");
	

	
				/* PASS BACK INFO */
				
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
	outputPtr->paneClip = setupDefPtr->view.paneClip;
	outputPtr->hither = setupDefPtr->camera.hither;				// remember hither/yon
	outputPtr->yon = setupDefPtr->camera.yon;
	
	outputPtr->isActive = true;							// it's now an active structure
	
	QD3D_MoveCameraFromTo(outputPtr,&v,&v);				// call this to set outputPtr->currentCameraCoords & camera matrix
	
	
			/* FOG */
			
	if (setupDefPtr->lights.useFog)
	{
		QD3D_SetRaveFog(outputPtr,setupDefPtr->lights.fogHither,setupDefPtr->lights.fogYon,
						&setupDefPtr->view.clearColor,setupDefPtr->lights.fogMode);
	}
}


/***************** QD3D_DisposeWindowSetup ***********************/
//
// Disposes of all data created by QD3D_SetupWindow
//

void QD3D_DisposeWindowSetup(QD3DSetupOutputType **dataHandle)
{
QD3DSetupOutputType	*data;

	gRaveDrawContext = nil;											// this is no longer valid

	data = *dataHandle;
	if (data == nil)												// see if this setup exists
		DoFatalAlert("QD3D_DisposeWindowSetup: data == nil");

	Q3Object_Dispose(data->viewObject);
	Q3Object_Dispose(data->interpolationStyle);
	Q3Object_Dispose(data->backfacingStyle);
	Q3Object_Dispose(data->fillStyle);
	Q3Object_Dispose(data->cameraObject);
	Q3Object_Dispose(data->lightGroup);
	Q3Object_Dispose(data->drawContext);
	Q3Object_Dispose(data->shaderObject);
	Q3Object_Dispose(data->nullShaderObject);
		
	data->isActive = false;									// now inactive
	
		/* FREE MEMORY & NIL POINTER */
		
	DisposePtr((Ptr)data);
	*dataHandle = nil;
}


/******************* CREATE GAME VIEW *************************/

static void CreateView(QD3DSetupInputType *setupDefPtr)
{
TQ3Status	status;
TQ3Uns32	hints;

				/* CREATE NEW VIEW OBJECT */
				
	gQD3D_ViewObject = Q3View_New();
	if (gQD3D_ViewObject == nil)
		DoFatalAlert("Q3View_New failed!");


			/* CREATE & SET DRAW CONTEXT */
	
	CreateDrawContext(&setupDefPtr->view); 											// init draw context
	
	status = Q3View_SetDrawContext(gQD3D_ViewObject, gQD3D_DrawContext);			// assign context to view
	if (status == kQ3Failure)
		DoFatalAlert("Q3View_SetDrawContext Failed!");



			/* CREATE & SET RENDERER */


	gQD3D_RendererObject = Q3Renderer_NewFromType(setupDefPtr->view.rendererType);	// create new RENDERER object
	if (gQD3D_RendererObject == nil)
	{
		DoFatalAlert("Q3Renderer_NewFromType Failed!");
	}

	status = Q3InteractiveRenderer_SetPreferences(gQD3D_RendererObject, kQAVendor_BestChoice, kQAEngine_AppleHW);
	if (status == kQ3Failure)
		DoFatalAlert("Q3InteractiveRenderer_SetPreferences Failed!");
	
	status = Q3View_SetRenderer(gQD3D_ViewObject, gQD3D_RendererObject);				// assign renderer to view
	if (status == kQ3Failure)
		DoFatalAlert("Q3View_SetRenderer Failed!");
		
		
		/* SET RENDERER FEATURES */
		
	Q3InteractiveRenderer_GetRAVEContextHints(gQD3D_RendererObject, &hints);
	hints &= ~kQAContext_NoZBuffer; 				// Z buffer is on 
	hints &= ~kQAContext_DeepZ; 					// shallow z
	hints &= ~kQAContext_NoDither; 					// yes-dither
	Q3InteractiveRenderer_SetRAVEContextHints(gQD3D_RendererObject, hints);	
	
	Q3InteractiveRenderer_SetRAVETextureFilter(gQD3D_RendererObject,kQATextureFilter_Fast);	// texturing
	Q3InteractiveRenderer_SetDoubleBufferBypass(gQD3D_RendererObject,kQ3True);
}


/**************** CREATE DRAW CONTEXT *********************/

static void CreateDrawContext(QD3DViewDefType *viewDefPtr)
{
TQ3DrawContextData		drawContexData;
TQ3MacDrawContextData	myMacDrawContextData;
Rect					r;

			/* SEE IF DOING PIXMAP CONTEXT */
			
	if (!viewDefPtr->useWindow)
	{
		CreateDrawContext_Pixmap(viewDefPtr);
		return;	
	}

	r = viewDefPtr->displayWindow->portRect;


			/* FILL IN DRAW CONTEXT DATA */

	drawContexData.clearImageMethod = kQ3ClearMethodWithColor;				// how to clear
	drawContexData.clearImageColor = viewDefPtr->clearColor;				// color to clear to
	drawContexData.pane.min.x = viewDefPtr->paneClip.left;					// set bounds?
	drawContexData.pane.max.x = r.right-viewDefPtr->paneClip.right;
	drawContexData.pane.min.y = viewDefPtr->paneClip.top;
	drawContexData.pane.max.y = r.bottom-viewDefPtr->paneClip.bottom;
	
	drawContexData.pane.min.x += gAdditionalClipping;						// offset bounds by user clipping
	drawContexData.pane.max.x -= gAdditionalClipping;
	drawContexData.pane.min.y += gAdditionalClipping*.75;
	drawContexData.pane.max.y -= gAdditionalClipping*.75;
	
	
	
	drawContexData.paneState = kQ3True;										// use bounds?
	drawContexData.maskState = kQ3False;									// no mask
	drawContexData.doubleBufferState = kQ3True;								// double buffering

	myMacDrawContextData.drawContextData = drawContexData;					// set MAC specifics
	myMacDrawContextData.window = (CWindowPtr)viewDefPtr->displayWindow;	// assign window to draw to
	myMacDrawContextData.library = kQ3Mac2DLibraryNone;						// use standard QD libraries (no GX crap!)
	myMacDrawContextData.viewPort = nil;									// (for GX only)
	myMacDrawContextData.grafPort = (CWindowPtr)viewDefPtr->displayWindow;	// assign grafport


			/* CREATE DRAW CONTEXT */

	gQD3D_DrawContext = Q3MacDrawContext_New(&myMacDrawContextData);
	if (gQD3D_DrawContext == nil)
		DoFatalAlert("Q3MacDrawContext_New Failed!");
}


/**************** CREATE DRAW CONTEXT: PIXMAP *********************/

static void CreateDrawContext_Pixmap(QD3DViewDefType *viewDefPtr)
{
TQ3DrawContextData			drawContexData;
TQ3PixmapDrawContextData	myPixDrawContextData;
Rect						r;
TQ3Pixmap					pixmap;
PixMapHandle 			hPixMap;
long					width,height;
GWorldPtr				theGWorld;
long					pixelSize;

		/* GET GWORLD INFO */
		
	theGWorld = viewDefPtr->gworld;
	r = theGWorld->portRect;

	width = r.right-r.left;
	height = r.bottom-r.top;

	hPixMap = GetGWorldPixMap(theGWorld);							// calc addr & rowbytes
	NoPurgePixels(hPixMap);
	LockPixels(hPixMap);
		
	pixelSize = (**hPixMap).pixelSize;

		/* CREATE PIXMAP DATA */
			
	pixmap.image		= GetPixBaseAddr(hPixMap);
	pixmap.width 		= width;
	pixmap.height		= height;
	pixmap.rowBytes 	= (**hPixMap).rowBytes & 0x7fff;
	pixmap.pixelSize 	= pixelSize;
	if (pixelSize == 16)
		pixmap.pixelType	= kQ3PixelTypeRGB16;
	else
		pixmap.pixelType	= kQ3PixelTypeARGB32;
	pixmap.bitOrder		= kQ3EndianBig;
	pixmap.byteOrder	= kQ3EndianBig;


			/* FILL IN DRAW CONTEXT DATA */

	drawContexData.clearImageMethod = kQ3ClearMethodWithColor;				// how to clear
	drawContexData.clearImageColor = viewDefPtr->clearColor;				// color to clear to
	
	drawContexData.paneState = kQ3False;									// use bounds?
	drawContexData.maskState = kQ3False;									// no mask
	drawContexData.doubleBufferState = kQ3False;								// double buffering

	myPixDrawContextData.drawContextData = drawContexData;					// set PIXMAP specifics
	myPixDrawContextData.pixmap = pixmap;


			/* CREATE DRAW CONTEXT */

	gQD3D_DrawContext = Q3PixmapDrawContext_New(&myPixDrawContextData);
	if (gQD3D_DrawContext == nil)
		DoFatalAlert("Q3PixmapDrawContext_New Failed!");
}


/**************** SET STYLES ****************/
//
// Creates style objects which define how the scene is to be rendered.
// It also sets the shader object.
//

static void SetStyles(QD3DStyleDefType *styleDefPtr)
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



/****************** CREATE CAMERA *********************/

static void CreateCamera(QD3DSetupInputType *setupDefPtr)
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
		pane.min.x = pane.min.y = 0;
		pane.max.x = setupDefPtr->view.gworld->portRect.right;
		pane.max.y = setupDefPtr->view.gworld->portRect.bottom;
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


/********************* CREATE LIGHTS ************************/

static void CreateLights(QD3DLightDefType *lightDefPtr)
{
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
	
			/************************/
			/* CREATE AMBIENT LIGHT */
			/************************/

	if (lightDefPtr->ambientBrightness != 0)						// see if ambient exists
	{
		myLightData.color = lightDefPtr->ambientColor;				// set color of light
		myLightData.brightness = lightDefPtr->ambientBrightness;	// set brightness value
		myLight = Q3AmbientLight_New(&myLightData);					// make it
		if ( myLight == nil )
			DoFatalAlert("Q3AmbientLight_New Failed!");

		myGroupPosition = Q3Group_AddObject(gQD3D_LightGroup, myLight);	// add to group
		if ( myGroupPosition == 0 )
			DoFatalAlert(" Q3Group_AddObject Failed!");

		Q3Object_Dispose(myLight);									// dispose of light

	}

			/**********************/
			/* CREATE FILL LIGHTS */
			/**********************/
			
	for (i=0; i < lightDefPtr->numFillLights; i++)
	{		
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
	}
	
			/* ASSIGN LIGHT GROUP TO VIEW */
			
	myErr = Q3View_SetLightGroup(gQD3D_ViewObject, gQD3D_LightGroup);		// assign light group to view
	if (myErr == kQ3Failure)
		DoFatalAlert("Q3View_SetLightGroup Failed!");		

}



/******************** QD3D CHANGE DRAW SIZE *********************/
//
// Changes size of stuff to fit new window size and/or shink factor
//

void QD3D_ChangeDrawSize(QD3DSetupOutputType *setupInfo)
{
Rect			r;
TQ3Area			pane;
TQ3ViewAngleAspectCameraData	cameraData;
TQ3Status		status;

			/* CHANGE DRAW CONTEXT PANE SIZE */
			
	if (setupInfo->window == nil)
		return;
			
	r = setupInfo->window->portRect;														// get size of window
	pane.min.x = setupInfo->paneClip.left;													// set pane size
	pane.max.x = r.right-setupInfo->paneClip.right;
	pane.min.y = setupInfo->paneClip.top;
	pane.max.y = r.bottom-setupInfo->paneClip.bottom;

	status = Q3DrawContext_SetPane(setupInfo->drawContext,&pane);							// update pane in draw context
	if (status == kQ3Failure)
		DoFatalAlert("Q3DrawContext_SetPane Failed!");		

				/* CHANGE CAMERA ASPECT RATIO */
				
	status = Q3ViewAngleAspectCamera_GetData(setupInfo->cameraObject,&cameraData);			// get camera data
	if (status == kQ3Failure)
		DoFatalAlert("Q3ViewAngleAspectCamera_GetData Failed!");		

	
	cameraData.aspectRatioXToY = (pane.max.x-pane.min.x)/(pane.max.y-pane.min.y);			// set new aspect ratio
	status = Q3ViewAngleAspectCamera_SetData(setupInfo->cameraObject,&cameraData);			// set new camera data
	if (status == kQ3Failure)
		DoFatalAlert("Q3ViewAngleAspectCamera_SetData Failed!");		
}


/******************* QD3D DRAW SCENE *********************/

void QD3D_DrawScene(QD3DSetupOutputType *setupInfo, void (*drawRoutine)(QD3DSetupOutputType *))
{
TQ3Status				myStatus;
TQ3ViewStatus			myViewStatus;

	if (setupInfo == nil)
		DoFatalAlert("QD3D_DrawScene setupInfo == nil");

	if (!setupInfo->isActive)									// make sure it's legit
		DoFatalAlert("QD3D_DrawScene isActive == false");


			/* START RENDERING */

			
	myStatus = Q3View_StartRendering(setupInfo->viewObject);			
	if ( myStatus == kQ3Failure )
	{
		DoAlert("ERROR: Q3View_StartRendering Failed!");
		QD3D_ShowRecentError();
	}

			/***************/
			/* RENDER LOOP */
			/***************/
	do
	{
				/* DRAW STYLES */
				
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
		
	} while ( myViewStatus == kQ3ViewStatusRetraverse );	
}


//=======================================================================================================
//=============================== CAMERA STUFF ==========================================================
//=======================================================================================================

#pragma mark ---------- camera -------------

/*************** QD3D_UpdateCameraFromTo ***************/

void QD3D_UpdateCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Point3D *from, TQ3Point3D *to)
{
TQ3Status	status;
TQ3CameraPlacement	placement;
TQ3CameraObject		camera;

			/* GET CURRENT CAMERA INFO */

	camera = setupInfo->cameraObject;
			
	status = Q3Camera_GetPlacement(camera, &placement);
	if (status == kQ3Failure)
		DoFatalAlert("Q3Camera_GetPlacement failed!");


			/* SET CAMERA LOOK AT */
			
	placement.pointOfInterest = *to;
	setupInfo->currentCameraLookAt = *to;


			/* SET CAMERA COORDS */
			
	placement.cameraLocation = *from;
	setupInfo->currentCameraCoords = *from;				// keep global copy for quick use


			/* UPDATE CAMERA INFO */
			
	status = Q3Camera_SetPlacement(camera, &placement);
	if (status == kQ3Failure)
		DoFatalAlert("Q3Camera_SetPlacement failed!");

	CalcCameraMatrixInfo(setupInfo);	
}


/*************** QD3D_UpdateCameraFrom ***************/

void QD3D_UpdateCameraFrom(QD3DSetupOutputType *setupInfo, TQ3Point3D *from)
{
TQ3Status	status;
TQ3CameraPlacement	placement;

			/* GET CURRENT CAMERA INFO */
			
	status = Q3Camera_GetPlacement(setupInfo->cameraObject, &placement);
	if (status == kQ3Failure)
		DoFatalAlert("Q3Camera_GetPlacement failed!");


			/* SET CAMERA COORDS */
			
	placement.cameraLocation = *from;
	setupInfo->currentCameraCoords = *from;				// keep global copy for quick use
	

			/* UPDATE CAMERA INFO */
			
	status = Q3Camera_SetPlacement(setupInfo->cameraObject, &placement);
	if (status == kQ3Failure)
		DoFatalAlert("Q3Camera_SetPlacement failed!");

	CalcCameraMatrixInfo(setupInfo);	
}


/*************** QD3D_MoveCameraFromTo ***************/

void QD3D_MoveCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Vector3D *moveVector, TQ3Vector3D *lookAtVector)
{
TQ3Status	status;
TQ3CameraPlacement	placement;

			/* GET CURRENT CAMERA INFO */
			
	status = Q3Camera_GetPlacement(setupInfo->cameraObject, &placement);
	if (status == kQ3Failure)
		DoFatalAlert("Q3Camera_GetPlacement failed!");


			/* SET CAMERA COORDS */
			

	placement.cameraLocation.x += moveVector->x;
	placement.cameraLocation.y += moveVector->y;
	placement.cameraLocation.z += moveVector->z;

	placement.pointOfInterest.x += lookAtVector->x;
	placement.pointOfInterest.y += lookAtVector->y;
	placement.pointOfInterest.z += lookAtVector->z;
	
	setupInfo->currentCameraCoords = placement.cameraLocation;	// keep global copy for quick use
	setupInfo->currentCameraLookAt = placement.pointOfInterest;

			/* UPDATE CAMERA INFO */
			
	status = Q3Camera_SetPlacement(setupInfo->cameraObject, &placement);
	if (status == kQ3Failure)
		DoFatalAlert("Q3Camera_SetPlacement failed!");
		
	CalcCameraMatrixInfo(setupInfo);	
}




//=======================================================================================================
//=============================== LIGHTS STUFF ==========================================================
//=======================================================================================================

#pragma mark ---------- lights -------------


/********************* QD3D ADD POINT LIGHT ************************/

TQ3GroupPosition QD3D_AddPointLight(QD3DSetupOutputType *setupInfo,TQ3Point3D *point, TQ3ColorRGB *color, float brightness)
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


/****************** QD3D SET POINT LIGHT COORDS ********************/

void QD3D_SetPointLightCoords(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition, TQ3Point3D *point)
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


/****************** QD3D SET POINT LIGHT BRIGHTNESS ********************/

void QD3D_SetPointLightBrightness(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition, float bright)
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



/********************* QD3D ADD FILL LIGHT ************************/

TQ3GroupPosition QD3D_AddFillLight(QD3DSetupOutputType *setupInfo,TQ3Vector3D *fillVector, TQ3ColorRGB *color, float brightness)
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

/********************* QD3D ADD AMBIENT LIGHT ************************/

TQ3GroupPosition QD3D_AddAmbientLight(QD3DSetupOutputType *setupInfo, TQ3ColorRGB *color, float brightness)
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




/****************** QD3D DELETE LIGHT ********************/

void QD3D_DeleteLight(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition)
{
TQ3LightObject		light;

	light = Q3Group_RemovePosition(setupInfo->lightGroup, lightPosition);
	if (light == nil)
		DoFatalAlert("Q3Group_RemovePosition Failed!");

	Q3Object_Dispose(light);
}


/****************** QD3D DELETE ALL LIGHTS ********************/
//
// Deletes ALL lights from the light group, including the ambient light.
//

void QD3D_DeleteAllLights(QD3DSetupOutputType *setupInfo)
{
TQ3Status				status;

	status = Q3Group_EmptyObjects(setupInfo->lightGroup);
	if (status == kQ3Failure)
		DoFatalAlert("QD3D_DeleteAllLights: Q3Group_EmptyObjects Failed!");

}




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
			
		if (FSRead(fRefNum,&pictSize,*picture) != noErr)
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


/**************** QD3D PICT TO TEXTURE ***********************/
//
//
// INPUT: picture = handle to PICT.
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.
//

TQ3SurfaceShaderObject	QD3D_PICTToTexture(PicHandle picture)
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


/**************** QD3D GWORLD TO TEXTURE ***********************/
//
// INPUT: picture = handle to PICT.
//
// OUTPUT: TQ3ShaderObject = shader object for texture map.
//

TQ3SurfaceShaderObject	QD3D_GWorldToTexture(GWorldPtr theGWorld, Boolean pointToGWorld)
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


/******************** DRAW PICT INTO MIPMAP ********************/
//
// OUTPUT: mipmap = new mipmap holding texture image
//

static void DrawPICTIntoMipmap(PicHandle pict,long width, long height, TQ3Mipmap *mipmap)
{
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
}


/******************** GWORLD TO MIPMAP ********************/
//
// Creates a mipmap from an existing GWorld
//
// NOTE: Assumes that GWorld is 16bit!!!!
//
// OUTPUT: mipmap = new mipmap holding texture image
//

void QD3D_GWorldToMipMap(GWorldPtr pGWorld, TQ3Mipmap *mipmap, Boolean pointToGWorld)
{
unsigned long 			pictMapAddr;
PixMapHandle 			hPixMap;
unsigned long 			pictRowBytes;
long					width, height;
short					depth;
	
				/* GET GWORLD INFO */
				
	hPixMap = GetGWorldPixMap(pGWorld);								// calc addr & rowbytes
	
	depth = (**hPixMap).pixelSize;									// get gworld depth
	
	pictMapAddr = (unsigned long )GetPixBaseAddr(hPixMap);
	pictRowBytes = (unsigned long)(**hPixMap).rowBytes & 0x3fff;
	width = ((**hPixMap).bounds.right - (**hPixMap).bounds.left);
	height = ((**hPixMap).bounds.bottom - (**hPixMap).bounds.top);

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
	mipmap->reserved = nil;
	mipmap->mipmaps[0].width = width;
	mipmap->mipmaps[0].height = height;
	mipmap->mipmaps[0].rowBytes = pictRowBytes;
	mipmap->mipmaps[0].offset = 0;
}



//=======================================================================================================
//=============================== STYLE STUFF =====================================================
//=======================================================================================================


/****************** QD3D:  SET BACKFACE STYLE ***********************/

void SetBackFaceStyle(QD3DSetupOutputType *setupInfo, TQ3BackfacingStyle style)
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


/****************** QD3D:  SET FILL STYLE ***********************/

void SetFillStyle(QD3DSetupOutputType *setupInfo, TQ3FillStyle style)
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


//=======================================================================================================
//=============================== MISC ==================================================================
//=======================================================================================================

/************** QD3D CALC FRAMES PER SECOND *****************/

void	QD3D_CalcFramesPerSecond(void)
{
UnsignedWide	wide;
unsigned long	now;
static	unsigned long then = 0;
Str255			s;

			/* HANDLE SPECIAL DEMO MODE STUFF */
			
	if (gDemoMode)
	{
		gFramesPerSecond = DEMO_FPS;
		gFramesPerSecondFrac = 1.0f/gFramesPerSecond;
	
		do											// speed limiter
		{	
			Microseconds(&wide);
		}while((wide.lo - then) < (1000000.0f / 25.0f));
	}


			/* DO REGULAR CALCULATION */
			
	Microseconds(&wide);
	now = wide.lo;
	if (then != 0)
	{
		gFramesPerSecond = 1000000.0f/(float)(now-then);
		if (gFramesPerSecond < DEFAULT_FPS)			// (avoid divide by 0's later)
			gFramesPerSecond = DEFAULT_FPS;
	
#if 1
		if (GetKeyState_Real(KEY_F8))
		{
			RGBColor	color;
				
			SetPort(gCoverWindow);
			GetForeColor(&color);
			FloatToString(gFramesPerSecond,s);		// print # rounded up to nearest integer
			MoveTo(20,20);
			ForeColor(greenColor);
			TextSize(12);
			TextMode(srcCopy);
			DrawString(s);
			DrawChar(' ');
			RGBForeColor(&color);
		}
#endif
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
		NumToString(q3Err,s);
		DoFatalAlert(s);
	}
}

/***************** QD3D: DO MEMORY ERROR **********************/

void QD3D_DoMemoryError(void)
{
	InitCursor();
	NoteAlert(129,nil);
	CleanQuit();
}


/**************** QD3D: COLOR TO QUICKDRAW COLOR ******************/
//
// Converts QD3D colors to RGBColors.
//

void QD3D_ColorToQDColor(TQ3ColorRGB *in, RGBColor *out)
{
	out->red = in->r * 0xffff;
	out->green = in->g * 0xffff;
	out->blue = in->b * 0xffff;



}


/**************** QD3D: QD COLOR TO QD3D COLOR ******************/
//
// Converts QD3D colors to RGBColors.
//

void QD3D_QDColorToColor(RGBColor *in, TQ3ColorRGB *out)
{
	out->r = (float)in->red / (float)0xffff;
	out->g = (float)in->green / (float)0xffff;
	out->b = (float)in->blue / (float)0xffff;

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


/******************** DATA16 TO MIPMAP ********************/
//
// Creates a mipmap from an existing 16bit data buffer
//
// NOTE: Assumes that GWorld is 16bit!!!!
//
// OUTPUT: mipmap = new mipmap holding texture image
//

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
	mipmap->reserved = nil;
	mipmap->mipmaps[0].width = width;
	mipmap->mipmaps[0].height = height;
	mipmap->mipmaps[0].rowBytes = width*2;
	mipmap->mipmaps[0].offset = 0;
}


/**************** QD3D: GET MIPMAP STORAGE OBJECT FROM ATTRIB **************************/

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


/*********************** MAKE SHADOW TEXTURE ***************************/
//
// The shadow is acutally just an alpha map, but we load in the PICT resource, then
// convert it to a texture, then convert the texture to an alpha channel with a texture of all black.
//

static void MakeShadowTexture(void)
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
			pixelPtr[x] = ((pixelPtr[x]&0xff) << 24);			// put Blue into Alpha & leave map black
			
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

#pragma mark ---------- fog & settings -------------

/********************** SET RAVE FOG ******************************/

void QD3D_SetRaveFog(QD3DSetupOutputType *setupInfo, float fogHither, float fogYon, TQ3ColorARGB *fogColor, short fogMode)
{
#if 1
	TODO();
#else
OSErr		iErr;

				/********************/
				/* GET RAVE CONTEXT */
				/********************/
			
	gRaveDrawContext = nil;
	if (gATI)
	{	
		long response = (long)&gATIRaveDevice;			// must pass this in this way for Chris

		Q3View_StartRendering(setupInfo->viewObject);				// must do this during render state		
			
		if ((iErr = QAEngineGestalt(gATIRaveEngine, kQATIGestalt_CurrentContext, &response)) == kQANoErr)
		{
			if ((response != nil) && (response != (long)&gATIRaveDevice))
			{
				gRaveDrawContext = (TQADrawContext *)response;
		
						/* SET FOG PARAMETERS */
						
				if (gGamePrefs.canDoFog)		// see if allow fog
				{
					gFogMode = fogMode;
					QASetInt(gRaveDrawContext, (TQATagInt)kATIFogMode, fogMode);
					QASetFloat(gRaveDrawContext, (TQATagFloat)kATIFogColor_r, fogColor->r );
					QASetFloat(gRaveDrawContext, (TQATagFloat)kATIFogColor_g, fogColor->g );
					QASetFloat(gRaveDrawContext, (TQATagFloat)kATIFogColor_b, fogColor->b);

					QASetFloat(gRaveDrawContext, (TQATagFloat)kATIFogStart, fogHither );
					QASetFloat(gRaveDrawContext, (TQATagFloat)kATIFogEnd, 1);

					QASetFloat(gRaveDrawContext, (TQATagFloat)kATIFogDensity, fogYon);
				}				
							/* ALSO SET TEXTURE COMPRESSION */
							
				QASetInt(gRaveDrawContext, (TQATagInt)kATITexCompress, kQATexture_NoCompression);


								/* SET TEXTURE FILTER */
								
				QASetInt(gRaveDrawContext, (TQATagInt)kQATag_TextureFilter, kQATextureFilter_Fast);
				
//				QD3D_SetBlendingMode(kQABlend_Interpolate);		//-------------
				
//				QASetInt(gRaveDrawContext, (TQATagInt)kQATag_Blend, kQABlend_OpenGL);
//				QASetInt(gRaveDrawContext, (TQATagInt)kQATagGL_BlendDst,GL_ONE);
//				QASetInt(gRaveDrawContext, (TQATagInt)kQATagGL_BlendSrc,GL_ONE);
			}
		}
		else
		{
			if (!gNotGoodATI)
			{
				gNotGoodATI = true;
				DoAlert("WARNING: This version of Nanosaur needs an ATI Rage Pro card with the latest beta drivers for all of the features to work!");
				HideCursor();
			}
		}
		
		Q3View_Cancel(setupInfo->viewObject);		// stop rendering state
		Q3View_EndRendering(setupInfo->viewObject);		
	}	
#endif
}



/******************************* DISABLE FOG *********************************/

void QD3D_DisableFog(void)
{
	if (!gRaveDrawContext)
		return;

	QASetInt(gRaveDrawContext, (TQATagInt)kATIFogMode, kQATIFogDisable);
}


/******************************* REENABLE FOG *********************************/

void QD3D_ReEnableFog(void)
{
	if (!gRaveDrawContext)
		return;
		
	QASetInt(gRaveDrawContext, (TQATagInt)kATIFogMode, gFogMode);
}


/************************ SET TEXTURE FILTER **************************/

void QD3D_SetTextureFilter(unsigned long textureMode)
{
	if (!gGamePrefs.highQualityTextures)		// see if allow high quality
		return;

	if (!gRaveDrawContext)
		return;

	QASetInt(gRaveDrawContext, (TQATagInt)kQATag_TextureFilter, textureMode);
}



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


/************************ SET BLENDING MODE ************************/

void QD3D_SetBlendingMode(int mode)
{
	if (gATIis431)							// only do this for ATI driver version 4.30 or newer
	{
		if (!gRaveDrawContext)
			return;

		QASetInt(gRaveDrawContext,kQATag_Blend, mode);
	}
}







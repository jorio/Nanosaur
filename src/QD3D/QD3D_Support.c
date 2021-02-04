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

#include "GamePatches.h" // Source port addition - for backdrop quad

extern	SDL_Window	*gSDLWindow;
extern	long		gScreenXOffset,gScreenYOffset;
extern	PrefsType	gGamePrefs;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	long		gNodesDrawn;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void CreateLights(QD3DLightDefType *lightDefPtr);
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

SDL_GLContext					gGLContext;

TQ3ShaderObject					gQD3D_gShadowTexture = nil;

float	gFramesPerSecond = DEFAULT_FPS;				// this is used to maintain a constant timing velocity as frame rates differ
float	gFramesPerSecondFrac = 1/DEFAULT_FPS;

float	gAdditionalClipping = 0;


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
	outputPtr->hither = setupDefPtr->camera.hither;				// remember hither/yon
	outputPtr->yon = setupDefPtr->camera.yon;
	outputPtr->fov = setupDefPtr->camera.fov;

	outputPtr->cameraPlacement.upVector				= setupDefPtr->camera.up;
	outputPtr->cameraPlacement.pointOfInterest		= setupDefPtr->camera.to;
	outputPtr->cameraPlacement.cameraLocation		= setupDefPtr->camera.from;

	outputPtr->lights = setupDefPtr->lights;

	outputPtr->isActive = true;							// it's now an active structure

	TQ3Point3D v = {0, 0, 0};
	QD3D_MoveCameraFromTo(outputPtr,&v,&v);				// call this to set outputPtr->currentCameraCoords & camera matrix

			/* SET UP OPENGL RENDERER PROPERTIES NOW THAT WE HAVE A CONTEXT */

	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);

	AllocBackdropTexture();

	CreateLights(&setupDefPtr->lights);

	if (setupDefPtr->lights.useFog)
	{
		float camHither = setupDefPtr->camera.hither;
		float camYon	= setupDefPtr->camera.yon;
		float fogHither	= setupDefPtr->lights.fogHither;
		float fogYon	= setupDefPtr->lights.fogYon;
		glEnable(GL_FOG);
		glHint(GL_FOG_HINT,		GL_FASTEST);
		glFogi(GL_FOG_MODE,		GL_LINEAR);
		glFogf(GL_FOG_START,	camHither + fogHither * (camYon - camHither));
		glFogf(GL_FOG_END,		camHither + fogYon    * (camYon - camHither));
		glFogfv(GL_FOG_COLOR,	(float *)&setupDefPtr->view.clearColor);
		//glFogf(GL_FOG_DENSITY,	0.5f);
	}
	else
		glDisable(GL_FOG);

	glAlphaFunc(GL_NOTEQUAL, 0);
	glEnable(GL_ALPHA_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);									// CCW is front face

	glEnable(GL_DEPTH_TEST);

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	glClearColor(setupDefPtr->view.clearColor.r, setupDefPtr->view.clearColor.g, setupDefPtr->view.clearColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CHECK_GL_ERROR();

	SDL_GL_SwapWindow(gSDLWindow);
}


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



/**************** SET STYLES ****************/
//
// Creates style objects which define how the scene is to be rendered.
// It also sets the shader object.
//

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
	GAME_ASSERT(setupInfo);
	GAME_ASSERT(setupInfo->isActive);							// make sure it's legit


			/* START RENDERING */

	int mkc = SDL_GL_MakeCurrent(gSDLWindow, gGLContext);
	GAME_ASSERT_MESSAGE(mkc == 0, SDL_GetError());

	int windowWidth, windowHeight;
	SDL_GetWindowSize(gSDLWindow, &windowWidth, &windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



			/* PREPARE FRUSTUM PLANES FOR SPHERE VISIBILITY CHECKS */
			// (Source port addition)

	UpdateFrustumPlanes();


			/***************/
			/* RENDER LOOP */
			/***************/

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

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

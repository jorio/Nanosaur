/*  NAME:
        Qut.c

    DESCRIPTION:
        Quesa Utility Toolkit.

    COPYRIGHT:
        Copyright (c) 1999-2019, Quesa Developers. All rights reserved.

        For the current release of Quesa, please see:

            <http://www.quesa.org/>
        
        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions
        are met:
        
            o Redistributions of source code must retain the above copyright
              notice, this list of conditions and the following disclaimer.
        
            o Redistributions in binary form must reproduce the above
              copyright notice, this list of conditions and the following
              disclaimer in the documentation and/or other materials provided
              with the distribution.
        
            o Neither the name of Quesa nor the names of its contributors
              may be used to endorse or promote products derived from this
              software without specific prior written permission.
        
        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
        "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
        LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
        A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
        OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
        SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
        TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
        PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
        LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
        NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
        SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    ___________________________________________________________________________
*/
//=============================================================================
//      Include files
//-----------------------------------------------------------------------------
#include "Qut.h"
#include "QutInternal.h"







//=============================================================================
//      Constants
//-----------------------------------------------------------------------------
#define kFPSUpdateTime                          2.0
#define kFPSUpdateCount                         400





//=============================================================================
//      Globals
//-----------------------------------------------------------------------------
TQ3ViewObject                   gView            = NULL;
void                            *gWindow         = NULL;
float                           gFPS             = 0.0f;
TQ3Boolean                      gWindowCanResize = kQ3True;
TQ3ObjectType                   gRenderers[kRendererMaxNum];

qutFuncAppMenuSelect            gAppMenuSelect     = NULL;
qutFuncAppRenderPre             gFuncAppRenderPre  = NULL;
qutFuncAppRender                gFuncAppRender     = NULL;
qutFuncAppRenderPost            gFuncAppRenderPost = NULL;
qutFuncAppMouseDown             gFuncAppMouseDown  = NULL;
qutFuncAppMouseTrack            gFuncAppMouseTrack = NULL;
qutFuncAppMouseUp	            gFuncAppMouseUp    = NULL;
qutFuncAppIdle	         	    gFuncAppIdle       = NULL;
qutFuncAppRedraw	            gFuncAppRedraw     = NULL;

static TQ3Uns32                 gFrameCount = 0;
static double					gElapsedTime = 0.0;

static TQ3ShaderObject			gShaderIllumination = NULL;
static TQ3FillStyle             gStyleFill;
static TQ3BackfacingStyle       gStyleBackfacing;
static TQ3InterpolationStyle    gStyleInterpolation;
static TQ3OrientationStyle      gStyleOrientation;
static TQ3AntiAliasStyleData    gStyleDataAntiAlias;
static TQ3FogStyleData          gStyleDataFog = { kQ3Off, kQ3FogModeLinear,
												2.0f, 5.0f, 0.5f,
												{1.0f, 1.0f, 1.0f, 1.0f} };
static TQ3SubdivisionStyleData  gStyleDataSubdivision;





//=============================================================================
//      Internal functions.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//      qut_CPU_time : Get CPU time, for use in finding render time.
//-----------------------------------------------------------------------------
static double
qut_CPU_time()
{
#if WIN32
	// I hear that on Windows, clock() is broken in the sense that it reports
	// wall clock time, not CPU time.
	FILETIME	ft[4];
	ULARGE_INTEGER	uu, uk;
	double theTime;
	GetProcessTimes( GetCurrentProcess(), &ft[0], &ft[1], &ft[2], &ft[3] );
	uu.LowPart = ft[3].dwLowDateTime;
	uu.HighPart = ft[3].dwHighDateTime;
	uk.LowPart = ft[2].dwLowDateTime;
	uk.HighPart = ft[2].dwHighDateTime;
	theTime = (uu.QuadPart + uk.QuadPart) / 10000000.0;
	return theTime;
#else
	clock_t		theClocks = clock();
	return theClocks / ((double) CLOCKS_PER_SEC);
#endif
}


//-----------------------------------------------------------------------------
//      qut_create_camera : Create the camera for our view.
//-----------------------------------------------------------------------------
static TQ3CameraObject
qut_create_camera(TQ3DrawContextObject theDrawContext)
{   TQ3Point3D                      cameraFrom  = { 0.0f, 0.0f, 5.0f };
    TQ3Point3D                      cameraTo    = { 0.0f, 0.0f, 0.0f };
    TQ3Vector3D                     cameraUp    = { 0.0f, 1.0f, 0.0f };
    float                           fieldOfView = Q3Math_DegreesToRadians(50.0f);
    float                           hither      =  0.1f;
#if defined(INFINITY) && !defined(TARGET_API_MAC_OS8)
    float                           yon         = INFINITY;
#else
	float                           yon         = 10.0f;
#endif
    float                           rectWidth, rectHeight;
    TQ3ViewAngleAspectCameraData    cameraData;
    TQ3CameraObject                 theCamera;
    TQ3Area                         theArea;



    // Get the size of the image we're rendering
    Q3DrawContext_GetPane(theDrawContext, &theArea);



    // Fill in the camera data
    cameraData.cameraData.placement.cameraLocation  = cameraFrom;
    cameraData.cameraData.placement.pointOfInterest = cameraTo;
    cameraData.cameraData.placement.upVector        = cameraUp;
    cameraData.cameraData.range.hither              = hither;
    cameraData.cameraData.range.yon                 = yon;
    cameraData.cameraData.viewPort.origin.x         = -1.0f;
    cameraData.cameraData.viewPort.origin.y         =  1.0f;
    cameraData.cameraData.viewPort.width            =  2.0f;
    cameraData.cameraData.viewPort.height           =  2.0f;
    cameraData.fov                                  = fieldOfView;

    rectWidth                  = theArea.max.x - theArea.min.x;
    rectHeight                 = theArea.max.y - theArea.min.y;
    cameraData.aspectRatioXToY = (rectWidth / rectHeight);



    // Create the camera object
    theCamera = Q3ViewAngleAspectCamera_New(&cameraData);
    return(theCamera);
}





//=============================================================================
//      qut_create_light : Create a light object, and add it to a view.
//-----------------------------------------------------------------------------
static void
qut_create_light(TQ3ViewObject theView, TQ3ObjectType lightType, void *lightData)
{   TQ3GroupObject      lightGroup;
    TQ3Status           qd3dStatus;
    TQ3LightObject      theLight;



    // Get the light group for the view
    qd3dStatus = Q3View_GetLightGroup(theView, &lightGroup);
    if (qd3dStatus != kQ3Success)
        return;
    
    
    
    // If we don't have a light group yet, create one
    if (lightGroup == NULL)
        {
        lightGroup = Q3LightGroup_New();
        if (lightGroup == NULL)
            return;
        
        Q3View_SetLightGroup(theView, lightGroup);
        }



    // Create the light object
    switch (lightType) {
        case kQ3LightTypeAmbient:
            theLight = Q3AmbientLight_New((TQ3LightData *) lightData);
            break;

        case kQ3LightTypeDirectional:
            theLight = Q3DirectionalLight_New((TQ3DirectionalLightData *) lightData);
            break;

        case kQ3LightTypePoint:
            theLight = Q3PointLight_New((TQ3PointLightData *) lightData);
            break;

        case kQ3LightTypeSpot:
            theLight = Q3SpotLight_New((TQ3SpotLightData *) lightData);
            break;
        
        default:
            theLight = NULL;
            break;
        }



    // Add the light to the light group
    if (theLight != NULL)
        {
        Q3Group_AddObject(lightGroup, theLight);
        Q3Object_Dispose(theLight);
        }

    Q3Object_Dispose(lightGroup);
}





//=============================================================================
//      qut_create_lights : Add the default set of lights to our view.
//-----------------------------------------------------------------------------
static void
qut_create_lights(TQ3ViewObject theView)
{   TQ3Vector3D                 sunDirection = {-1.0f, 0.0f, -1.0f};
    TQ3Vector3D                 eyeDirection = { 0.0f, 0.0f, -1.0f};
    TQ3ColorRGB                 colourWhite  = { 1.0f, 1.0f,  1.0f};
    TQ3LightData                ambientLight;
    TQ3DirectionalLightData     sunLight;
    TQ3DirectionalLightData     eyeLight;
    


    // Set up the ambient light
    ambientLight.isOn       = kQ3True;
    ambientLight.color      = colourWhite;
    ambientLight.brightness = 0.3f;



    // Set up the directional lights
    sunLight.lightData.isOn       = kQ3True;
    sunLight.lightData.color      = colourWhite;
    sunLight.lightData.brightness = 1.0f;
    sunLight.castsShadows         = kQ3True;
    sunLight.direction            = sunDirection;

    eyeLight.lightData.isOn       = kQ3True;
    eyeLight.lightData.color      = colourWhite;
    eyeLight.lightData.brightness = 0.2f;
    eyeLight.castsShadows         = kQ3False;
    eyeLight.direction            = eyeDirection;



    // Add the lights
    qut_create_light(theView, kQ3LightTypeAmbient,     &ambientLight);
    qut_create_light(theView, kQ3LightTypeDirectional, &sunLight);
    qut_create_light(theView, kQ3LightTypeDirectional, &eyeLight);
}





//=============================================================================
//      qut_create_defaults : Create the default state for the view.
//-----------------------------------------------------------------------------
static void
qut_create_defaults(TQ3ViewObject theView)
{


	// Grab the default style state from the view
	Q3View_GetFillStyleState(theView,          &gStyleFill);
	Q3View_GetBackfacingStyleState(theView,    &gStyleBackfacing);
    Q3View_GetOrientationStyleState(theView,   &gStyleOrientation);
    Q3View_GetAntiAliasStyleState(theView,     &gStyleDataAntiAlias);



	// And set up our own defaults
	gShaderIllumination          = Q3PhongIllumination_New();
	gStyleInterpolation          = kQ3InterpolationStyleVertex;
	gStyleDataAntiAlias.state    = kQ3Off;
	gStyleDataFog.state          = kQ3Off;
	gStyleDataSubdivision.method = kQ3SubdivisionMethodConstant;
	gStyleDataSubdivision.c1     = 25.0f;
	gStyleDataSubdivision.c2     = 25.0f;
}





//=============================================================================
//      qut_set_depth_and_stencil_size : Set preferred sizes of depth and stencil.
//-----------------------------------------------------------------------------
// On Windows, it is not possible to set the pixel format of a window more than
// once, so we must ask for a stencil buffer if we ever might want to turn on
// shadows.
static void
qut_set_depth_and_stencil_size( TQ3ViewObject theView, TQ3DrawContextObject inDC )
{
#if !TARGET_API_MAC_OS8
	TQ3Object	theRenderer;
	TQ3Uns32	depthBits = 24;
	TQ3Uns32	stencilBits = 8;
	
	Q3View_GetRenderer( theView, &theRenderer );
	Q3Object_AddElement( theRenderer, kQ3ElementTypeDepthBits, &depthBits );
	Q3Object_Dispose( theRenderer );

	Q3Object_SetProperty( inDC,
			kQ3DrawContextPropertyGLStencilBufferDepth,
			sizeof(stencilBits), &stencilBits );
#endif
}





//=============================================================================
//      Public functions.
//-----------------------------------------------------------------------------
//      Qut_CreateView : Create the view.
//-----------------------------------------------------------------------------
#pragma mark -
void
Qut_CreateView( qutFuncAppCreateView appCreateView, qutFuncAppConfigureView appConfigureView )
{   TQ3DrawContextObject    theDrawContext;
     TQ3CameraObject         theCamera;



    // Create the objects we need for the view
    theDrawContext = Qut_CreateDrawContext();
    theCamera      = qut_create_camera(theDrawContext);



    // Create the view
    if (theDrawContext != NULL && theCamera != NULL)
        {
        // Create the view
        if (appCreateView != NULL)
        	gView = (*appCreateView)();
        else
	        gView = Q3View_New();
	    
        if (gView != NULL)
            {
            // Configure the view
            Q3View_SetDrawContext(gView,    theDrawContext);
            Q3View_SetCamera(gView,         theCamera);
            Q3View_SetRendererByType(gView, kQ3RendererTypeOpenGL);

			qut_set_depth_and_stencil_size( gView, theDrawContext );
            qut_create_lights(gView);
            qut_create_defaults(gView);

            if (appConfigureView != NULL)
                appConfigureView(gView, theDrawContext, theCamera);
            }
        }



    // Clean up
    if (theDrawContext != NULL)
        Q3Object_Dispose(theDrawContext);

    if (theCamera != NULL)
        Q3Object_Dispose(theCamera);
}





//=============================================================================
//      Qut_CalcBounds : Calculate the bounding box of an object.
//-----------------------------------------------------------------------------
void Qut_CalcBounds(TQ3ViewObject theView, TQ3Object theObject, TQ3BoundingBox *theBounds)
{   float           sizeX, sizeY, sizeZ;
    TQ3Status       qd3dStatus;



    // Reset bounding box
	memset(theBounds, 0x00, sizeof(TQ3BoundingBox));
    


    // Submit the object to the view to calculate the bounding box
    qd3dStatus = Q3View_StartBoundingBox(theView, kQ3ComputeBoundsExact);
    if (qd3dStatus == kQ3Success)
        {
        do
            {
            Qut_SubmitDefaultState( theView );
            Q3Object_Submit(theObject, theView);
            }
        while (Q3View_EndBoundingBox(theView, theBounds) == kQ3ViewStatusRetraverse);
        }



    // If we have an empty bounding box, bump it up slightly
    sizeX = theBounds->max.x - theBounds->min.x;
    sizeY = theBounds->max.y - theBounds->min.y;
    sizeZ = theBounds->max.z - theBounds->min.z;

    if (sizeX <= kQ3RealZero && sizeY <= kQ3RealZero && sizeZ <= kQ3RealZero)
        {
        theBounds->max.x += 0.0001f;
        theBounds->max.y += 0.0001f;
        theBounds->max.z += 0.0001f;
            
        theBounds->min.x -= 0.0001f;
        theBounds->min.y -= 0.0001f;
        theBounds->min.z -= 0.0001f;
        }
}





//=============================================================================
//      Qut_CalcBoundingSphere : Calculate the bounding sphere of an object.
//-----------------------------------------------------------------------------
void Qut_CalcBoundingSphere(TQ3ViewObject theView, TQ3Object theObject, TQ3BoundingSphere *theBoundingSphere)
{    TQ3Status       qd3dStatus;



    // Reset bounding sphere
	memset(theBoundingSphere, 0x00, sizeof(TQ3BoundingSphere));
    


    // Submit the object to the view to calculate the bounding sphere
    qd3dStatus = Q3View_StartBoundingSphere(theView, kQ3ComputeBoundsExact);
    if (qd3dStatus == kQ3Success)
        {
        do
            {
            Qut_SubmitDefaultState( theView );
            Q3Object_Submit(theObject, theView);
            }
        while (Q3View_EndBoundingSphere(theView, theBoundingSphere) == kQ3ViewStatusRetraverse);
        }



    // If we have an empty bounding sphere, bump it up slightly
    if (theBoundingSphere->radius <= kQ3RealZero)
        theBoundingSphere->radius = 0.0001f;
}





//=============================================================================
//      Qut_SubmitDefaultState : Submit the default state to a view.
//-----------------------------------------------------------------------------
void
Qut_SubmitDefaultState(TQ3ViewObject theView)
{


	// Submit the state
	//
	// Note that a view contains a default set of styles and so a real application
	// would only submit the styles that it had changed. However our style menu
	// could change any of them, we just submit them all.
	if (gShaderIllumination != NULL)
		Q3Shader_Submit(gShaderIllumination,          theView);
    
    Q3FillStyle_Submit(gStyleFill,                    theView);
    Q3BackfacingStyle_Submit(gStyleBackfacing,        theView);
    Q3InterpolationStyle_Submit(gStyleInterpolation,  theView);
    Q3OrientationStyle_Submit(gStyleOrientation,      theView);
    Q3AntiAliasStyle_Submit(&gStyleDataAntiAlias,     theView);
    Q3FogStyle_Submit(&gStyleDataFog,                 theView);
    Q3SubdivisionStyle_Submit(&gStyleDataSubdivision, theView);
}





//=============================================================================
//      Qut_GetWindow : Get the window.
//-----------------------------------------------------------------------------
void *
Qut_GetWindow(void)
{

    // Return the window
    return(gWindow);
}





//=============================================================================
//      Qut_SetRenderPreFunc : Set the pre-render callback.
//-----------------------------------------------------------------------------
void
Qut_SetRenderPreFunc(qutFuncAppRenderPre appRenderPre)
{

    // Set the callback
    gFuncAppRenderPre = appRenderPre;
}





//=============================================================================
//      Qut_SetRenderFunc : Set the render callback.
//-----------------------------------------------------------------------------
void
Qut_SetRenderFunc(qutFuncAppRender appRender)
{

    // Set the callback
    gFuncAppRender = appRender;
}





//=============================================================================
//      Qut_SetRenderPostFunc : Set the post-render callback.
//-----------------------------------------------------------------------------
void
Qut_SetRenderPostFunc(qutFuncAppRenderPost appRenderPost)
{

    // Set the callback
    gFuncAppRenderPost = appRenderPost;
}





//=============================================================================
//      Qut_SetMouseDownFunc : Set the mouse down callback.
//-----------------------------------------------------------------------------
void
Qut_SetMouseDownFunc(qutFuncAppMouseDown appMouseDown)
{

    // Set the callback
    gFuncAppMouseDown = appMouseDown;
}



//=============================================================================
//      Qut_SetMouseTrackFunc : Set the mouse tracking callback.
//-----------------------------------------------------------------------------
void
Qut_SetMouseTrackFunc(qutFuncAppMouseTrack appMouseTrack)
{

    // Set the callback
    gFuncAppMouseTrack = appMouseTrack;
}



//=============================================================================
//      Qut_SetMouseUpFunc : Set the mouse up callback.
//-----------------------------------------------------------------------------
void
Qut_SetMouseUpFunc(qutFuncAppMouseUp appMouseUp)
{

    // Set the callback
    gFuncAppMouseUp = appMouseUp;
}


//=============================================================================
//      Qut_SetIdleFunc : Set the idle callback.
//-----------------------------------------------------------------------------
void
Qut_SetIdleFunc(qutFuncAppIdle appIdle)
{

    // Set the callback
    gFuncAppIdle = appIdle;
}


//=============================================================================
//      Qut_SetRedrawFunc : Set the redraw callback.
//-----------------------------------------------------------------------------
void
Qut_SetRedrawFunc(qutFuncAppRedraw appRedraw)
{

    // Set the callback
    gFuncAppRedraw = appRedraw;
}





//=============================================================================
//      Qut_ReadModel : reads a model from storage.
//-----------------------------------------------------------------------------
TQ3GroupObject
Qut_ReadModel(TQ3StorageObject  storageObj)
{   TQ3GroupObject model = NULL;
    TQ3FileObject       fileObj;
    TQ3Object           tempObj;
    TQ3FileMode         fileMode;

    
    if( storageObj != NULL )
    {
		//create the model
		model = Q3DisplayGroup_New();
		
		if (model != NULL)
		{
			//create the file object
			fileObj = Q3File_New();
			if (fileObj != NULL)
			{
				// Set the storage for the file object
				Q3File_SetStorage(fileObj, storageObj);
				
				// open the file
				if (Q3File_OpenRead(fileObj, &fileMode) == kQ3Success)
				{
					//read the model
					while (Q3File_IsEndOfFile(fileObj) == kQ3False)
					{
						tempObj = Q3File_ReadObject( fileObj );
						if(tempObj != NULL)
						{
							if ( Q3Object_IsDrawable(tempObj) ) 
								Q3Group_AddObject(model, tempObj);
							Q3Object_Dispose(tempObj);
						}
					}
					
					// Done -- close
					Q3File_Close(fileObj);
				}
				
				Q3Object_Dispose(fileObj);
			}
		}
		
		Q3Object_Dispose(storageObj);
	}
        
    return model;
}





//=============================================================================
//      Private functions.
//-----------------------------------------------------------------------------
//      Qut_Initialise : Initialise Qut.
//-----------------------------------------------------------------------------
#pragma mark -
void
Qut_Initialise(void)
{
}





//=============================================================================
//      Qut_Terminate : Terminate Qut.
//-----------------------------------------------------------------------------
void
Qut_Terminate(void)
{


	// Clean up
	if (gView != NULL)
		Q3Object_Dispose(gView);

	if (gShaderIllumination != NULL)
		Q3Object_Dispose(gShaderIllumination);
}





//=============================================================================
//      Qut_RenderFrame : Render another frame.
//-----------------------------------------------------------------------------
void
Qut_RenderFrame(void)
{   double		startRenderTime, endRenderTime;
    TQ3Status   qd3dStatus;



    // Make sure we can render
    if (gView == NULL || gFuncAppRender == NULL)
        return;



    // Save the start time if this is the first update
	startRenderTime = qut_CPU_time();
    if (gFrameCount == 0)
	{
		gElapsedTime = 0.0;
	}



    // Call the pre-render callback, if any
    if (gFuncAppRenderPre != NULL)
        gFuncAppRenderPre(gView);



    // Render another frame
    qd3dStatus = Q3View_StartRendering(gView);
    if (qd3dStatus == kQ3Success)
        {
        do
            {
            Qut_SubmitDefaultState(gView);
            gFuncAppRender(gView);
            }
        while (Q3View_EndRendering(gView) == kQ3ViewStatusRetraverse);
        }



    // Call the post-render callback, if any
    if (gFuncAppRenderPost != NULL)
        {
        Q3View_Sync(gView);
        gFuncAppRenderPost(gView);
        }



    // Update the frame count. Once enough frames have passed, work out the
    // time it took to render them and from that the FPS. Since we can only
    // rely on CLOCKS_PER_SEC timing with ANSI C, we need to accumulate the
    // time over a number of frames to get an accurate rate
	endRenderTime = qut_CPU_time();
	gElapsedTime += endRenderTime - startRenderTime;
    gFrameCount++;

    if (gFrameCount > kFPSUpdateCount || gElapsedTime > kFPSUpdateTime)
        {
        gFPS        = (float)( gFrameCount / gElapsedTime );
        gFrameCount = 0;
        }
}





//=============================================================================
//      Qut_InvokeStyleCommand : Invoke a style command.
//-----------------------------------------------------------------------------
void
Qut_InvokeStyleCommand(TQ3Int32 theCmd)
{	TQ3DrawContextData			drawContextData;
	TQ3DrawContextObject		theDrawContext;



	// Handle the command
	switch (theCmd) {
		case kStyleCmdShaderNull:
		case kStyleCmdShaderLambert:
		case kStyleCmdShaderPhong:
			if (gShaderIllumination != NULL)
				Q3Object_Dispose(gShaderIllumination);
			
			if (theCmd == kStyleCmdShaderNull)
				gShaderIllumination = Q3NULLIllumination_New();
			else if (theCmd == kStyleCmdShaderLambert)
				gShaderIllumination = Q3LambertIllumination_New();
			else
				gShaderIllumination = Q3PhongIllumination_New();
			break;

		case kStyleCmdFillFilled:
			gStyleFill = kQ3FillStyleFilled;
			break;

		case kStyleCmdFillEdges:
			gStyleFill = kQ3FillStyleEdges;
			break;

		case kStyleCmdFillPoints:
			gStyleFill = kQ3FillStylePoints;
			break;

		case kStyleCmdBackfacingBoth:
			gStyleBackfacing = kQ3BackfacingStyleBoth;
			break;

		case kStyleCmdBackfacingRemove:
			gStyleBackfacing = kQ3BackfacingStyleRemove;
			break;

		case kStyleCmdBackfacingRemoveFront:
			gStyleBackfacing = kQ3BackfacingStyleRemoveFront;
			break;

		case kStyleCmdBackfacingFlip:
			gStyleBackfacing = kQ3BackfacingStyleFlip;
			break;

		case kStyleCmdInterpolationNone:
			gStyleInterpolation = kQ3InterpolationStyleNone;
			break;

		case kStyleCmdInterpolationVertex:
			gStyleInterpolation = kQ3InterpolationStyleVertex;
			break;

		case kStyleCmdInterpolationPixel:
			gStyleInterpolation = kQ3InterpolationStylePixel;
			break;

		case kStyleCmdOrientationClockwise:
			gStyleOrientation = kQ3OrientationStyleClockwise;
			break;

		case kStyleCmdOrientationCounterClockwise:
			gStyleOrientation = kQ3OrientationStyleCounterClockwise;
			break;

		case kStyleCmdAntiAliasNone:
			gStyleDataAntiAlias.state = kQ3Off;
			break;

		case kStyleCmdAntiAliasEdges:
			gStyleDataAntiAlias.state   = kQ3On;
			gStyleDataAntiAlias.mode    = kQ3AntiAliasModeMaskEdges;
			gStyleDataAntiAlias.quality = 1.0f;
			break;

		case kStyleCmdAntiAliasFilled:
			gStyleDataAntiAlias.state   = kQ3On;
			gStyleDataAntiAlias.mode    = kQ3AntiAliasModeMaskFilled;
			gStyleDataAntiAlias.quality = 1.0f;
			break;

		case kStyleCmdFogOn:
			gStyleDataFog.state    = kQ3On;
			gStyleDataFog.mode     = kQ3FogModeLinear;
			gStyleDataFog.fogStart = 2.0f;
			gStyleDataFog.fogEnd   = 5.0f;
			gStyleDataFog.density  = 0.5f;
			Q3ColorARGB_Set(&gStyleDataFog.color, 1.0f, 1.0f, 1.0f, 1.0f);

			// For more realistic fog, we try and use the clear colour of the draw context
			if (Q3View_GetDrawContext(gView, &theDrawContext) == kQ3Success)
			{
				Q3DrawContext_GetData(theDrawContext, &drawContextData);
				if (drawContextData.clearImageMethod == kQ3ClearMethodWithColor)
					gStyleDataFog.color = drawContextData.clearImageColor;

				Q3Object_Dispose(theDrawContext);
			}
			break;

		case kStyleCmdFogOff:
			gStyleDataFog.state = kQ3Off;
			break;

		case kStyleCmdSubdivisionConstant1:
			gStyleDataSubdivision.method = kQ3SubdivisionMethodConstant;
			gStyleDataSubdivision.c1     = 5.0f;
			gStyleDataSubdivision.c2     = 5.0f;
			break;

		case kStyleCmdSubdivisionConstant2:
			gStyleDataSubdivision.method = kQ3SubdivisionMethodConstant;
			gStyleDataSubdivision.c1     = 25.0f;
			gStyleDataSubdivision.c2     = 25.0f;
			break;

		case kStyleCmdSubdivisionConstant3:
			gStyleDataSubdivision.method = kQ3SubdivisionMethodConstant;
			gStyleDataSubdivision.c1     = 50.0f;
			gStyleDataSubdivision.c2     = 50.0f;
			break;

		case kStyleCmdSubdivisionConstant4:
			gStyleDataSubdivision.method = kQ3SubdivisionMethodConstant;
			gStyleDataSubdivision.c1     = 50.0f;
			gStyleDataSubdivision.c2     =  5.0f;
			break;

		case kStyleCmdSubdivisionWorldSpace1:
			gStyleDataSubdivision.method = kQ3SubdivisionMethodWorldSpace;
			gStyleDataSubdivision.c1     = 0.1f;
			gStyleDataSubdivision.c2     = 0.0f;
			break;

		case kStyleCmdSubdivisionWorldSpace2:
			gStyleDataSubdivision.method = kQ3SubdivisionMethodWorldSpace;
			gStyleDataSubdivision.c1     = 0.5f;
			gStyleDataSubdivision.c2     = 0.0f;
			break;

		case kStyleCmdSubdivisionWorldSpace3:
			gStyleDataSubdivision.method = kQ3SubdivisionMethodWorldSpace;
			gStyleDataSubdivision.c1     = 2.5f;
			gStyleDataSubdivision.c2     = 0.0f;
			break;

		case kStyleCmdSubdivisionScreenSpace1:
			gStyleDataSubdivision.method = kQ3SubdivisionMethodScreenSpace;
			gStyleDataSubdivision.c1     = 3.0f;
			gStyleDataSubdivision.c2     = 0.0f;
			break;

		case kStyleCmdSubdivisionScreenSpace2:
			gStyleDataSubdivision.method = kQ3SubdivisionMethodScreenSpace;
			gStyleDataSubdivision.c1     = 10.0f;
			gStyleDataSubdivision.c2     = 0.0f;
			break;

		case kStyleCmdSubdivisionScreenSpace3:
			gStyleDataSubdivision.method = kQ3SubdivisionMethodScreenSpace;
			gStyleDataSubdivision.c1     = 30.0f;
			gStyleDataSubdivision.c2     = 0.0f;
			break;
		
		default:
			break;
		}
}


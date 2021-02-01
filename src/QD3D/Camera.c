/****************************/
/*   	CAMERA.C    	    */
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

#include "globals.h"
#include "objects.h"
#include "camera.h"
#include "qd3d_support.h"
#include "3dmath.h"
#include "misc.h"
#include "player_control.h"
#include "input.h"
#include "mobjtypes.h"
#include "collision.h"
#include "skeletonjoints.h"
#include "sound2.h"
#include "terrain.h"
#include "myguy.h"
#include "enemy.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	float					gFramesPerSecond,gFramesPerSecondFrac,gMyHeightOffGround;
extern	ObjNode					*gPlayerObj;
extern	TQ3Point3D				gMyCoord;
extern	Boolean					gPlayerGotKilledFlag;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveCamera_Manual(void);
static void MoveCamera_FirstPerson(void);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	CAM_MINY			30

#define	CAMERA_CLOSEST		110
#define	CAMERA_FARTHEST		600


/*********************/
/*    VARIABLES      */
/*********************/

ObjNode		*gCameraNode = nil;
Byte		gCameraMode;

float		gCameraViewYAngle = 0;

TQ3Matrix4x4	gCameraWorldToViewMatrix,gCameraViewToFrustumMatrix,gCameraAdjustMatrix;

float		gCameraLookAtAccel,gCameraFromAccelY,gCameraFromAccel;
float		gCameraDistFromMe, gCameraHeightFactor,gCameraLookAtYOff;

/*************** MAKE CAMERA EVENT ***********************/
//
// This MUST be called after I've (the player) been created so that we know
// where to put the camera.
//

void MakeCameraEvent(void)
{		
float		rotY,x,z;
TQ3Point3D	from,to;

	ResetCameraSettings();
		

			/* MAKE CAMERA OBJECT */
			
	gNewObjectDefinition.genre = EVENT_GENRE;
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = CAMERA_SLOT;
	gNewObjectDefinition.moveCall = MoveCamera;
	gCameraNode = MakeNewObject(&gNewObjectDefinition);
	if (gCameraNode == nil)
		return;
			
	
			/* SET CAMERA STARTING COORDS */
	
	rotY = gPlayerObj->Rot.y;
	x = sin(rotY);																// calc point in front of player to start at
	z = cos(rotY);
	from.x = gMyCoord.x + (x*gCameraDistFromMe);
	from.y = gMyCoord.y; 
	from.z = gMyCoord.z + (z*gCameraDistFromMe) + 800;
		
	to = gMyCoord;
	to.y += 1000;
		
	QD3D_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);
		
}

/******************** RESET CAMERA SETTINGS **************************/

void ResetCameraSettings(void)
{
	gCameraMode 		= CAMERA_MODE_MANUAL;
	gCameraViewYAngle 	= 0;

	gCameraLookAtAccel 	= 8;
	gCameraFromAccel 	= 4.5;	//3.2;
	gCameraFromAccelY	= 3;
	gCameraDistFromMe 	= 300;
	gCameraHeightFactor = 0.3;
	gCameraLookAtYOff 	= 12;	
}

/*************** MOVE CAMERA ***************/

void MoveCamera(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
	
			/* SEE IF TOGGLE MODES */
			
	if (GetNewKeyState(kKey_CameraMode))	
	{
		gCameraMode++;
		if (gCameraMode > 1)
			gCameraMode = 0;
		switch(gCameraMode)
		{
			case	CAMERA_MODE_MANUAL:
					ResetCameraSettings();
					gPlayerObj->StatusBits &= ~STATUS_BIT_HIDDEN;
					break;
			case	CAMERA_MODE_FIRSTPERSON:
					gPlayerObj->StatusBits |= STATUS_BIT_HIDDEN;
					break;
		}
	}

			/************************/
			/* HANDLE CAMERA MOTION */
			/************************/

	if (gPlayerGotKilledFlag)											// if I'm dead, zoom in
	{	
		gCameraDistFromMe -= fps * 300;		
		if (gCameraDistFromMe < CAMERA_CLOSEST)
			gCameraDistFromMe = CAMERA_CLOSEST;
			
		gCameraLookAtYOff -= fps * 100;
		if (gCameraLookAtYOff < 0)
			gCameraLookAtYOff = 0;
			
		gCameraViewYAngle += 1.5*fps;	
		gCameraHeightFactor = .7;
		
		gCameraMode = CAMERA_MODE_MANUAL;
		MoveCamera_Manual();

	}
	else
	switch(gCameraMode)
	{
						/******************************/
						/* MOVE CAMERA IN MAUNAL MODE */
						/******************************/
						
		case	CAMERA_MODE_MANUAL:
				if (Nano_GetKeyState(kKey_ZoomIn))	
				{
					gCameraDistFromMe -= fps * 100;		// closer camera
					if (gCameraDistFromMe < CAMERA_CLOSEST)
						gCameraDistFromMe = CAMERA_CLOSEST;
				}
				else
				if (Nano_GetKeyState(kKey_ZoomOut))	
				{
					gCameraDistFromMe += fps * 100;		// farther camera
					if (gCameraDistFromMe > CAMERA_FARTHEST)
						gCameraDistFromMe = CAMERA_FARTHEST;
				}
				
						/* CHECK CAMERA VIEW ROTATION */
						
				if (Nano_GetKeyState(kKey_SwivelCameraLeft))
				{
					gCameraViewYAngle -= 2*fps;
				}
				else
				if (Nano_GetKeyState(kKey_SwivelCameraRight))
				{
					gCameraViewYAngle += 2*fps;
				}

					
						/* HANDLE CAMERA MOTION */
					
				MoveCamera_Manual();
				break;
						
				/************************************/
				/* MOVE CAMERA IN FIRST PERSON MODE */
				/************************************/
					
		case	CAMERA_MODE_FIRSTPERSON:
				MoveCamera_FirstPerson();
				break;
									
						
	}
	
	CalcCameraMatrixInfo(gGameViewInfoPtr);


}


/********************** CALC CAMERA MATRIX INFO ************************/

void CalcCameraMatrixInfo(QD3DSetupOutputType *setupInfo)
#if 0
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
	int temp, w, h;
	QD3D_GetCurrentViewport(setupInfo, &temp, &temp, &w, &h);
	float aspect = (float)w/(float)h;

			/* INIT PROJECTION MATRIX */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective (Q3Math_RadiansToDegrees(setupInfo->fov),	// fov
					aspect,					// aspect
					setupInfo->hither,		// hither
					setupInfo->yon);		// yon


			/* INIT MODELVIEW MATRIX */

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	TQ3CameraPlacement* placement = &setupInfo->cameraPlacement;
	gluLookAt(placement->cameraLocation.x, placement->cameraLocation.y, placement->cameraLocation.z,
			  placement->pointOfInterest.x, placement->pointOfInterest.y, placement->pointOfInterest.z,
			  placement->upVector.x, placement->upVector.y, placement->upVector.z);







			/* GET CAMERA VIEW MATRIX INFO */

#if 1	// TODO noquesa
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)&gCameraWorldToViewMatrix);				// get camera's world to view matrix
	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)&gCameraViewToFrustumMatrix);			// get camera's view to frustrum matrix (to calc screen coords)
//	Q3Matrix4x4_Multiply(&gCameraWorldToViewMatrix, &gCameraViewToFrustumMatrix, &gCameraWorldToFrustumMatrix);		// (from otto)

//	OGLMatrix4x4_GetFrustumToWindow(setupInfo, &gFrustumToWindowMatrix);		// (from otto)
//	OGLMatrix4x4_Multiply(&gWorldToFrustumMatrix, &gFrustumToWindowMatrix, &gWorldToWindowMatrix);		// (from otto)
#else
	Q3Camera_GetWorldToView(viewPtr->cameraObject, &gCameraWorldToViewMatrix);			// get camera's world to view matrix
	Q3Camera_GetViewToFrustum(viewPtr->cameraObject, &gCameraViewToFrustumMatrix);		// get camera's view to frustrum matrix (to calc screen coords)
#endif
	
	
			/* CALCULATE THE ADJUSTMENT MATRIX */
			//
			// this gets a view->world matrix for putting stuff in the infobar.
			//
				
	Q3Matrix4x4_Invert(&gCameraWorldToViewMatrix,&gCameraAdjustMatrix);
	



	CHECK_GL_ERROR();
}
#endif


/**************** MOVE CAMERA: MANUAL ********************/

static void MoveCamera_Manual(void)
{
TQ3Point3D	from,to,target;
float		rotY,x,z,distX,distZ,distY,dist;

			/**********************/
			/* CALC LOOK AT POINT */
			/**********************/

	target.x = gMyCoord.x;								// accelerate "to" toward target "to"
	target.y = gMyCoord.y + gCameraLookAtYOff;
	target.z = gMyCoord.z;


	distX = target.x - gGameViewInfoPtr->currentCameraLookAt.x;
	distY = target.y - gGameViewInfoPtr->currentCameraLookAt.y;
	distZ = target.z - gGameViewInfoPtr->currentCameraLookAt.z;

	if (distX > 50)		
		distX = 50;
	else
	if (distX < -50)
		distX = -50;
	if (distZ > 50)
		distZ = 50;
	else
	if (distZ < -50)
		distZ = -50;

	to.x = gGameViewInfoPtr->currentCameraLookAt.x+(distX * (gFramesPerSecondFrac * gCameraLookAtAccel));
	to.y = gGameViewInfoPtr->currentCameraLookAt.y+(distY * (gFramesPerSecondFrac * (gCameraLookAtAccel*.7)));
	to.z = gGameViewInfoPtr->currentCameraLookAt.z+(distZ * (gFramesPerSecondFrac * gCameraLookAtAccel));


			/*******************/
			/* CALC FROM POINT */
			/*******************/
				
	rotY = gPlayerObj->Rot.y;								// get rotation value
	rotY += gCameraViewYAngle;												// factor in camera view rot
	x = sin(rotY);															// calc point around player to be
	z = cos(rotY);
	target.x = gMyCoord.x + (x * gCameraDistFromMe);
	target.z = gMyCoord.z + (z * gCameraDistFromMe);


			/* MOVE CAMERA TOWARDS POINT */
			
	distX = target.x - gGameViewInfoPtr->currentCameraCoords.x;
	distZ = target.z - gGameViewInfoPtr->currentCameraCoords.z;
	
	if (distX > 100)													// pin max accel factor
		distX = 100;
	else
	if (distX < -100)
		distX = -100;
	if (distZ > 100)
		distZ = 100;
	else
	if (distZ < -100)
		distZ = -100;
		
	from.x = gGameViewInfoPtr->currentCameraCoords.x+(distX * (gFramesPerSecondFrac * gCameraFromAccel));
	from.z = gGameViewInfoPtr->currentCameraCoords.z+(distZ * (gFramesPerSecondFrac * gCameraFromAccel));


			/***************/
			/* CALC FROM Y */
			/***************/
	
	if (gPlayerObj->JetThrust)														// SPECIAL if jet is on
	{
		target.y = to.y + 40;
	}
	else		
	{
		dist = CalcQuickDistance(from.x, from.z, to.x, to.z);						// dist from camera to target
		dist -= CAMERA_CLOSEST;
		if (dist < 0)
			dist = 0;
		
		target.y = to.y + (dist*gCameraHeightFactor) + CAM_MINY;					// calc desired y based on dist and height factor	
		target.y += gMyHeightOffGround;	
	}
	
	dist = (target.y - gGameViewInfoPtr->currentCameraCoords.y)*gCameraFromAccelY;	// calc dist from current y to desired y
	from.y = gGameViewInfoPtr->currentCameraCoords.y+(dist*gFramesPerSecondFrac);

	if (from.y < (to.y+CAM_MINY))													// make sure camera never under the "to" point (insures against camera flipping over)
		from.y = (to.y+CAM_MINY);	
	
	
			/* MAKE SURE NOT UNDERGROUND */
			
	dist = GetTerrainHeightAtCoord_Planar(from.x, from.z) + 50;
	if (from.y < dist)
		from.y = dist;

	

				/* UPDATE CAMERA INFO */	

	QD3D_UpdateCameraFromTo(gGameViewInfoPtr,&from,&to);
}


/************************ MOVE CAMERA_FIRST PERSON *********************************/

static void MoveCamera_FirstPerson(void)
{
static TQ3Point3D	inPt = {0,20,-100};
static TQ3Point3D	inPt2 = {0,20,20};
TQ3Point3D outPt,outPt2;

				/* GET LOOK AT PT */
				
	FindCoordOnJoint(gPlayerObj, MYGUY_LIMB_HEAD, &inPt, &outPt);
	
				/* GET FROM PT */
				
	FindCoordOnJoint(gPlayerObj, MYGUY_LIMB_HEAD, &inPt2, &outPt2);

	QD3D_UpdateCameraFromTo(gGameViewInfoPtr,&outPt2,&outPt);
}







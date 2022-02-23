/****************************/
/*   	CAMERA.C    	    */
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
TQ3Matrix4x4	gCameraWorldToFrustumMatrix;

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

	(void) theNode;
	
			/* SEE IF TOGGLE MODES */
			
	if (GetNewNeedState(kNeed_CameraMode))
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
				if (GetNeedState(kNeed_ZoomIn))
				{
					gCameraDistFromMe -= fps * 100;		// closer camera
					if (gCameraDistFromMe < CAMERA_CLOSEST)
						gCameraDistFromMe = CAMERA_CLOSEST;
				}
				else
				if (GetNeedState(kNeed_ZoomOut))
				{
					gCameraDistFromMe += fps * 100;		// farther camera
					if (gCameraDistFromMe > CAMERA_FARTHEST)
						gCameraDistFromMe = CAMERA_FARTHEST;
				}
				
						/* CHECK CAMERA VIEW ROTATION */
						
				if (GetNeedState(kNeed_CameraLeft))
				{
					gCameraViewYAngle -= 2*fps;
				}
				else
				if (GetNeedState(kNeed_CameraRight))
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

#if 0	// noquesa - MoveCamera_Manual and MoveCamera_FirstPerson ultimately set the camera matrix
	CalcCameraMatrixInfo(gGameViewInfoPtr);
#endif


}


/********************** FILL PROJECTION MATRIX ************************/
//
// Equivalent to gluPerspective
//

static void FillProjectionMatrix(TQ3Matrix4x4* m, float fov, float aspect, float hither, float yon)
{
	float f = 1.0f / tanf(fov/2.0f);

#define M(x,y) m->value[x][y]
	M(0,0) = f/aspect;		M(1,0) = 0;			M(2,0) = 0;							M(3,0) = 0;
	M(0,1) = 0;				M(1,1) = f;			M(2,1) = 0;							M(3,1) = 0;
	M(0,2) = 0;				M(1,2) = 0;			M(2,2) = (yon+hither)/(hither-yon);	M(3,2) = 2*yon*hither/(hither-yon);
	M(0,3) = 0;				M(1,3) = 0;			M(2,3) = -1;						M(3,3) = 0;
#undef M
}


/********************** FILL LOOKAT MATRIX ************************/
//
// Equivalent to gluLookAt
//

static void FillLookAtMatrix(
		TQ3Matrix4x4* m,
		const TQ3Point3D* eye,
		const TQ3Point3D* target,
		const TQ3Vector3D* upDir)
{
	TQ3Vector3D forward;
	Q3Point3D_Subtract(eye, target, &forward);
	Q3Vector3D_Normalize(&forward, &forward);

	TQ3Vector3D left;
	Q3Vector3D_Cross(upDir, &forward, &left);
	Q3Vector3D_Normalize(&left, &left);

	TQ3Vector3D up;
	Q3Vector3D_Cross(&forward, &left, &up);

	TQ3Vector3D eyeVec = {eye->x, eye->y, eye->z};
	float tx = Q3Vector3D_Dot(&eyeVec, &left);
	float ty = Q3Vector3D_Dot(&eyeVec, &up);
	float tz = Q3Vector3D_Dot(&eyeVec, &forward);

#define M(x,y) m->value[x][y]
	M(0,0) = left.x;	M(1,0) = left.y;	M(2,0) = left.z;		M(3,0) = -tx;
	M(0,1) = up.x;		M(1,1) = up.y;		M(2,1) = up.z;			M(3,1) = -ty;
	M(0,2) = forward.x;	M(1,2) = forward.y;	M(2,2) = forward.z;		M(3,2) = -tz;
	M(0,3) = 0;			M(1,3) = 0;			M(2,3) = 0;				M(3,3) = 1;
#undef M
}


/********************** CALC CAMERA MATRIX INFO ************************/

void CalcCameraMatrixInfo(QD3DSetupOutputType *setupInfo)
{
			/* INIT PROJECTION MATRIX */

	glMatrixMode(GL_PROJECTION);

	FillProjectionMatrix(
		&gCameraViewToFrustumMatrix,
		setupInfo->fov,
		QD3D_GetCurrentViewportAspectRatio(setupInfo),
		setupInfo->hither,
		setupInfo->yon);

	glLoadMatrixf((const GLfloat*)&gCameraViewToFrustumMatrix.value[0][0]);


			/* INIT MODELVIEW MATRIX */

	glMatrixMode(GL_MODELVIEW);

	FillLookAtMatrix(
		&gCameraWorldToViewMatrix,
		&setupInfo->cameraPlacement.cameraLocation,
		&setupInfo->cameraPlacement.pointOfInterest,
		&setupInfo->cameraPlacement.upVector);

	glLoadMatrixf((const GLfloat*)&gCameraWorldToViewMatrix.value[0][0]);






			/* UPDATE LIGHT POSITIONS */

	for (int i = 0; i < setupInfo->lights.numFillLights; i++)
	{
		GLfloat lightVec[4];

		lightVec[0] = -setupInfo->lights.fillDirection[i].x;			// negate vector because OGL is stupid
		lightVec[1] = -setupInfo->lights.fillDirection[i].y;
		lightVec[2] = -setupInfo->lights.fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);
	}


			/* GET CAMERA VIEW MATRIX INFO */

	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)&gCameraWorldToViewMatrix.value[0][0]);		// get camera's world to view matrix
	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)&gCameraViewToFrustumMatrix.value[0][0]);	// get camera's view to frustrum matrix (to calc screen coords)
	Q3Matrix4x4_Multiply(&gCameraWorldToViewMatrix, &gCameraViewToFrustumMatrix, &gCameraWorldToFrustumMatrix);		// calc world to frustum matrix


			/* CALCULATE THE ADJUSTMENT MATRIX */
			//
			// this gets a view->world matrix for putting stuff in the infobar.
			//
				
	Q3Matrix4x4_Invert(&gCameraWorldToViewMatrix,&gCameraAdjustMatrix);
	



	CHECK_GL_ERROR();
}


/**************** MOVE CAMERA: MANUAL ********************/

static void MoveCamera_Manual(void)
{
TQ3Point3D	from,to,target;
float		rotY,x,z,distX,distZ,distY,dist;

const TQ3Point3D*	currentCameraLookAt	= &gGameViewInfoPtr->cameraPlacement.pointOfInterest;
const TQ3Point3D*	currentCameraCoords	= &gGameViewInfoPtr->cameraPlacement.cameraLocation;

			/**********************/
			/* CALC LOOK AT POINT */
			/**********************/

	target.x = gMyCoord.x;								// accelerate "to" toward target "to"
	target.y = gMyCoord.y + gCameraLookAtYOff;
	target.z = gMyCoord.z;


	distX = target.x - currentCameraLookAt->x;
	distY = target.y - currentCameraLookAt->y;
	distZ = target.z - currentCameraLookAt->z;

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

	to.x = currentCameraLookAt->x + (distX * (gFramesPerSecondFrac * gCameraLookAtAccel));
	to.y = currentCameraLookAt->y + (distY * (gFramesPerSecondFrac * (gCameraLookAtAccel*.7)));
	to.z = currentCameraLookAt->z + (distZ * (gFramesPerSecondFrac * gCameraLookAtAccel));


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
			
	distX = target.x - currentCameraCoords->x;
	distZ = target.z - currentCameraCoords->z;

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

	from.x = currentCameraCoords->x + (distX * (gFramesPerSecondFrac * gCameraFromAccel));
	from.z = currentCameraCoords->z + (distZ * (gFramesPerSecondFrac * gCameraFromAccel));


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

	dist = (target.y - currentCameraCoords->y)*gCameraFromAccelY;	// calc dist from current y to desired y
	from.y = currentCameraCoords->y+(dist*gFramesPerSecondFrac);

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







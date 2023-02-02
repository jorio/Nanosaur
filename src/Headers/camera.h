//
// camera.h
//

#define	HITHER_DISTANCE	10.0f

extern float YON_DISTANCE;


enum
{
	CAMERA_MODE_MANUAL,
	CAMERA_MODE_FIRSTPERSON
};

extern	void MakeCameraEvent(void);
extern	void MoveCamera(ObjNode *theNode);
extern	void CalcCameraMatrixInfo(QD3DSetupOutputType *);
extern	void ResetCameraSettings(void);
Boolean IsFirstPersonSteadyCamera(void);


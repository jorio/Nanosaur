//
// camera.h
//

#define	HITHER_DISTANCE	10.0f

#ifdef PRO_MODE
#define	YON_DISTANCE	2800.0f
#else
#define	YON_DISTANCE	1900.0f
#endif


enum
{
	CAMERA_MODE_MANUAL,
	CAMERA_MODE_FIRSTPERSON
};

extern	void MakeCameraEvent(void);
extern	void MoveCamera(ObjNode *theNode);
extern	void CalcCameraMatrixInfo(QD3DSetupOutputType *);
extern	void ResetCameraSettings(void);



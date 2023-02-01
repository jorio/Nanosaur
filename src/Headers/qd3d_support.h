//
// qd3d_support.h
//

#pragma once

#include <QD3D.h>


#define	DEFAULT_FPS			4

#define	MAX_FILL_LIGHTS		4

typedef	struct
{
	GWorldPtr				gworld;
	TQ3ColorRGBA			clearColor;
	Rect					paneClip;			// not pane size, but clip:  left = amount to clip off left
	int						backdropFit;
}QD3DViewDefType;


typedef	struct
{
	TQ3InterpolationStyle	interpolation;
	TQ3BackfacingStyle		backfacing;
	TQ3FillStyle			fill;
	Boolean					usePhong;
}QD3DStyleDefType;


typedef struct
{
	TQ3Point3D				from;
	TQ3Point3D				to;
	TQ3Vector3D				up;
	float					hither;
	float					yon;
	float					fov;
}QD3DCameraDefType;

typedef	struct
{
	Boolean			useFog;
	float			fogHither;
	float			fogYon;
	TQ3FogMode		fogMode;

	float			ambientBrightness;
	TQ3ColorRGB		ambientColor;
	long			numFillLights;
	TQ3Vector3D		fillDirection[MAX_FILL_LIGHTS];
	TQ3ColorRGB		fillColor[MAX_FILL_LIGHTS];
	float			fillBrightness[MAX_FILL_LIGHTS];
}QD3DLightDefType;


		/* QD3DSetupInputType */
		
typedef struct
{
	QD3DViewDefType			view;
	QD3DStyleDefType		styles;
	QD3DCameraDefType		camera;
	QD3DLightDefType		lights;
}QD3DSetupInputType;


		/* QD3DSetupOutputType */

typedef struct
{
	Boolean					isActive;
	Rect					paneClip;			// not pane size, but clip:  left = amount to clip off left
	bool					needScissorTest;
	int						backdropFit;
	float					hither,yon;
	float					fov;
	TQ3CameraPlacement		cameraPlacement;
	QD3DLightDefType		lights;
}QD3DSetupOutputType;


//===========================================================

void QD3D_Boot(void);
void QD3D_Shutdown(void);
extern	void QD3D_SetupWindow(QD3DSetupInputType *setupDefPtr, QD3DSetupOutputType **outputHandle);
extern	void QD3D_DisposeWindowSetup(QD3DSetupOutputType **dataHandle);
extern	void QD3D_UpdateCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Point3D *from, TQ3Point3D *to);
extern	void QD3D_DrawScene(QD3DSetupOutputType *setupInfo, void (*drawRoutine)(QD3DSetupOutputType *));
extern	void QD3D_UpdateCameraFrom(QD3DSetupOutputType *setupInfo, TQ3Point3D *from);
extern	void QD3D_MoveCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Vector3D *moveVector, TQ3Vector3D *lookAtVector);
extern	void	QD3D_CalcFramesPerSecond(void);
extern	void QD3D_NewViewDef(QD3DSetupInputType *viewDef);

void MakeShadowTexture(void);

void QD3D_OnWindowResized(void);

float QD3D_GetCurrentViewportAspectRatio(const QD3DSetupOutputType *setupInfo);


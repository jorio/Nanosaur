//
// qd3d_support.h
//

#ifndef QD3D_SUP
#define QD3D_SUP



#include <QD3D.h>


#define	DEFAULT_FPS			4

#define	MAX_FILL_LIGHTS		4

typedef	struct
{
	Boolean					useWindow;			// true if render to window, false if render to pixmap
	WindowPtr				displayWindow;
	GWorldPtr				gworld;
	TQ3ObjectType			rendererType;
	TQ3ColorARGB			clearColor;
	Rect					paneClip;			// not pane size, but clip:  left = amount to clip off left
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
#if 0 // TODO noquesa
	TQ3ViewObject			viewObject;
	TQ3ShaderObject			shaderObject;
	TQ3ShaderObject			nullShaderObject;
	TQ3StyleObject			interpolationStyle;
	TQ3StyleObject			backfacingStyle;
	TQ3StyleObject			fillStyle;
	TQ3CameraObject			cameraObject;	// another ref is in viewObject, this one's just for convenience!
	TQ3GroupObject			lightGroup;		// another ref is in viewObject, this one's just for convenience!
	TQ3DrawContextObject	drawContext;	// another ref is in viewObject, this one's just for convenience!
#endif
	WindowPtr				window;
	Rect					paneClip;			// not pane size, but clip:  left = amount to clip off left
	TQ3Point3D				currentCameraCoords;
	TQ3Point3D				currentCameraLookAt;
	float					hither,yon;
}QD3DSetupOutputType;


//===========================================================

extern	void QD3D_Boot(void);
extern	void QD3D_SetupWindow(QD3DSetupInputType *setupDefPtr, QD3DSetupOutputType **outputHandle);
extern	void QD3D_DisposeWindowSetup(QD3DSetupOutputType **dataHandle);
extern	void QD3D_UpdateCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Point3D *from, TQ3Point3D *to);
extern	void QD3D_DrawScene(QD3DSetupOutputType *setupInfo, void (*drawRoutine)(QD3DSetupOutputType *));
extern	void QD3D_UpdateCameraFrom(QD3DSetupOutputType *setupInfo, TQ3Point3D *from);
extern	void QD3D_MoveCameraFromTo(QD3DSetupOutputType *setupInfo, TQ3Vector3D *moveVector, TQ3Vector3D *lookAtVector);
extern	void	QD3D_CalcFramesPerSecond(void);
#if 0 // TODO noquesa
extern	TQ3SurfaceShaderObject	QD3D_GetTextureMap(long	textureRezID, FSSpec *myFSSpec);
extern	TQ3GroupPosition QD3D_AddPointLight(QD3DSetupOutputType *setupInfo,TQ3Point3D *point, TQ3ColorRGB *color, float brightness);
extern	void QD3D_SetPointLightCoords(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition, TQ3Point3D *point);
extern	void QD3D_SetPointLightBrightness(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition, float bright);
extern	void QD3D_DeleteLight(QD3DSetupOutputType *setupInfo, TQ3GroupPosition lightPosition);
extern	TQ3SurfaceShaderObject	QD3D_PICTToTexture(PicHandle picture);
extern	TQ3SurfaceShaderObject	QD3D_GWorldToTexture(GWorldPtr theGWorld, Boolean pointToGWorld);
#endif
extern	void SetBackFaceStyle(QD3DSetupOutputType *setupInfo, TQ3BackfacingStyle style);
extern	void SetFillStyle(QD3DSetupOutputType *setupInfo, TQ3FillStyle style);
extern	void QD3D_DeleteAllLights(QD3DSetupOutputType *setupInfo);
extern	TQ3GroupPosition QD3D_AddFillLight(QD3DSetupOutputType *setupInfo,TQ3Vector3D *fillVector, TQ3ColorRGB *color, float brightness);
extern	TQ3GroupPosition QD3D_AddAmbientLight(QD3DSetupOutputType *setupInfo, TQ3ColorRGB *color, float brightness);
extern	void QD3D_DoMemoryError(void);
extern	void QD3D_ShowRecentError(void);
extern	void QD3D_NewViewDef(QD3DSetupInputType *viewDef, WindowPtr theWindow);
#if 0 // TODO noquesa
extern	TQ3SurfaceShaderObject	QD3D_Data16ToTexture_NoMip(Ptr data, short width, short height);
extern	TQ3StorageObject QD3D_GetMipmapStorageObjectFromAttrib(TQ3AttributeSet attribSet);
extern	GWorldPtr QD3D_MipmapToGWorld(TQ3Mipmap *mipmap, Rect *r);
extern	void QD3D_GWorldToMipMap(GWorldPtr pGWorld, TQ3Mipmap *mipmap, Boolean pointToGWorld);
#endif

extern	void QD3D_SetRaveFog(QD3DSetupOutputType *setupInfo, float fogHither, float fogYon, TQ3ColorARGB *fogColor, TQ3FogMode fogMode);
extern	void QD3D_SetTextureFilter(unsigned long textureMode);
extern	void QD3D_SetTriangleCacheMode(Boolean isOn);
extern	void QD3D_SetTextureWrapMode(int mode);
extern	void QD3D_SetBlendingMode(int mode);

extern	void QD3D_OnWindowResized(int windowWidth, int windowHeight);



#endif





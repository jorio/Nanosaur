#ifndef ATI_RAVE_H
#define ATI_RAVE_H
/****************************************************************/
/* headers														*/
/****************************************************************/
#include "RAVE.h"
#include "gl.h"

/****************************************************************/
/* defines														*/
/****************************************************************/
#define kQAGL_Nearest		0
#define	kQAGL_Linear		1

#define QATIClearDrawBuffer(atiExtens,drawContext,rect) \
		(*((atiExtens)->clearDrawBuffer))( drawContext, rect )

#define QATIClearZBuffer(atiExtens,drawContext,rect) \
		(*((atiExtens)->clearZBuffer))( drawContext, rect )

#define QATITextureUpdate(atiExtens,flags,pixelType,images,texture) \
		(*((atiExtens)->textureUpdate))( flags, pixelType, images, texture )
		
#define QATITextureBindCodeBook(atiExtens,texture,pCodebook) \
		(*((atiExtens)->bindCodeBook))(texture, pCodebook)				
			
/****************************************************************/
/* typedefs														*/
/****************************************************************/
typedef enum TQATIGestaltSelector
{
	kQATIGestalt_CurrentContext 	= 1000,
	kQATIGestalt_NumATISelectors	= 1
	
} TQATIGestaltSelector;

typedef enum TQATIImagePixelType {
	kQATIPixel_RGB4444				= 1000,
	kQATIPixel_ARGB4444				= 1001,
	kQATIPixel_YUV422				= 1002,
	kQATIPixel_ARGB8				= 1003,
	kQATIPixel_NumPixelTypes		= 4
} TQATIImagePixelType;

typedef enum {
	kQATIFogDisable					= 0,
	kQATIFogExp						= 1,
	kQATIFogExp2					= 2,
	kQATIFogAlpha					= 3,
	kQATIFogLinear					= 4
} TQATIFogMode;

typedef enum {
	kQATIComposeFunctionBlend		= 0,
	kQATIComposeFunctionModulate	= 1,
	kQATIComposeFunctionAddSpec		= 2
} TQATIComposeFunction;

typedef enum {
	kQATIDitherDisable				= 0,
	kQATIDitherErrDiff				= 1,
	kQATIDitherLUT					= 2
} TQATIDitherMode;

typedef struct {
    unsigned long       flags;              /* Mask of kQATexture_xxx flags */
    unsigned long		pixelType;          /* Depth, color space, etc. */
    TQAImage      		image;           	/* Image(s) for texture */
} TQATITexImage;

typedef enum
{
	kATITriCache		=	kQATag_EngineSpecific_Minimum,
	kATIClearPreCallback=	kQATag_EngineSpecific_Minimum + 1,
 	kATIFogMode			=	kQATag_EngineSpecific_Minimum + 2,
	kATIFogColor_r		=	kQATag_EngineSpecific_Minimum + 3,
 	kATIFogColor_g		=	kQATag_EngineSpecific_Minimum + 4,
 	kATIFogColor_b		=	kQATag_EngineSpecific_Minimum + 5,
 	kATIFogColor_a		=	kQATag_EngineSpecific_Minimum + 6,
 	kATIFogDensity      =   kQATag_EngineSpecific_Minimum + 7,
 	kATIFogStart        =   kQATag_EngineSpecific_Minimum + 8,
 	kATIFogEnd          =   kQATag_EngineSpecific_Minimum + 9,
 	kATILodBias			=	kQATag_EngineSpecific_Minimum + 10,
 	kATIChipID			=	kQATag_EngineSpecific_Minimum + 11,
 	kATICompositing     =   kQATag_EngineSpecific_Minimum + 12,
	kATICompositingFunc =   kQATag_EngineSpecific_Minimum + 13,
 	kATICompositingFactor=  kQATag_EngineSpecific_Minimum + 14,
 	kATISecondTexMin    =   kQATag_EngineSpecific_Minimum + 15,
 	kATISecondTexMag    =   kQATag_EngineSpecific_Minimum + 16,
	kATIVQTexCompresion =   kQATag_EngineSpecific_Minimum + 17,
 	kATISecond_Texture  =   kQATag_EngineSpecific_Minimum + 18,
 	kATICompositingAlpha=	kQATag_EngineSpecific_Minimum + 19,
 	kATINoDefaultClear  =	kQATag_EngineSpecific_Minimum + 20,
	kATIRaveExtFuncs	=	kQATag_EngineSpecific_Minimum + 21,
	kATIEnableZWrite	=	kQATag_EngineSpecific_Minimum + 22,
	kATIRop2			=	kQATag_EngineSpecific_Minimum + 23,
	kATIAlphaTestEnable =	kQATag_EngineSpecific_Minimum + 24,
	kATIAlphaTestRef	=	kQATag_EngineSpecific_Minimum + 25,
	kATIAlphaTestFunc	=	kQATag_EngineSpecific_Minimum + 26,
	kATITexCompress		=	kQATag_EngineSpecific_Minimum + 27,
	kATI_TextureContext =	kQATag_EngineSpecific_Minimum + 28,
	kATISecond_TextureContext = kQATag_EngineSpecific_Minimum + 29,
	kATISecondTextureWrapU 	  = kQATag_EngineSpecific_Minimum + 30,
	kATISecondTextureWrapV 	  = kQATag_EngineSpecific_Minimum + 31,
	kATISafeCallbacks	= 	kQATag_EngineSpecific_Minimum + 32,
	kATITextureAddr		=	kQATag_EngineSpecific_Minimum + 33,
	kATIAlphaEnable		=	kQATag_EngineSpecific_Minimum + 34,
	kATIRedEnable		=	kQATag_EngineSpecific_Minimum + 35,
	kATIGreenEnable		=	kQATag_EngineSpecific_Minimum + 36,
	kATIBlueEnable		=	kQATag_EngineSpecific_Minimum + 37,
	kATINotifyZBuffer	=	kQATag_EngineSpecific_Minimum + 38,
	kATIModulate2x		=   kQATag_EngineSpecific_Minimum + 39,
	kATIDitherMode		=	kQATag_EngineSpecific_Minimum + 40,
	kATIChipBusy		=	kQATag_EngineSpecific_Minimum + 41,
	kATIMeshAsStrip		=	kQATag_EngineSpecific_Minimum + 42,
	kATITags			=	43
} E_ATIVAR;

/*
 * ATI RAVE extension function pointers
 */
typedef TQAError (*TQATIClearDrawBuffer)( 
	const TQADrawContext	*drawContext, 
	const TQARect 			*dirtyRect );
	
typedef TQAError (*TQATIClearZBuffer)(
	const TQADrawContext	*drawContext,
	const TQARect 			*dirtyRect );

typedef TQAError (*TQATITextureUpdate)(
	unsigned long       	flags,             	/* Mask of kQATexture_xxx flags */  
	TQAImagePixelType		pixelType,          /* Depth, color space, etc. */
	const TQAImage 			*images,
	TQATexture 				*texture );  		/* Previously allocated by QATextureNew() */			/* new Image(s) for texture contents */

typedef TQAError (*TQATITextureBindCodeBook)(
	TQATexture 				*texture,
	void					*pCodebook );

typedef struct RaveExtFuncs{
	TQATIClearDrawBuffer		clearDrawBuffer;
	TQATIClearZBuffer			clearZBuffer;
	TQATITextureUpdate			textureUpdate;
	TQATITextureBindCodeBook    bindCodeBook; 
}RaveExtFuncs;

#endif

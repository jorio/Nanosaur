/*  NAME:
        Qut.h

    DESCRIPTION:
        Header file for Qut.c.

    COPYRIGHT:
        Copyright (c) 1999-2004, Quesa Developers. All rights reserved.

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
#ifndef QUT_HDR
#define QUT_HDR
//=============================================================================
//      Include files
//-----------------------------------------------------------------------------
// Everyone but Mac OS X Mach-O
#if !defined(TARGET_RT_MAC_MACHO) || !TARGET_RT_MAC_MACHO
	#include "Quesa.h"
	#include "QuesaCamera.h"
	#include "QuesaCustomElements.h"
	#include "QuesaDrawContext.h"
	#include "QuesaErrors.h"
	#include "QuesaExtension.h"
	#include "QuesaGeometry.h"
	#include "QuesaGroup.h"
	#include "QuesaIO.h"
	#include "QuesaLight.h"
	#include "QuesaMath.h"
	#include "QuesaMemory.h"
	#include "QuesaPick.h"
	#include "QuesaRenderer.h"
	#include "QuesaSet.h"
	#include "QuesaShader.h"
	#include "QuesaStorage.h"
	#include "QuesaString.h"
	#include "QuesaStyle.h"
	#include "QuesaTransform.h"
	#include "QuesaView.h"
// Mac OS X Mach-O framework layout
#else
	#include <Quesa/Quesa.h>
	#include <Quesa/QuesaCamera.h>
	#include <Quesa/QuesaCustomElements.h>
	#include <Quesa/QuesaDrawContext.h>
	#include <Quesa/QuesaErrors.h>
	#include <Quesa/QuesaExtension.h>
	#include <Quesa/QuesaGeometry.h>
	#include <Quesa/QuesaGroup.h>
	#include <Quesa/QuesaIO.h>
	#include <Quesa/QuesaLight.h>
	#include <Quesa/QuesaMath.h>
	#include <Quesa/QuesaMemory.h>
	#include <Quesa/QuesaPick.h>
	#include <Quesa/QuesaRenderer.h>
	#include <Quesa/QuesaSet.h>
	#include <Quesa/QuesaShader.h>
	#include <Quesa/QuesaStorage.h>
	#include <Quesa/QuesaString.h>
	#include <Quesa/QuesaStyle.h>
	#include <Quesa/QuesaTransform.h>
	#include <Quesa/QuesaView.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>





//=============================================================================
//      Platform specific
//-----------------------------------------------------------------------------
#if QUESA_OS_MACINTOSH
	
	#include "QutMac.h"
#endif

#if QUESA_OS_WIN32
	#include <Windows.h>
	#pragma warning(disable : 4068)
#endif





//=============================================================================
//      Textures
//-----------------------------------------------------------------------------
//#include "QutTexture.h"





//=============================================================================
//		C++ preamble
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif





//=============================================================================
//      Constants
//-----------------------------------------------------------------------------
#define kMenuItemLast							32767
#define kMenuItemDivider						"-"





//=============================================================================
//      Types
//-----------------------------------------------------------------------------
typedef TQ3ViewObject (*qutFuncAppCreateView)();
typedef void (*qutFuncAppConfigureView)(TQ3ViewObject			theView,
										TQ3DrawContextObject	theDrawContext,
										TQ3CameraObject			theCamera);
typedef void (*qutFuncAppMenuSelect)(TQ3ViewObject theView, TQ3Uns32 menuItem);
typedef void (*qutFuncAppRenderPre)(TQ3ViewObject theView);
typedef void (*qutFuncAppRender)(TQ3ViewObject theView);
typedef void (*qutFuncAppRenderPost)(TQ3ViewObject theView);
typedef void (*qutFuncAppMouseDown)(TQ3ViewObject theView, TQ3Point2D mousePoint);
typedef void (*qutFuncAppMouseTrack)(TQ3ViewObject theView, TQ3Point2D mouseDiff);
typedef void (*qutFuncAppMouseUp)(TQ3ViewObject theView, TQ3Point2D mousePoint);
typedef void (*qutFuncAppIdle)(TQ3ViewObject theView);
typedef void (*qutFuncAppRedraw)(TQ3ViewObject theView);





//=============================================================================
//      Function prototypes
//-----------------------------------------------------------------------------
// App
void	App_Initialise(void);
void	App_Terminate(void);


// Qut - Platform independent
void			Qut_CreateView(qutFuncAppCreateView appCreateView,
								qutFuncAppConfigureView appConfigureView);
void			Qut_CalcBounds(TQ3ViewObject theView, TQ3Object theObject, TQ3BoundingBox *theBounds);
void			Qut_CalcBoundingSphere(TQ3ViewObject theView, TQ3Object theObject, TQ3BoundingSphere *theBoundingSphere);
void			Qut_SubmitDefaultState(TQ3ViewObject theView);
void   		   *Qut_GetWindow(void);
void			Qut_SetRenderPreFunc(qutFuncAppRenderPre   appRenderPre);
void			Qut_SetRenderFunc(qutFuncAppRender         appRender);
void			Qut_SetRenderPostFunc(qutFuncAppRenderPost appRenderPost);
void			Qut_SetMouseDownFunc(qutFuncAppMouseDown   appMouseTrack);
void			Qut_SetMouseTrackFunc(qutFuncAppMouseTrack appMouseTrack);
void			Qut_SetMouseUpFunc(qutFuncAppMouseUp	   appMouseUp);
void			Qut_SetIdleFunc(qutFuncAppIdle	 		   appIdle);
void			Qut_SetRedrawFunc(qutFuncAppRedraw	 	   appRedraw);
TQ3GroupObject	Qut_ReadModel(TQ3StorageObject	storageObj);


// Qut - Platform specific
void					Qut_CreateWindow(const char		*windowTitle,
											TQ3Uns32	theWidth,
											TQ3Uns32	theHeight,
											TQ3Boolean	canResize);
TQ3DrawContextObject	Qut_CreateDrawContext(void);
TQ3StorageObject		Qut_SelectMetafileToOpen(void);
TQ3StorageObject		Qut_SelectMetafileToSaveTo(TQ3FileMode* fileMode);
TQ3Status				Qut_SelectPictureFile(void *theFile, TQ3Uns32 fileLen);
void					Qut_CreateMenu(qutFuncAppMenuSelect appMenuSelect);
void					Qut_CreateMenuItem(TQ3Uns32 itemNum, const char *itemText);





//=============================================================================
//		C++ postamble
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif


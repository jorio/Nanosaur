/*  NAME:
        QutInternal.h

    DESCRIPTION:
        Internal header for Qut.

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
#ifndef QUT_INTERNAL_HDR
#define QUT_INTERNAL_HDR
//=============================================================================
//      Include files
//-----------------------------------------------------------------------------
#include "Qut.h"





//=============================================================================
//		C++ preamble
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif





//=============================================================================
//      Constants
//-----------------------------------------------------------------------------
#define kRendererMaxNum									50

#define kStyleCmdShaderNull								1
#define kStyleCmdShaderLambert							2
#define kStyleCmdShaderPhong							3
#define kStyleCmdDummy1									4
#define kStyleCmdFillFilled								5
#define kStyleCmdFillEdges								6
#define kStyleCmdFillPoints								7
#define kStyleCmdDummy2									8
#define kStyleCmdBackfacingBoth							9
#define kStyleCmdBackfacingRemove						10
#define kStyleCmdBackfacingRemoveFront					50
#define kStyleCmdBackfacingFlip							11
#define kStyleCmdDummy3									12
#define kStyleCmdInterpolationNone						13
#define kStyleCmdInterpolationVertex					14
#define kStyleCmdInterpolationPixel						15
#define kStyleCmdDummy4									16
#define kStyleCmdOrientationClockwise					17
#define kStyleCmdOrientationCounterClockwise			18
#define kStyleCmdDummy5									19
#define kStyleCmdAntiAliasNone							20
#define kStyleCmdAntiAliasEdges							21
#define kStyleCmdAntiAliasFilled						22
#define kStyleCmdDummy6									23
#define kStyleCmdFogOn									24
#define kStyleCmdFogOff									25
#define kStyleCmdDummy7									26
#define kStyleCmdSubdivisionConstant1					27
#define kStyleCmdSubdivisionConstant2					28
#define kStyleCmdSubdivisionConstant3					29
#define kStyleCmdSubdivisionConstant4					30
#define kStyleCmdSubdivisionWorldSpace1					31
#define kStyleCmdSubdivisionWorldSpace2					32
#define kStyleCmdSubdivisionWorldSpace3					33
#define kStyleCmdSubdivisionScreenSpace1				34
#define kStyleCmdSubdivisionScreenSpace2				35
#define kStyleCmdSubdivisionScreenSpace3				36





//=============================================================================
//      Globals
//-----------------------------------------------------------------------------
extern TQ3ViewObject			gView;
//extern void						*gWindow;
extern float					gFPS;
extern TQ3Boolean				gWindowCanResize;
extern TQ3ObjectType			gRenderers[kRendererMaxNum];

extern qutFuncAppMenuSelect		gAppMenuSelect;
extern qutFuncAppRenderPre		gFuncAppRenderPre;
extern qutFuncAppRender			gFuncAppRender;
extern qutFuncAppRenderPost		gFuncAppRenderPost;
extern qutFuncAppMouseDown		gFuncAppMouseDown;
extern qutFuncAppMouseTrack		gFuncAppMouseTrack;
extern qutFuncAppMouseUp		gFuncAppMouseUp;
extern qutFuncAppIdle			gFuncAppIdle;
extern qutFuncAppRedraw			gFuncAppRedraw;





//=============================================================================
//      Function prototypes
//-----------------------------------------------------------------------------
// Qut - private
void	Qut_Initialise(void);
void	Qut_Terminate(void);
void	Qut_RenderFrame(void);
void	Qut_InvokeStyleCommand(TQ3Int32 theCmd);





//=============================================================================
//		C++ postamble
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif



#endif


//    
//  Filename: atimem.h
//  $Revision:   1.0  $
//
//  Description: ATI Mem interface header file
//
//  Trade secret of ATI Technologies, Inc.
//  Copyright 1997, ATI Technologies, Inc., (unpublished)
//
//  All rights reserved.  This notice is intended as a precaution against
//  inadvertent publication and does not imply publication or any waiver
//  of confidentiality.  The year included in the foregoing notice is the
//  year of creation of the work.
//
//
#ifndef ATIMEM_H
#define ATIMEM_H

/************************************************************************/
/* headers                                                              */
/************************************************************************/
#include <Quickdraw.h>

/************************************************************************/
/* defines                                                              */
/************************************************************************/
#define ATIMEM_NO_INIT		NULL
#define	ATIMEM_NO_BLIT		false
#define ATIMEM_NO_CLIP		NULL

/************************************************************************/
/* typedefs	                                                            */
/************************************************************************/
typedef void (*MemCallBack)( GDHandle hGDevice, unsigned int flag, void *data );

#ifndef MEMMAN_H
typedef enum {
	kAllocOutOfSysMem 		= (1 << 0),
	kAllocOutOfVRAM 		= (1 << 1),
	kAllocPreCompactVRAM 	= (1 << 2),
	kAllocPostCompactVRAM 	= (1 << 3),
	kAllocModeSwitchNoClear = (1 << 4),
	kAllocModeSwitchClear 	= (1 << 5),
	kAllocModeSwitch		= (3 << 4)	  // kAllocModeSwitchNoClear | kAllocModeSwitchClear
} EAllocStat;
#endif

/************************************************************************/
/* prototypes                                                           */
/************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void 	*ATIMem_AllocVRAM(	GDHandle hGDevice, unsigned long width, unsigned long height,
							unsigned long pixDepth, unsigned long *pRowBytes, void *init, 
							unsigned long blit, unsigned long blitLeft, unsigned long blitTop,
							RgnHandle clipRgn );
void	ATIMem_FreeVRAM( GDHandle hGDevice, void *data );
int 	ATIMem_AdjustVRAM(	GDHandle hGDevice, void *data, void *init, unsigned long blit, 
							unsigned long blitLeft, unsigned long blitTop, RgnHandle clipRgn );
int		ATIMem_RegHeapCallback( GDHandle hGDevice, MemCallBack callback, void *data );
void	ATIMem_DeregHeapCallback( GDHandle hGDevice, int callbackIndex );
long 	ATIMem_GetFreeVRAM( GDHandle hGDevice );

#ifdef __cplusplus
}
#endif

#endif

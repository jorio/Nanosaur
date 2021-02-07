/****************************/
/*  		INFOBAR.C		*/
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
#include <QD3DMath.h>
#include "globals.h"
#include "objects.h"
#include "windows_nano.h"
#include "misc.h"
#include "infobar.h"
#include "file.h"
#include "weapons.h"
#include "sprites.h"
#include "objtypes.h"
#include "mobjtypes.h"
#include "sound2.h"
#include "input.h"
#include "terrain.h"
#include "3dmath.h"
#include "enemy.h"
#include "timeportal.h"

#include "GamePatches.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Byte			gCurrentAttackMode;
extern	float			gFramesPerSecond,gFramesPerSecondFrac,gCameraRotY,gCameraRotX,gMyHealth;
extern	WindowPtr				gCoverWindow;
extern	Boolean			gPossibleAttackModes[],gGameOverFlag;
extern	short			gWeaponInventory[];
extern	ObjNode			*gPlayerObj;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	TQ3Matrix4x4	gCameraAdjustMatrix;
extern	TQ3Point3D	gMyCoord;
extern	long	gNumSuperTilesDeep,gNumSuperTilesWide;	
extern	FSSpec		gDataSpec;
extern	TimePortalType	gTimePortalList[];
extern	Boolean			gMuteMusicFlag;
extern	long	gOriginalSystemVolume,gCurrentSystemVolume;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void PrintNumber(unsigned long num, short numDigits, long x, long y);
static void ShowTimeRemaining(void);
static void ShowHealth(void);
static void UpdateInfobarIcon(ObjNode *theNode);
static void MoveCompass(ObjNode *theNode);
static void ShowEggs(void);
static void InitGPSMap(void);
static void MoveGPS(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	HEALTH_METER_X		190
#define	HEALTH_METER_Y		457
#define	HEALTH_METER_WIDTH	210.0f
#define	HEALTH_METER_HEIGHT	9
#define	TIME_REM_X			38
#define	TIME_REM_Y			58
#define	EGG_X				476
#define	EGG_Y				435
#define	WEAPON_ICON_X		21
#define	WEAPON_ICON_Y		133
#define	WEAPON_QUAN_X		71
#define	WEAPON_QUAN_Y		219
#define	SCORE_X				270
#define	SCORE_Y				419
#define	FUEL_X				52
#define	FUEL_Y				327
#define	LIVES_X				51
#define	LIVES_Y				458

#define	HEALTH_METER_COLOR16	(((0x14 << 10) << 16) | (0x14 << 10))

#define	INFOBAR_Z		-15.0f

		/* INFOBAR OBJTYPES */
enum
{
	INFOBAR_ObjType_Quit,
	INFOBAR_ObjType_Resume,
	INFOBAR_ObjType_Compass
};



#define	GPS_MAP_TEXTURE_SIZE	64
#define	GPS_MAP_SIZE			(GPS_MAP_TEXTURE_SIZE-2)
#define	GPS_DISPLAY_SIZE		2.5f


/*********************/
/*    VARIABLES      */
/*********************/

unsigned long 	gInfobarUpdateBits = 0;
unsigned long 	gScore;
short			gNumLives;
float			gFuel;
float			gTimeRemaining;
ObjNode			*gCompassObj = nil;
short			gRecoveredEggs[NUM_EGG_SPECIES];

static GWorldPtr gGPSGWorld = nil,gGPSFullImage = nil;
static	ObjNode 	*gGPSObj;
static	long	gOldGPSCoordX,gOldGPSCoordY;

static float gOldTime;


/**************** INIT MY INVENTORY *********************/

void InitMyInventory(void)
{
short	i;

	gNumLives = 2;
	gScore = 0;
	gFuel = 0; //MAX_FUEL_CAPACITY/6;
	gTimeRemaining = LEVEL_DURATION;
	gOldTime = 1000000000;
	
	for (i=0; i < NUM_EGG_SPECIES; i++)
		gRecoveredEggs[i] = 0;
}


/*************** INIT INFOBAR **********************/
//
// Doesnt init inventories, just the physical infobar itself.
//

void InitInfobar(void)
{
FSSpec	spec;


			/* INIT GPS MAP */

	InitGPSMap();


			/* DRAW INFOBAR */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":images:infobar.pict", &spec);
    DrawPictureToScreen(&spec, 0,0);


			/* MAKE COMPASS */

	gNewObjectDefinition.group = MODEL_GROUP_INFOBAR;
	gNewObjectDefinition.type = INFOBAR_ObjType_Compass;
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = -7;
	gNewObjectDefinition.coord.z = INFOBAR_Z;
	gNewObjectDefinition.flags = STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot = INFOBAR_SLOT;
	gNewObjectDefinition.moveCall = MoveCompass;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 2;
	gCompassObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/* PRIME SCREEN */

	gInfobarUpdateBits = 0xffffffff;
	UpdateInfobar();

}



/******************* UPDATE INFOBAR *********************/

void UpdateInfobar(void)
{
unsigned long	bits;

	bits = gInfobarUpdateBits;

		/* SHOW ATTACK MODE */
	
	if (bits & UPDATE_WEAPONICON)
	{
		DrawSpriteFrameToScreen(SPRITE_GROUP_INFOBAR, gCurrentAttackMode, WEAPON_ICON_X, WEAPON_ICON_Y);
		PrintNumber(gWeaponInventory[gCurrentAttackMode],3, WEAPON_QUAN_X,WEAPON_QUAN_Y);
	}

		/* SHOW SCORE */
		
	if (bits & UPDATE_SCORE)
		PrintNumber(gScore,8, SCORE_X,SCORE_Y);


		/* SHOW FUEL GAUGE */

	if (bits & UPDATE_FUEL)
		DrawSpriteFrameToScreen(SPRITE_GROUP_INFOBAR, INFOBAR_FRAMENUM_FUELGAUGE + (MAX_FUEL_CAPACITY - gFuel),
								 FUEL_X, FUEL_Y);

		/* UPDATE TIMERS */
		
	if (bits & UPDATE_IMPACTTIME)
		ShowTimeRemaining();


		/* UPDATE HEALTH */
		
	if (bits & UPDATE_HEALTH)
		ShowHealth();


		/* UPDATE EGGS */
		
	if (bits & UPDATE_EGGS)
		ShowEggs();
		
		
		/* UPDATE LIVES */
		
	if (bits & UPDATE_LIVES)
		PrintNumber(gNumLives,1, LIVES_X,LIVES_Y);

	gInfobarUpdateBits = 0;
}


/********************* NEXT ATTACK MODE ***********************/

void NextAttackMode(void)
{
short	i;

			/* SCAN FOR NEXT AVAILABLE ATTACK MODE */
			
	i = gCurrentAttackMode;
	
	do
	{
		if (++i >= NUM_ATTACK_MODES)						// see if wrap around
			i = 0;
	
		if (gPossibleAttackModes[i])						// can I do this one?
			break;
			
	}while(i !=	gCurrentAttackMode);


			/* SEE IF IT CHANGED */
			
	if (i != gCurrentAttackMode)
	{
		gCurrentAttackMode = i;
		gInfobarUpdateBits |= UPDATE_WEAPONICON;
		PlayEffect(EFFECT_SELECT);		// play sound
	}

}


/****************** PRINT NUMBER ******************/

static void PrintNumber(unsigned long num, short numDigits, long x, long y)
{
unsigned long digit;
short i;

	for (i = 0; i < numDigits; i++)
	{
		digit = num % 10;				// get digit value
		num /= 10;
		DrawSpriteFrameToScreen(SPRITE_GROUP_INFOBAR, INFOBAR_FRAMENUM_0+digit, x, y);
		x -= 16;
	}

}


/********************** ADD TO SCORE **************************/

void AddToScore(long points)
{
	gInfobarUpdateBits |= UPDATE_SCORE;
	gScore += points;
}



/******************** SHOW TIME REMAINING ***********************/

static void ShowTimeRemaining(void)
{
short	minutes,seconds;

	minutes = (int)gTimeRemaining / 60;				// calc # minutes
	seconds = (int)gTimeRemaining - (minutes * 60);	// calc # seconds
	
	PrintNumber(seconds, 2, TIME_REM_X+38, TIME_REM_Y);
	PrintNumber(minutes, 2, TIME_REM_X, TIME_REM_Y);
}

/********************** SHOW HEALTH **************************/

static void ShowHealth(void)
{
float	health;
long	width;
Rect	r;
static const RGBColor	color = {0x9000,0,0x0800};

			/* GET MY HEALTH */
			
	health = gMyHealth;
	if (health < 0)
		health = 0;


				/* DRAW HEALTH METER */

	width = (long)(health * HEALTH_METER_WIDTH);					// calc pixel width to draw
	r.left = HEALTH_METER_X;
	r.right = r.left + width;
	r.top = HEALTH_METER_Y;
	r.bottom = r.top + HEALTH_METER_HEIGHT;

	SetPort(gCoverWindow);

	RGBForeColor(&color);
	PaintRect(&r);

				/* ERASE TAIL */
					
	ForeColor(blackColor);
	r.left = r.right;
	r.right = HEALTH_METER_X+HEALTH_METER_WIDTH;
	PaintRect(&r);
	
}


/******************** UPDATE INFOBAR ICON *************************/

static void UpdateInfobarIcon(ObjNode *theNode)
{
TQ3Matrix4x4	matrix,matrix2,matrix3;	

			/* APPLY SCALE TO OBJECT */
				
	Q3Matrix4x4_SetScale(&matrix, theNode->Scale.x,		// make scale matrix
							 theNode->Scale.y,			
							 theNode->Scale.z);

			/* APPLY LOCAL ROTATION TO OBJECT */
			
	Q3Matrix4x4_SetRotate_XYZ(&matrix2,theNode->Rot.x,theNode->Rot.y,theNode->Rot.z);
	Q3Matrix4x4_Multiply(&matrix,&matrix2,&matrix3);


		/* TRANSLATE TO DESIRED VIEW-SPACE COORD */

	Q3Matrix4x4_SetTranslate(&matrix2,theNode->Coord.x,theNode->Coord.y,theNode->Coord.z);
	Q3Matrix4x4_Multiply(&matrix3,&matrix2,&matrix);
	
	
			/* TRANSFORM TO WORLD COORDINATES */

	Q3Matrix4x4_Multiply(&matrix,&gCameraAdjustMatrix,&theNode->BaseTransformMatrix);
}


/************************ DO PAUSED ******************************/

void DoPaused(void)
{
ObjNode	*resume,*quit;
Byte	selected = 0;
float	fluc = 0;
Boolean	toggleMusic = !gMuteMusicFlag;

	if (toggleMusic)
		ToggleMusic();								// pause music

	Pomme_PauseAllChannels(true);					// Source port addition: pause all looping channels

			/***************/
			/* MAKE RESUME */
			/***************/
			
	gNewObjectDefinition.group = MODEL_GROUP_INFOBAR;
	gNewObjectDefinition.type = INFOBAR_ObjType_Resume;
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 1.2;
	gNewObjectDefinition.coord.z = INFOBAR_Z;
	gNewObjectDefinition.flags = STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot = INFOBAR_SLOT;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = .8;
	resume = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/***************/
			/* MAKE QUIT   */
			/***************/
			
	gNewObjectDefinition.type = INFOBAR_ObjType_Quit;
	gNewObjectDefinition.coord.y = -3.0;
	quit = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/*************/
			/* MAIN LOOP */
			/*************/
			
	do
	{
		QD3D_CalcFramesPerSecond();					// calc frame rate				
		
				/* SEE IF CHANGE SELECT */
				
		ReadKeyboard();
		if (GetNewKeyState_Real(KEY_UP) && (selected > 0))
		{
			selected = 0;
			PlayEffect(EFFECT_SELECT);
		}
		else
		if (GetNewKeyState_Real(KEY_DOWN) && (selected == 0))
		{
			selected = 1;
			PlayEffect(EFFECT_SELECT);
		}

		if (GetNewKeyState(kKey_Pause))					// ESC does quick un-pause
		{
			selected = 0;
			break;
		}
	
				/* FLUCTUATE SELECTED */
						
		fluc += gFramesPerSecondFrac * 8;
		if (selected == 0)
		{
			resume->Coord.z = INFOBAR_Z + sin(fluc) * 2;
			quit->Coord.z = INFOBAR_Z;
		}
		else
		{
			quit->Coord.z = INFOBAR_Z + sin(fluc) * 2;
			resume->Coord.z = INFOBAR_Z;
		}
		UpdateInfobarIcon(resume);
		UpdateInfobarIcon(quit);
		
		RenderBackdropQuad(BACKDROP_FILL);				// Source port addition: repaint backdrop so it looks
															// correct when user resizes window while paused

		QD3D_DrawScene(gGameViewInfoPtr,DrawTerrain);
		DoSDLMaintenance();
	}
	while(!GetNewKeyState_Real(KEY_SPACE) && !GetNewKeyState_Real(KEY_RETURN));					// see if select

			/* CLEANUP */
			
	DeleteObject(quit);
	DeleteObject(resume);
	
	if (toggleMusic)
		ToggleMusic();										// restart music

	Pomme_PauseAllChannels(false);						// Source port addition: unpause looping channels
	
	if (selected == 1)									// see if want out
	{
		gGameOverFlag = true;
	}
}


/****************** MOVE COMPASS ********************/

static void MoveCompass(ObjNode *theNode)
{
float	rot,x,z;
short	n;

	n = FindClosestPortal();
	if (n >= 0)
	{
			/* ANGLE TO CURRENT ACTIVE TIME PORTAL */
			
		x = gPlayerObj->Coord.x;
		z = gPlayerObj->Coord.z;
		
		rot = CalcYAngleFromPointToPoint(x,z, gTimePortalList[n].coord.x,
										gTimePortalList[n].coord.y);	// calc angle directly at target
		rot -= gPlayerObj->Rot.y;
		theNode->Rot.y = rot;
		theNode->StatusBits &= ~STATUS_BIT_HIDDEN;
		
		UpdateInfobarIcon(theNode);
	}
	else
	{
		theNode->StatusBits |= STATUS_BIT_HIDDEN;				// make invisible
	}

	
}


/********************** SHOW EGGS ************************/

static void ShowEggs(void)
{
short	i;

	for (i=0; i < NUM_EGG_SPECIES; i++)
	{
		if (gRecoveredEggs[i])
			DrawSpriteFrameToScreen(SPRITE_GROUP_INFOBAR, INFOBAR_FRAMENUM_EGGICON+i, EGG_X + (i*29), EGG_Y);
	}
}


/******************* INIT GPS MAP ********************/

static void InitGPSMap(void)
{
Rect					r;
OSErr					myErr;
PicHandle			pict;
GDHandle				oldGD;
GWorldPtr				oldGW;

static TQ3Param2D				uvs[4] = {{0,0},	{1,0},	{1,1},	{0,1}};		// NOTE: flipped V coords wrt original QuickDraw 3D version
static TQ3TriMeshTriangleData	triangles[2] = { {{0,3,1}},  {{1,3,2}} };
static TQ3Point3D				points[4] = { { -GPS_DISPLAY_SIZE,  GPS_DISPLAY_SIZE, 0 },
											  {  GPS_DISPLAY_SIZE,  GPS_DISPLAY_SIZE, 0 },
											  {  GPS_DISPLAY_SIZE, -GPS_DISPLAY_SIZE, 0 },
											  { -GPS_DISPLAY_SIZE, -GPS_DISPLAY_SIZE, 0 } };

			/* NUKE OLD ONE */

	if (gGPSFullImage)
	{
		DisposeGWorld(gGPSFullImage);			
		gGPSFullImage = nil;
	}
	if (gGPSGWorld)
	{
		DisposeGWorld(gGPSGWorld);
		gGPSGWorld = nil;
	}
	// Previous GPS trimesh & texture should have been deleted by ObjNode destructor


			/* CREATE TRIMESH */

	TQ3TriMeshData* mesh = Q3TriMeshData_New(2, 4);
	GAME_ASSERT(mesh);


			/* DRAW FULL-SIZE IMAGE INTO GWORLD */
			
	pict = GetPicture(128);													// load map PICT
	GAME_ASSERT(pict);

	r = (*pict)->picFrame;													// get size of PICT
	myErr = NewGWorld(&gGPSFullImage, 16, &r, 0, 0, 0L);					// make gworld
	GAME_ASSERT(!myErr);


	GetGWorld(&oldGW, &oldGD);								
	SetGWorld(gGPSFullImage, nil);	
	DrawPicture(pict,&gGPSFullImage->portRect);								// draw PICT into GWorld
	SetGWorld (oldGW, oldGD);
	ReleaseResource((Handle)pict);											// free the PICT rez


				/* CREATE THE TEXTURE GWORLD */

	SetRect(&r, 0, 0, GPS_MAP_TEXTURE_SIZE, GPS_MAP_TEXTURE_SIZE);							// set dimensions
	myErr = NewGWorld(&gGPSGWorld, 16, &r, 0, 0, 0L);						// make gworld
	GAME_ASSERT(!myErr);

	SetGWorld(gGPSGWorld, nil);	
	ForeColor(blackColor);
	FrameRect(&r);
	SetGWorld (oldGW, oldGD);

		/* CREATE THE QD3D SHADER OBJECT */

	GLuint textureName = Render_LoadTexture(
			GL_RGB,
			GPS_MAP_TEXTURE_SIZE,
			GPS_MAP_TEXTURE_SIZE,
			GL_BGRA,
			GL_UNSIGNED_INT_8_8_8_8,
			GetPixBaseAddr(GetGWorldPixMap(gGPSGWorld)),
			kRendererTextureFlags_ClampBoth);
	mesh->glTextureName = textureName;
	mesh->hasTexture = true;

		/* BUILD GEOMETRY FOR THIS */

	memcpy(mesh->triangles,		triangles,	sizeof(triangles));
	memcpy(mesh->points,		points,		sizeof(points));
	memcpy(mesh->vertexUVs,		uvs,		sizeof(uvs));

	mesh->bBox.min.x = points[0].x;
	mesh->bBox.min.y = points[3].y;
	mesh->bBox.min.z = points[0].z;
	mesh->bBox.max.x = points[1].x;
	mesh->bBox.max.y = points[0].y;
	mesh->bBox.max.z = points[0].z;
	mesh->bBox.isEmpty = kQ3False;


			/* CREATE OBJECT TO DISPLAY THIS */

	gNewObjectDefinition.genre = DISPLAY_GROUP_GENRE;
	gNewObjectDefinition.coord.x = 8.5;
	gNewObjectDefinition.coord.y = 5;
	gNewObjectDefinition.coord.z = INFOBAR_Z;
	gNewObjectDefinition.flags = STATUS_BIT_DONTCULL|STATUS_BIT_NULLSHADER|STATUS_BIT_HIGHFILTER;
	gNewObjectDefinition.slot = INFOBAR_SLOT;
	gNewObjectDefinition.moveCall = MoveGPS;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1;
	gGPSObj = MakeNewObject(&gNewObjectDefinition);
	CreateBaseGroup(gGPSObj);								// create group object
	AttachGeometryToDisplayGroupObject(gGPSObj, 1, &mesh);
	gGPSObj->OwnsMeshMemory[0] = true;						// let DeleteObject dispose of trimesh memory
	gGPSObj->OwnsMeshTexture[0] = true;						// let DeleteObject dispose of OpenGL texture

	MakeObjectTransparent(gGPSObj,.75);						// make xparent

			/* INIT TRACKING THING */

	gOldGPSCoordX = gOldGPSCoordY = -100000;
}


/****************** MOVE GPS *********************/

static void MoveGPS(ObjNode *theNode)
{
long				x,y,left,right,top,bottom;
Rect				sRect,dRect;
GDHandle				oldGD;
GWorldPtr				oldGW;
Boolean					forceUpdate = false;


					/* SEE IF TOGGLE ON/OFF */

	if (GetNewKeyState(kKey_ToggleGPS))
	{
		gGPSObj->StatusBits ^= STATUS_BIT_HIDDEN;
		if (!(gGPSObj->StatusBits & STATUS_BIT_HIDDEN))		// see if just now made re-visible
			forceUpdate = true;
	}

	if (gGPSObj->StatusBits & STATUS_BIT_HIDDEN)
		return;


		/* SEE IF NEED TO UPDATE POSITION */
		
	x = gMyCoord.x * .005f;
	y = gMyCoord.z * .005f;
	
	if ((x != gOldGPSCoordX) || (y != gOldGPSCoordY) || forceUpdate)
	{
		long	w,h;

				/* COPY VISIBLE SECTION OF GWORLD */
				
		dRect = gGPSGWorld->portRect;									// get dest rect
		dRect.left++;	dRect.right--;
		dRect.top++;	dRect.bottom--;
		w = dRect.right - dRect.left;
		h = dRect.bottom - dRect.top;
					
		sRect.left = (gMyCoord.x * TERRAIN_POLYGON_SIZE_Frac) - (GPS_MAP_SIZE/2);	// get src rect
		sRect.top = (gMyCoord.z * TERRAIN_POLYGON_SIZE_Frac) - (GPS_MAP_SIZE/2);
		sRect.right = sRect.left + w;
		sRect.bottom = sRect.top + h;
		
			
				/* CHECK EDGE CLIP */
				
		left = right = top = bottom = 0;					// assume no edge clipping

		if (sRect.left < 0)
		{
			left = -sRect.left;
			sRect.left = 0;
			dRect.left += left;
		}
		if (sRect.right > gGPSFullImage->portRect.right)
		{
			right = gGPSFullImage->portRect.right - sRect.right;
			sRect.right = gGPSFullImage->portRect.right;
			dRect.right += right;
		}
		if (sRect.top < 0)
		{
			top = -sRect.top;
			sRect.top = 0;
			dRect.top += top;
		}
		if (sRect.bottom > gGPSFullImage->portRect.bottom)
		{
			bottom = gGPSFullImage->portRect.bottom - sRect.bottom;
			sRect.bottom = gGPSFullImage->portRect.bottom;
			dRect.bottom += bottom;
		}


				/* DRAW IT */
				
		DumpGWorldToGWorld(gGPSFullImage, gGPSGWorld, &sRect, &dRect);


				/* ERASE EDGES */

		GetGWorld(&oldGW, &oldGD);								
		SetGWorld(gGPSGWorld, nil);	
				
		BackColor(blackColor);
		if (left > 0)
		{
			SetRect(&dRect,1,1,left+1,gGPSGWorld->portRect.bottom-1);
			EraseRect(&dRect);		
		}
		if (right > 0)
		{
			SetRect(&dRect,gGPSGWorld->portRect.right-1-right,1,gGPSGWorld->portRect.right-1,gGPSGWorld->portRect.bottom-1);
			EraseRect(&dRect);		
		}
		if (top > 0)
		{
			SetRect(&dRect,1,1,gGPSGWorld->portRect.right-1,top+1);
			EraseRect(&dRect);		
		}
		if (bottom > 0)
		{
			SetRect(&dRect,1,gGPSGWorld->portRect.bottom-1-bottom,gGPSGWorld->portRect.right-1,gGPSGWorld->portRect.bottom-1);
			EraseRect(&dRect);		
		}



				/* DRAW CROSSHAIRS */
								
		ForeColor(yellowColor);
		MoveTo(GPS_MAP_SIZE/2, 0);
		LineTo(GPS_MAP_SIZE/2, GPS_MAP_SIZE);
		MoveTo(0,GPS_MAP_SIZE/2);
		LineTo(GPS_MAP_SIZE,GPS_MAP_SIZE/2);
		MoveTo(GPS_MAP_SIZE/2-3, 0);
		LineTo(GPS_MAP_SIZE/2+3, 0);

		SetGWorld (oldGW, oldGD);

				/**********************/
				/* UPDATE THE TEXTURE */
				/**********************/

		glBindTexture(GL_TEXTURE_2D, gGPSObj->MeshList[0]->glTextureName);
		glTexSubImage2D(
				GL_TEXTURE_2D,
				0,
				0,
				0,
				GPS_MAP_TEXTURE_SIZE,
				GPS_MAP_TEXTURE_SIZE,
				GL_BGRA,
				GL_UNSIGNED_INT_8_8_8_8,
				GetPixBaseAddr(GetGWorldPixMap(gGPSGWorld)));
		CHECK_GL_ERROR();


		gOldGPSCoordX = x;
		gOldGPSCoordY = y;
	}
	
	
			/* UPDATE IT IN THE INFOBAR */
			
	theNode->Rot.z = -gPlayerObj->Rot.y;
						
	UpdateInfobarIcon(theNode);
}


/************** DEC ASTEROID TIMER *********************/

void DecAsteroidTimer(void)
{
			/* DEC TIMER & SEE IF DONE */
			
	if (gTimeRemaining <= 0.0f)								// see if done
	{
		gGameOverFlag = true;	
	}
			
	if ((gTimeRemaining -= gFramesPerSecondFrac) <= 0.0f)	// dec timer
		gTimeRemaining = 0;

	
	if (fabs(gOldTime - gTimeRemaining) >= 1.0f)
	{
		gInfobarUpdateBits |= UPDATE_IMPACTTIME;
		gOldTime = gTimeRemaining;

				/* DO 30 SECOND WARNING BEEP */
				
		if (gTimeRemaining < 30.0f)
			PlayEffect_Parms(EFFECT_ALARM,FULL_CHANNEL_VOLUME,kMiddleC-5);
	}	
}


/******************* GET HEALTH **************************/

void GetHealth(float amount)
{
	gMyHealth += amount;
	if (gMyHealth > 1.0f)
		gMyHealth = 1.0f;
	gInfobarUpdateBits |= UPDATE_HEALTH;
}


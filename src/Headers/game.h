#pragma once

#include <SDL.h>
#include <SDL_opengl.h>
#include <Pomme.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <QD3D.h>
#include <QD3DMath.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "globals.h"
#include "sprites.h"
#include "mobjtypes.h"
#include "objtypes.h"

#include "3dmath.h"
#include "3dmf.h"
#include "bones.h"
#include "camera.h"
#include "collision.h"
#include "effects.h"
#include "enemy.h"
#include "environmentmap.h"
#include "file.h"
#include "frustumculling.h"
#include "highscores.h"
#include "infobar.h"
#include "input.h"
#include "items.h"
#include "main.h"
#include "mainmenu.h"
#include "misc.h"
#include "movie.h"
#include "myguy.h"
#include "mytraps.h"
#include "objects.h"
#include "pickups.h"
#include "player_control.h"
#include "qd3d_geometry.h"
#include "renderer.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include "skeletonobj.h"
#include "sound2.h"
#include "structformats.h"
#include "terrain.h"
#include "tga.h"
#include "timeportal.h"
#include "title.h"
#include "triggers.h"
#include "version.h"
#include "weapons.h"
#include "window.h"

extern	Boolean					gDisableAnimSounds;
extern	Boolean					gGameOverFlag;
extern	Boolean					gMuteMusicFlag;
extern	Boolean					gPlayerGotKilledFlag;
extern	Boolean					gPossibleAttackModes[];
extern	Boolean					gResetSong;
extern	Boolean					gSongPlayingFlag;
extern	Boolean					gWonGameFlag;
extern	Byte					gCameraMode;
extern	Byte					gCurrentAttackMode;
extern	Byte					gMyStartAim;
extern	char					gTextInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];
extern	CollisionRec			gCollisionList[];
extern	const KeyBinding		kDefaultKeyBindings[NUM_CONTROL_NEEDS];
extern	float					gCameraDistFromMe;
extern	float					gCameraRotX;
extern	float					gCameraRotY;
extern	float					gCameraViewYAngle;
extern	float					gFramesPerSecond;
extern	float					gFramesPerSecondFrac;
extern	float					gFuel;
extern	float					gMinLavaDist;
extern	float					gMinSteamDist;
extern	float					gMostRecentCharacterFloorY;
extern	float					gMyHealth;
extern	float					gMyHeightOffGround;
extern	float					gMySpeedPuffCounter;
extern	float					gObjectGroupRadiusList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	FSSpec					gDataSpec;
extern	GLuint					gShadowGLTextureName;
extern	int						EXPLODEGEOMETRY_DENOMINATOR;
extern	int						MAX_PTERA;
extern	int						MAX_REX;
extern	int						MAX_SPITTER;
extern	int						MAX_STEGO;
extern	int						MAX_TRICER;
extern	int						PRO_MODE;
extern	KeyControlType			gMyControlBits;
extern	long					gCurrentSuperTileCol;
extern	long					gCurrentSuperTileRow;
extern	long					gCurrentSystemVolume;
extern	long					gMyStartX;
extern	long					gMyStartZ;
extern	long					gNumSuperTilesDeep;
extern	long					gNumSuperTilesWide;
extern	long					gNumTerrainTextureTiles;
extern	long					gPrefsFolderDirID;
extern	long					gScreenXOffset;
extern	long					gScreenYOffset;
extern	long					gTerrainItemDeleteWindow_Far;
extern	long					gTerrainItemDeleteWindow_Left;
extern	long					gTerrainItemDeleteWindow_Near;
extern	long					gTerrainItemDeleteWindow_Right;
extern	long					gTerrainTileDepth;
extern	long					gTerrainTileWidth;
extern	long					gTerrainUnitDepth;
extern	long					gTerrainUnitWidth;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode*				gCurrentNode;
extern	ObjNode*				gFirstNodePtr;
extern	ObjNode*				gInventoryObject;
extern	ObjNode*				gMyTimePortal;
extern	ObjNode*				gPlayerNode[];
extern	ObjNode*				gPlayerObj;
extern	PrefsType				gGamePrefs;
extern	Ptr						gTerrainHeightMapPtrs[];
extern	Ptr						gTerrainPtr;
extern	Ptr						gTileFilePtr;
extern	QD3DSetupOutputType*	gGameViewInfoPtr;
extern	RenderStats				gRenderStats;
extern	SDL_Window*				gSDLWindow;
extern	short					gAmbientEffect;
extern	short					gMainAppRezFile;
extern	short					gNumCollisions;
extern	short					gNumEnemies;
extern	short					gNumItems;
extern	short					gNumLives;
extern	short					gNumObjectsInGroupList[MAX_3DMF_GROUPS];
extern	short					gNumTerrainItems;
extern	short					gPrefsFolderVRefNum;
extern	short					gRecoveredEggs[];
extern	short					gWeaponInventory[];
extern	short 					gLavaSoundChannel;
extern	short 					gSteamSoundChannel;
extern	signed char				gNumEnemyOfKind[];
extern	TerrainItemEntryType*	gMasterItemList;
extern	TerrainItemEntryType**	gTerrainItemLookupTableX;
extern	TileAttribType*			gTileAttributes;
extern	TimePortalType			gTimePortalList[];
extern	TQ3BoundingBox 			gObjectGroupBBoxList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3Matrix4x4			gCameraAdjustMatrix;
extern	TQ3Point3D				gCoord;
extern	TQ3Point3D				gMyCoord;
extern	TQ3TriMeshFlatGroup		gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3Vector3D				gDelta;
extern	TQ3Vector3D				gRecentTerrainNormal;
extern	UInt16					gMyLatestPathTileNum;
extern	UInt16					gMyLatestTileAttribs;
extern	UInt16*					gTileDataPtr;
extern	UInt16**				gTerrainHeightMapLayer;
extern	UInt16**				gTerrainPathLayer;
extern	UInt16**				gTerrainTextureLayer;
extern	UInt32* const			gCoverWindowPixPtr;
extern	unsigned long			gScore;
extern	unsigned long 			gInfobarUpdateBits;
extern	WindowPtr				gCoverWindow;

#ifdef __cplusplus
}
#endif

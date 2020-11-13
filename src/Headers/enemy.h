//
// enemy_skeleton.h
//

#include "terrain.h"

extern const int MAX_ENEMIES;

#define	DEFAULT_ENEMY_COLLISION_CTYPES	(CTYPE_MISC|CTYPE_HURTENEMY|CTYPE_BGROUND|CTYPE_BGROUND2|CTYPE_TRIGGER)


		/* ENEMY KIND */
		
#define	NUM_ENEMY_KINDS	15			// always keep at or more than actual types
enum
{
	ENEMY_KIND_REX,
	ENEMY_KIND_PTERA,
	ENEMY_KIND_STEGO,
	ENEMY_KIND_TRICER,
	ENEMY_KIND_SPITTER
};


			/* ENEMY */
			
extern	ObjNode *MakeEnemySkeleton(Byte skeletonType, float x, float z);
extern	void DeleteEnemy(ObjNode *theEnemy);
extern	Boolean DoEnemyCollisionDetect(ObjNode *theEnemy, unsigned long ctype);
extern	void UpdateEnemy(ObjNode *theNode);
extern	Boolean EnemyGotHurt(ObjNode *theEnemy, ObjNode *theHurter, float damage);
extern	void InitEnemyManager(void);
extern	ObjNode *FindClosestEnemy(TQ3Point3D *pt, float *dist);
extern	Boolean MoveEnemy(ObjNode *theNode, float flightHeight);


			/* REX */
			
extern	Boolean AddEnemy_Rex(TerrainItemEntryType *itemPtr, long x, long z);



			/* PTERANODON */
			
extern	Boolean AddEnemy_Ptera(TerrainItemEntryType *itemPtr, long x, long z);


			/* STEGO */
			
extern	Boolean AddEnemy_Stego(TerrainItemEntryType *itemPtr, long x, long z);

			/* TRICER */
			
extern	ObjNode *MakeTriceratops(ObjNode *theBush, long x, long z);
extern	Boolean AddEnemy_Tricer(TerrainItemEntryType *itemPtr, long x, long z);

			/* SPITTER */
			
extern	Boolean AddEnemy_Spitter(TerrainItemEntryType *itemPtr, long x, long z);


/****************************/
/*   	EFFECTS.C		    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveDustPuff(ObjNode *theNode);
static void MoveSmokePuff(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

/*********************/
/*    VARIABLES      */
/*********************/



/************************* MAKE DUST PUFF *********************************/

ObjNode *MakeDustPuff(float x, float y, float z, float startScale)
{
ObjNode	*newObj;

	if (!gGamePrefs.dust)
		return(nil);

	gNewObjectDefinition.group = GLOBAL_MGroupNum_Dust;	
	gNewObjectDefinition.type = GLOBAL_MObjType_Dust;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = y;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall = MoveDustPuff;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = startScale + RandomFloat()*.01f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(nil);

	newObj->Rot.x = RandomFloat()*PI2;
	newObj->Rot.y = RandomFloat()*PI2;
	newObj->Rot.z = RandomFloat()*PI2;

	newObj->Health = .8;									// transparency value
	newObj->SpecialF[0] = (RandomFloat()*.8) + 1;			// random decay rate
	
	newObj->Delta.x = (RandomFloat()-.5) * 60;				// random drift
	newObj->Delta.z = (RandomFloat()-.5) * 60;
	newObj->Delta.y = RandomFloat() * 40;

	MakeObjectTransparent(newObj, newObj->Health);
	
	return(newObj);
}



/******************** MOVE DUST PUFF ************************/

static void MoveDustPuff(ObjNode *theNode)
{
	theNode->Health -= theNode->SpecialF[0] * gFramesPerSecondFrac;
	if (theNode->Health < 0)
	{
		DeleteObject(theNode);
		return;
	}
	
	MakeObjectTransparent(theNode, theNode->Health);

	theNode->Scale.x += gFramesPerSecondFrac;
	theNode->Scale.y += gFramesPerSecondFrac;
	theNode->Scale.z += gFramesPerSecondFrac;

	theNode->Coord.x += theNode->Delta.x * gFramesPerSecondFrac;
	theNode->Coord.y += theNode->Delta.y * gFramesPerSecondFrac;
	theNode->Coord.z += theNode->Delta.z * gFramesPerSecondFrac;

	theNode->Rot.x += gFramesPerSecondFrac * PI2;

	UpdateObjectTransforms(theNode);
}

/************************* MAKE SMOKE PUFF *********************************/

ObjNode *MakeSmokePuff(float x, float y, float z, float startScale)
{
ObjNode	*newObj;

	gNewObjectDefinition.group = GLOBAL_MGroupNum_Smoke;	
	gNewObjectDefinition.type = GLOBAL_MObjType_Smoke;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = y;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = STATUS_BIT_NOTRICACHE;
	gNewObjectDefinition.slot = SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall = MoveSmokePuff;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = startScale + RandomFloat()*.01f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(nil);

	newObj->Rot.x = RandomFloat()*PI2;
	newObj->Rot.y = RandomFloat()*PI2;
	newObj->Rot.z = RandomFloat()*PI2;

	newObj->Health = .8;									// transparency value
	newObj->SpecialF[0] = (RandomFloat()*.3f) + .2f;		// random decay rate
	
	newObj->Delta.x = (RandomFloat()-.5f) * 20.0f;			// random drift
	newObj->Delta.z = (RandomFloat()-.5f) * 20.0f;
	newObj->Delta.y = (RandomFloat() * 30.0f) + 30.0f;

	MakeObjectTransparent(newObj, newObj->Health);
	
	return(newObj);
}



/******************** MOVE SMOKE PUFF ************************/

static void MoveSmokePuff(ObjNode *theNode)
{
float	t,fps = gFramesPerSecondFrac;

	theNode->Health -= theNode->SpecialF[0] * fps;
	if (theNode->Health < 0)
	{
		DeleteObject(theNode);
		return;
	}
	
	t = 3 * theNode->Health;
	if (t > 1)
		t = 1;
	
	MakeObjectTransparent(theNode, t);

	theNode->Scale.x += fps * .5;
	theNode->Scale.y += fps * .5;
	theNode->Scale.z += fps * .5;

	theNode->Coord.x += theNode->Delta.x * fps;
	theNode->Coord.y += theNode->Delta.y * fps;
	theNode->Coord.z += theNode->Delta.z * fps;

	theNode->Rot.x += gFramesPerSecondFrac * PI;

	UpdateObjectTransforms(theNode);
}








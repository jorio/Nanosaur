//
// mobjtypes.h
//

enum
{
	MODEL_GROUP_LEVEL0		=	0,
	MODEL_GROUP_TITLE		=	1,
	MODEL_GROUP_MENU		=	1,
	MODEL_GROUP_HIGHSCORES 	=	2,
	MODEL_GROUP_GLOBAL		=	2,
	MODEL_GROUP_INFOBAR		=	3,
	MODEL_GROUP_LIMBS 		=	5		// NOTE!!! This MUST BE last in list b/c it is just the BASE of several groups!!!
};



/******************* TITLE *************************/

enum
{
	TITLE_MObjType_GameName,
	TITLE_MObjType_Pangea,
	TITLE_MObjType_Background
};


/******************* LEVEL 0 *************************/

enum
{
	LEVEL0_MObjType_BonusBox,
	LEVEL0_MObjType_LavaPatch,
	LEVEL0_MObjType_WaterPatch,
	LEVEL0_MObjType_Egg1,
	LEVEL0_MObjType_Egg2,
	LEVEL0_MObjType_Egg3,
	LEVEL0_MObjType_Egg4,
	LEVEL0_MObjType_Egg5,
	LEVEL0_MObjType_Boulder,
	LEVEL0_MObjType_Boulder2,
	LEVEL0_MObjType_Mushroom,
	LEVEL0_MObjType_Bush,
	LEVEL0_MObjType_Crystal1,
	LEVEL0_MObjType_Crystal2,
	LEVEL0_MObjType_Crystal3,
	LEVEL0_MObjType_Nest,
	LEVEL0_MObjType_Tree1,
	LEVEL0_MObjType_Tree2,
	LEVEL0_MObjType_Tree3,
	LEVEL0_MObjType_Tree4,
	LEVEL0_MObjType_Tree5,
	LEVEL0_MObjType_Tree6,
	LEVEL0_MObjType_GasVent,
	LEVEL0_MObjType_StepStone,
	LEVEL0_MObjType_Pod,
	LEVEL0_MObjType_Spore,
	LEVEL0_MObjType_Fireball
};

enum
{
	LEVEL0_MGroupNum_BonusBox = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_LavaPatch = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_WaterPatch = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Egg = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Boulder = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Boulder2 = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Mushroom = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Bush = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Crystal1 = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Crystal2 = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Crystal3 = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Nest = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Tree = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_GasVent = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_StepStone = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Pod = MODEL_GROUP_LEVEL0,
	LEVEL0_MGroupNum_Fireball = MODEL_GROUP_LEVEL0
};



/******************* GLOBAL *************************/

enum
{
	GLOBAL_MObjType_JetFlame,
	GLOBAL_MObjType_Shadow,
	GLOBAL_MObjType_Dust,
	GLOBAL_MObjType_Smoke,
	GLOBAL_MObjType_DinoSpit,
	GLOBAL_MObjType_SonicScream,
	GLOBAL_MObjType_Blaster,
	GLOBAL_MObjType_HeatSeek,
	GLOBAL_MObjType_HeatSeekEcho,
	GLOBAL_MObjType_Explosion,
	GLOBAL_MObjType_TimePortalRing,
	GLOBAL_MObjType_HeatSeekPOW,
	GLOBAL_MObjType_LaserPOW,
	GLOBAL_MObjType_TriBlast,
	GLOBAL_MObjType_TriBlastPOW,
	GLOBAL_MObjType_HealthPOW,
	GLOBAL_MObjType_ShieldPOW,
	GLOBAL_MObjType_NukePOW,
	GLOBAL_MObjType_Sonic,
	GLOBAL_MObjType_Shield,
	GLOBAL_MObjType_Nuke
};

enum
{
	GLOBAL_MGroupNum_JetFlame = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_Shadow = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_Dust = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_Smoke = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_DinoSpit = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_SonicScream = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_Blaster = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_HeatSeek = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_HeatSeekEcho = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_Explosion = MODEL_GROUP_GLOBAL,	
	GLOBAL_MGroupNum_TimePortalRing = MODEL_GROUP_GLOBAL,	
	GLOBAL_MGroupNum_HeatSeekPOW = MODEL_GROUP_GLOBAL,	
	GLOBAL_MGroupNum_LaserPOW = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_TriBlast = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_TriBlastPOW = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_HealthPOW = MODEL_GROUP_GLOBAL,	
	GLOBAL_MGroupNum_ShieldPOW = MODEL_GROUP_GLOBAL,	
	GLOBAL_MGroupNum_NukePOW = MODEL_GROUP_GLOBAL,	
	GLOBAL_MGroupNum_Shield = MODEL_GROUP_GLOBAL,
	GLOBAL_MGroupNum_Nuke = MODEL_GROUP_GLOBAL	
};








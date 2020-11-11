//
// myattack.h
//


#define	NUM_ATTACK_MODES	5

enum
{
	ATTACK_MODE_BLASTER = 0,
	ATTACK_MODE_HEATSEEK,
	ATTACK_MODE_SONICSCREAM,
	ATTACK_MODE_TRIBLAST,
	ATTACK_MODE_NUKE
};



//====================================



extern void CheckIfMeAttack(ObjNode *theNode);
extern	void InitWeaponManager(void);
extern	void GetWeaponPowerUp(short kind, short quantity);
extern	void GetCheatWeapons(void);







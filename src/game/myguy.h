//
// myguy.h
//

#define	DIST_FROM_ORIGIN_TO_FEET	54	//58


#define	KickIsLethal	Flag[0]					// flag set during kick anim when kick can do damage
#define	PickUpNow		Flag[0]					// set during pickup when can check for pickups
#define	ThrowNow		Flag[0]					// at moment when should throw item

#define	HurtTimer		SpecialF[0]				// timer for duration of hurting
#define	InvincibleTimer	SpecialF[4]				// timer for invicibility after being hurt
#define	JetThrust		SpecialF[3]				// how much thrust is jetpack pushing?

#define	ExitTimer		SpecialF[0]

#define	JetFlameObj		Special[4]				// ptr to JetFlame's objnode (which also links to 2nd jet flame)

		/* ANIMATION NUMBERS */
		
enum
{
	PLAYER_ANIM_STAND = 0,
	PLAYER_ANIM_WALK,
	PLAYER_ANIM_JUMP2,
	PLAYER_ANIM_LANDING,
	PLAYER_ANIM_BITE,
	PLAYER_ANIM_TURNLEFT,
	PLAYER_ANIM_TURNRIGHT,
	PLAYER_ANIM_PICKUP,
	PLAYER_ANIM_THROW,
	PLAYER_ANIM_GETHURT,
	PLAYER_ANIM_JETNEUTRAL,
	PLAYER_ANIM_JUMP1,
	PLAYER_ANIM_FALLING,
	PLAYER_ANIM_DEATH,
	PLAYER_ANIM_EXIT
};


		/* LIMB NUMBERS */
enum
{
	MYGUY_LIMB_BODY = 0,
	MYGUY_LIMB_HEAD	= 7,
	MYGUY_LIMB_JAW = 14,
	MYGUY_LIMB_GUN = 16
};



#define	PLAYER_COLLISION_CTYPE	(CTYPE_MISC|CTYPE_ENEMY|CTYPE_HURTME|CTYPE_TRIGGER|CTYPE_BGROUND)



//=======================================================

extern	void InitMyGuy(void);
extern	void StartMyShield(ObjNode *theNode);
extern	void PlayerGotHurt(ObjNode *theNode, float damage, Boolean doHurtAnim, Boolean overrideShield);
extern	void ResetPlayer(void);









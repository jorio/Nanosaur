//
// infobar.h
//

#define INFOBAR_HEIGHT	480

#define	MAX_FUEL_CAPACITY	(29-1)
#define	NUM_EGG_SPECIES		5
#define LEVEL_DURATION		(60*20)			// n seconds

enum
{
	UPDATE_WEAPONICON = (1),
	UPDATE_SCORE = (1<<1),
	UPDATE_FUEL = (1<<2),
	UPDATE_IMPACTTIME = (1<<3),
	UPDATE_HEALTH = (1<<4),
	UPDATE_EGGS = (1<<5),
	UPDATE_LIVES = (1<<6)
};

#define	EGG_POINTS		20000
#define	EGG_POINTS2		5000
#define	EGG_POINTS3		150000


extern	void InitInfobar(void);
extern	void UpdateInfobar(void);
extern	void NextAttackMode(void);
extern	void InitMyInventory(void);
extern	void AddToScore(long points);
extern	void DoPaused(void);
extern	void DecAsteroidTimer(void);
extern	void GetHealth(float amount);


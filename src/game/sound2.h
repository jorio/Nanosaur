//
// Sound2.h
//

#define		BASE_EFFECT_RESOURCE	10000

#define		FULL_CHANNEL_VOLUME		255
#define		NORMAL_CHANNEL_RATE		0x10000


enum
{
	SOUND_BANK_DEFAULT,
	SOUND_BANK_MENU
};



/***************** EFFECTS *************************/
// This table must match gEffectsTable
//

enum
{
	EFFECT_HEATSEEK,
	EFFECT_BLASTER,
	EFFECT_SELECT,
	EFFECT_EXPLODE,
	EFFECT_POWPICKUP,
	EFFECT_CRUNCH,
	EFFECT_ALARM,
	EFFECT_ENEMYDIE,
	EFFECT_JETLOOP,
	EFFECT_JUMP,
	EFFECT_ROAR,
	EFFECT_FOOTSTEP,
	EFFECT_DILOATTACK,
	EFFECT_WINGFLAP,
	EFFECT_PORTAL,
	EFFECT_BUBBLES,
	EFFECT_CRYSTAL,
	EFFECT_STEAM,
	EFFECT_ROCKSLAM,
	EFFECT_AMBIENT,
	EFFECT_SHIELD,
	EFFECT_SONIC,
	
	EFFECT_MENUCHANGE
};



/**********************/
/* SOUND BANK TABLES  */
/**********************/
//
// These are simple enums for equating the sound effect #'s in each sound
// bank's rez file.
//

/******** SoundBank_Default *********/

enum
{
	SOUND_DEFAULT_HEATSEEK,
	SOUND_DEFAULT_BLASTER,
	SOUND_DEFAULT_SELECT,
	SOUND_DEFAULT_EXPLODE,
	SOUND_DEFAULT_POWPICKUP,
	SOUND_DEFAULT_CRUNCH,
	SOUND_DEFAULT_ALARM,
	SOUND_DEFAULT_ENEMYDIE,
	SOUND_DEFAULT_JETLOOP,
	SOUND_DEFAULT_JUMP,
	SOUND_DEFAULT_ROAR,
	SOUND_DEFAULT_FOOTSTEP,
	SOUND_DEFAULT_DILOATTACK,
	SOUND_DEFAULT_WINGFLAP,
	SOUND_DEFAULT_PORTAL,
	SOUND_DEFAULT_BUBBLES,
	SOUND_DEFAULT_CRYSTAL,
	SOUND_DEFAULT_STEAM,
	SOUND_DEFAULT_ROCKSLAM,
	SOUND_DEFAULT_AMBIENT,
	SOUND_DEFAULT_SHIELD,
	SOUND_DEFAULT_SONIC
};


/******** SoundBank_Menu *********/

enum
{
	SOUND_MENU_CHANGE
};




//===================== PROTOTYPES ===================================


extern void	InitSoundTools(void);
void StopAChannel(short *channelNum);
extern	void StopAllEffectChannels(void);
void PlaySong(short songNum, Boolean loopFlag);
extern void	KillSong(void);
extern	short PlayEffect(short effectNum);
extern void	ToggleMusic(void);
extern void	ToggleEffects(void);
extern void	DoSoundMaintenance(void);
extern	void LoadSoundBank(FSSpec *spec, long bankNum);
extern	void WaitEffectsSilent(void);
extern	void DisposeSoundBank(short bankNum);
extern	void StreamSampleFromDisk(Str255 filename, Boolean stopExisting);
extern	void StopCurrentStreamingSample(void);
extern	void LoadSkeletonSoundBank(Byte skeletonType);
extern	void FreeSkeletonSoundBank(Byte skeletonType);
extern	void DisposeAllSoundBanks(void);
extern	void ChangeChannelFrequency(short channel, long freq);
extern	short PlayEffect_Parms(short effectNum, unsigned char volume, unsigned long freq);
extern	void ChangeChannelVolume(short channel, short volume);
extern	void StartAmbientEffect(void);










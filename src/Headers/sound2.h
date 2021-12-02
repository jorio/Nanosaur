//
// Sound2.h
//

#define		FULL_CHANNEL_VOLUME		255
#define		NORMAL_CHANNEL_RATE		0x10000



/***************** EFFECTS *************************/

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
	EFFECT_MENUCHANGE,
	NUM_EFFECTS
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
extern	void LoadSoundBank(void);
extern	void WaitEffectsSilent(void);
extern	void DisposeSoundBank(void);
extern	void ChangeChannelFrequency(short channel, long freq);
extern	short PlayEffect_Parms(short effectNum, unsigned char volume, unsigned long freq);
extern	void ChangeChannelVolume(short channel, short volume);
extern	void StartAmbientEffect(void);










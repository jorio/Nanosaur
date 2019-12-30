/****************************/
/*     SOUND ROUTINES       */
/* (c)1994-98 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/
#include "globals.h"
#include "misc.h"
#include "sound2.h"
#include "file.h"
#include "input.h"
#include "skeletonobj.h"

extern	short		gMainAppRezFile;
extern	short 	gLavaSoundChannel;
extern	float	gMinLavaDist;
extern	short 	gSteamSoundChannel;
extern	float	gMinSteamDist;

/****************************/
/*    PROTOTYPES            */
/****************************/

static short FindSilentChannel(void);
static void SongCompletionProc(SndChannelPtr chan);
static short EmergencyFreeChannel(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#ifdef PRO_MODE
#define		MAX_CHANNELS			14
#else
#define		MAX_CHANNELS			11
#endif

#define		MAX_SOUND_BANKS			5
#define		MAX_EFFECTS				30


typedef struct
{
	Byte	bank,sound;
}EffectType;


#define	STREAM_BUFFER_SIZE	100000

typedef struct
{
	UInt16	effectNum;
	float	volumeAdjust;
}ChannelInfoType;


/**********************/
/*     VARIABLES      */
/**********************/

static	SndListHandle		gSndHandles[MAX_SOUND_BANKS][MAX_EFFECTS];		// handles to ALL sounds
static  long				gSndOffsets[MAX_SOUND_BANKS][MAX_EFFECTS];

static	SndChannelPtr	gSndChannel[MAX_CHANNELS];
static	SndChannelPtr	gMusicChannel=nil;
static short			gMaxChannels = 0;

static short			gCurrentMusicChannel = -1;
static short			gMusicFileRefNum = 0x0ded;

static	ChannelInfoType		gChannelInfo[MAX_CHANNELS];
static	short			gNumSndsInBank[MAX_SOUND_BANKS] = {0,0,0,0,0};

static short				gCurrentSong = -1;

Boolean				gSongPlayingFlag = false;
Boolean				gResetSong = false;
Boolean				gLoopSongFlag = true;

static	short			gStatusBits[MAX_CHANNELS];				// set in maintainsounds (for debugging)

long	gOriginalSystemVolume,gCurrentSystemVolume;

Boolean			gMuteMusicFlag = false;

static Ptr				gMusicBuffer = nil;					// buffers to use for streaming play

		/*****************/
		/* EFFECTS TABLE */
		/*****************/
		
static EffectType	gEffectsTable[] =
{
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_HEATSEEK,					// EFFECT_HEATSEEK
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_BLASTER,					// EFFECT_BLASTER
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_SELECT,					// EFFECT_SELECT
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_EXPLODE,					// EFFECT_EXPLODE
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_POWPICKUP,					// EFFECT_POWPICKUP
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_CRUNCH,					// EFFECT_CRUNCH
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_ALARM,						// EFFECT_ALARM
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_ENEMYDIE,					// EFFECT_ENEMYDIE
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_JETLOOP,					// EFFECT_JETLOOP
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_JUMP,						// EFFECT_JUMP
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_ROAR,						// EFFECT_ROAR
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_FOOTSTEP,					// EFFECT_FOOTSTEP
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_DILOATTACK,				// EFFECT_DILOATTACK
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_WINGFLAP,					// EFFECT_WINGFLAP
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_PORTAL,					// EFFECT_PORTAL
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_BUBBLES,					// EFFECT_BUBBLES
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_CRYSTAL,					// EFFECT_CRYSTAL
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_STEAM,						// EFFECT_STEAM
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_ROCKSLAM,					// EFFECT_ROCKSLAM
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_AMBIENT,					// EFFECT_AMBIENT
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_SHIELD,					// EFFECT_SHIELD
	SOUND_BANK_DEFAULT,SOUND_DEFAULT_SONIC,						// EFFECT_SONIC

	SOUND_BANK_MENU,SOUND_MENU_CHANGE							// EFFECT_MENUCHANGE
};

short	gAmbientEffect = -1;


/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
OSErr		iErr;
short		i;

			/* SET SYSTEM VOLUME INFO */
			
	GetDefaultOutputVolume(&gOriginalSystemVolume);		
	gOriginalSystemVolume &= 0xffff;	
	gCurrentSystemVolume = gOriginalSystemVolume;


	gMaxChannels = 0;

			/* INIT BANK INFO */
			
	for (i = 0; i < MAX_SOUND_BANKS; i++)
		gNumSndsInBank[i] = 0;

			/******************/
			/* ALLOC CHANNELS */
			/******************/

			/* MAKE MUSIC CHANNEL */
			
	SndNewChannel(&gMusicChannel,sampledSynth,initStereo,nil);


			/* ALL OTHER CHANNELS */
				
	for (gMaxChannels = 0; gMaxChannels < MAX_CHANNELS; gMaxChannels++)
	{
			/* NEW SOUND CHANNEL */
			
		iErr = SndNewChannel(&gSndChannel[gMaxChannels],sampledSynth,initMono+initNoInterp,nil);
		if (iErr)												// if err, stop allocating channels
			break;
			
	}
	

		/* INIT MUSIC STREAMING BUFFER */
		
	if (gMusicBuffer == nil)
	{
		gMusicBuffer = AllocPtr(STREAM_BUFFER_SIZE);
		if (gMusicBuffer == nil)
			DoFatalAlert("InitSoundTools: gMusicBuffer == nil");
	}
}


/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(FSSpec *spec, long bankNum)
{
short			srcFile1,numSoundsInBank,i;
Str255			error = "Couldnt Open Sound Resource File.";
OSErr			iErr;

	StopAllEffectChannels();

	if (bankNum >= MAX_SOUND_BANKS)
		DoFatalAlert("LoadSoundBank: bankNum >= MAX_SOUND_BANKS");

			/* DISPOSE OF EXISTING BANK */
			
	DisposeSoundBank(bankNum);


			/* OPEN APPROPRIATE REZ FILE */
			
	srcFile1 = FSpOpenResFile(spec, fsRdPerm);
	if (srcFile1 == -1)
		DoFatalAlert("LoadSoundBank: OpenResFile failed!");


			/* LOAD ALL EFFECTS IN BANK */

	UseResFile( srcFile1 );												// open sound resource fork
	numSoundsInBank = Count1Resources('snd ');							// count # snd's in this bank
	if (numSoundsInBank > MAX_EFFECTS)
		DoFatalAlert("LoadSoundBank: numSoundsInBank > MAX_EFFECTS");

	for (i=0; i < numSoundsInBank; i++)
	{
		gSndHandles[bankNum][i] = (SndListResource **)GetResource('snd ',BASE_EFFECT_RESOURCE+i);
		if (gSndHandles[bankNum][i] == nil) 
		{
			iErr = ResError();
			DoAlert("LoadSoundBank: GetResource failed!");
			if (iErr == memFullErr)
				DoFatalAlert("LoadSoundBank: Out of Memory");		
			else
				ShowSystemErr(iErr);
		}
		DetachResource((Handle)gSndHandles[bankNum][i]);				// detach resource from rez file & make a normal Handle
		if ( iErr = ResError() ) 
			ShowSystemErr(iErr);
						
		HNoPurge((Handle)gSndHandles[bankNum][i]);						// make non-purgeable
		HLockHi((Handle)gSndHandles[bankNum][i]);

				/* GET OFFSET INTO IT */
				
		GetSoundHeaderOffset(gSndHandles[bankNum][i], &gSndOffsets[bankNum][i]);		
	}

	UseResFile(gMainAppRezFile );								// go back to normal res file
	CloseResFile(srcFile1);

	gNumSndsInBank[bankNum] = numSoundsInBank;					// remember how many sounds we've got
}


/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(short bankNum)
{
short	i; 

	if (bankNum > MAX_SOUND_BANKS)
		return;

	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

			/* FREE ALL SAMPLES */
			
	for (i=0; i < gNumSndsInBank[bankNum]; i++)
		DisposeHandle((Handle)gSndHandles[bankNum][i]);


	gNumSndsInBank[bankNum] = 0;
}


/******************* DISPOSE ALL SOUND BANKS *****************/

void DisposeAllSoundBanks(void)
{
short	i;

	for (i = 0; i < MAX_SOUND_BANKS; i++)
	{
		DisposeSoundBank(i);
	}
}

/********************* STOP A CHANNEL **********************/
//
// Stops the indicated sound channel from playing.
//

void StopAChannel(short *channelNum)
{
SndCommand 	mySndCmd;
OSErr 		myErr;
SCStatus	theStatus;
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))		// make sure its a legal #
		return;

	myErr = SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
	if (theStatus.scChannelBusy)					// if channel busy, then stop it
	{

		mySndCmd.cmd = flushCmd;	
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);

		mySndCmd.cmd = quietCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);
	}
	
	*channelNum = -1;
}


/********************* STOP ALL EFFECT CHANNELS **********************/

void StopAllEffectChannels(void)
{
short		i;

	for (i=0; i < gMaxChannels; i++)
	{
		short	c;
		
		c = i;
		StopAChannel(&c);
	}
}


/****************** WAIT EFFECTS SILENT *********************/

void WaitEffectsSilent(void)
{
short	i;
Boolean	isBusy;
SCStatus				theStatus;

	do
	{
		isBusy = 0;
		for (i=0; i < gMaxChannels; i++)
		{
			if (i==gCurrentMusicChannel)									// skip this channel if it's music
				continue;

			SndChannelStatus(gSndChannel[i],sizeof(SCStatus),&theStatus);	// get channel info
			isBusy |= theStatus.scChannelBusy;
		}
	}while(isBusy);
}


/******************** PLAY SONG ***********************/
//
// if songNum == -1, then play existing open song
//
// INPUT: loopFlag = true if want song to loop
//

void PlaySong(short songNum, Boolean loopFlag)
{
OSErr 	iErr;
Str255	errStr = "PlaySong: Couldnt Open Music AIFF File.";
static	SndCommand 		mySndCmd;

	if (songNum == gCurrentSong)					// see if this is already playing
		return;

		/* SEE IF JUST RESTART CURRENT STREAM */
		
	if (songNum == -1)	
		goto stream_again;

		/* ZAP ANY EXISTING SONG */
		
	gCurrentSong 	= songNum;
	gResetSong 		= false;
	gLoopSongFlag 	= loopFlag;
	KillSong();
	DoSoundMaintenance();

			/******************************/
			/* OPEN APPROPRIATE AIFF FILE */
			/******************************/
			
	switch(songNum)
	{
		case	0:
				OpenGameFile(":audio:gamesong.aiff",&gMusicFileRefNum,errStr);		
				break;

		case	1:
				OpenGameFile(":audio:titlesong.aiff",&gMusicFileRefNum,errStr);		
				break;

		case	2:
				OpenGameFile(":audio:song_pangea",&gMusicFileRefNum,errStr);		
				break;
	

		default:
				DoFatalAlert("PlaySong: unknown song #");
	}

	gCurrentSong = songNum;
	
	
				/*******************/
				/* START STREAMING */
				/*******************/
		
		/* RESET CHANNEL TO DEFAULT VALUES */
			
	mySndCmd.cmd 	= ampCmd;										// set sound playback volume
	mySndCmd.param1 = FULL_CHANNEL_VOLUME;
	mySndCmd.param2 = 0;
	iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
	if (iErr)
	    DoFatalAlert("PlaySong: SndDoImmediate failed");
		
	mySndCmd.cmd 	= rateCmd;										// set playback rate
	mySndCmd.param1 = 0;
	mySndCmd.param2 = NORMAL_CHANNEL_RATE;	
	iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
	if (iErr)
	    DoFatalAlert("PlaySong: SndDoImmediate failed");
			
			
			/* START PLAYING FROM FILE */

stream_again:					
	iErr = SndStartFilePlay(gMusicChannel, gMusicFileRefNum, 0, STREAM_BUFFER_SIZE, gMusicBuffer,
							nil, NewFilePlayCompletionProc(SongCompletionProc), true);
	if (iErr)
	{
		FSClose(gMusicFileRefNum);								// close the file
		gMusicFileRefNum = 0x0ded;
		DoAlert("PlaySong: SndStartFilePlay failed!");
		ShowSystemErr(iErr);
	}
	gSongPlayingFlag = true;

	
			/* SEE IF WANT TO MUTE THE MUSIC */
			
	if (gMuteMusicFlag)													
		SndPauseFilePlay(gMusicChannel);						// pause it	
}


/***************** SONG COMPLETION PROC *********************/

static void SongCompletionProc(SndChannelPtr chan)
{
	chan;
	
	if (gSongPlayingFlag)
		gResetSong = true;
}


/*********************** KILL SONG *********************/

void KillSong(void)
{
OSErr	iErr;

	gCurrentSong = -1;

	if (!gSongPlayingFlag)
		return;
		
	gSongPlayingFlag = false;											// tell callback to do nothing

	SndStopFilePlay(gMusicChannel, true);								// stop it
	
	if (gMusicFileRefNum == 0x0ded)
		DoAlert("KillSong: gMusicFileRefNum == 0x0ded");
	else
	{
		iErr = FSClose(gMusicFileRefNum);								// close the file
		if (iErr)
		{
			DoAlert("KillSong: FSClose failed!");
			ShowSystemErr_NonFatal(iErr);
		}
	}
		
	gMusicFileRefNum = 0x0ded;
}


/***************************** PLAY EFFECT ***************************/
//
// OUTPUT: channel # used to play sound
//

short PlayEffect(short effectNum)
{
	return(PlayEffect_Parms(effectNum,FULL_CHANNEL_VOLUME,kMiddleC));

}


/***************************** PLAY EFFECT PARMS ***************************/
//
// Plays an effect with parameters
//
// OUTPUT: channel # used to play sound
//

short PlayEffect_Parms(short effectNum, unsigned char volume, unsigned long freq)
{
static	SndCommand 		mySndCmd;
static 	OSErr			iErr;
static	SndChannelPtr	chanPtr;
short					theChan;
Byte					bankNum,soundNum;
OSErr	myErr;
	
			/* GET BANK & SOUND #'S FROM TABLE */
			
	bankNum = gEffectsTable[effectNum].bank;
	soundNum = gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);	
	}

			/* LOOK FOR FREE CHANNEL */
			
	theChan = FindSilentChannel();
	if (theChan == -1)
	{
		return(-1);
	}

					/* GET IT GOING */

	chanPtr = gSndChannel[theChan];						
	
	mySndCmd.cmd = flushCmd;	
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = quietCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = soundCmd;											// install sample in the channel
	mySndCmd.param1 = 0;
	mySndCmd.param2 = ((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);


	mySndCmd.cmd = ampCmd;								// set sound playback volume
	mySndCmd.param1 = volume;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);

	mySndCmd.cmd = rateCmd;								// set playback rate
	mySndCmd.param1 = 0;
	mySndCmd.param2 = NORMAL_CHANNEL_RATE;	
	SndDoImmediate(chanPtr, &mySndCmd);

	mySndCmd.cmd = freqCmd;								// call this to START sound & keep looping 
	mySndCmd.param1 = 0;
	mySndCmd.param2 = freq;								// MIDI freq
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

			/* SET MY INFO */
			
	gChannelInfo[theChan].effectNum = effectNum;		// remember what effect is playing on this channel

	return(theChan);									// return channel #	
}



/*************** CHANGE CHANNEL FREQUENCY **************/
//
// Modifies the frequency of a currently playing channel
//
// freq is relative to 22khz in fixpt format where $10000 is 22khz, $20000 is 44khz, etc.
//

void ChangeChannelFrequency(short channel, long freq)
{
static	SndCommand 		mySndCmd;
static 	OSErr			iErr;
static	SndChannelPtr	chanPtr;

	if (channel < 0)									// make sure it's valid
		return;

	chanPtr = gSndChannel[channel];						// get the actual channel ptr				

	mySndCmd.cmd = rateCmd;								// modify the rate to change the frequency 
	mySndCmd.param1 = 0;
	mySndCmd.param2 = freq;	
	SndDoImmediate(chanPtr, &mySndCmd);
}


/*************** CHANGE CHANNEL VOLUME **************/
//
// Modifies the volume of a currently playing channel
//

void ChangeChannelVolume(short channel, short volume)
{
static	SndCommand 		mySndCmd;
static 	OSErr			iErr;
static	SndChannelPtr	chanPtr;

	if (channel < 0)									// make sure it's valid
		return;

	chanPtr = gSndChannel[channel];						// get the actual channel ptr				

	mySndCmd.cmd = ampCmd;								// set sound playback volume
	mySndCmd.param1 = volume;
	mySndCmd.param2 = 0;
	SndDoImmediate(chanPtr, &mySndCmd);
}


/******************** TOGGLE MUSIC *********************/

void ToggleMusic(void)
{
	gMuteMusicFlag = !gMuteMusicFlag;
	SndPauseFilePlay(gMusicChannel);			// pause it
}


/******************** DO SOUND MAINTENANCE *************/
//
// 		ReadKeyboard() must have already been called
//

void DoSoundMaintenance(void)
{
				
			/* SEE IF TOGGLE MUSIC */

	if (GetNewKeyState(kKey_ToggleMusic))
		ToggleMusic();
	
		
				/* SEE IF TOGGLE AMBIENT */
				
	if (GetNewKeyState(kKey_ToggleAmbient))
	{
		if (gAmbientEffect == -1)
			StartAmbientEffect();
		else
		{
			StopAChannel(&gAmbientEffect);
			gAmbientEffect = -1;
		}	
	}

			/* SEE IF CHANGE VOLUME */

	if (GetNewKeyState(kKey_RaiseVolume))
	{
		if (gCurrentSystemVolume < 0x100)
			gCurrentSystemVolume += 8;
		if (gCurrentSystemVolume > 0x100)
			gCurrentSystemVolume = 0x100;
		SetDefaultOutputVolume((gCurrentSystemVolume<<16)|gCurrentSystemVolume); // set system volume
	}
	else
	if (GetNewKeyState(kKey_LowerVolume))
	{
		if (gCurrentSystemVolume > 0x000)
			gCurrentSystemVolume -= 8;
		if (gCurrentSystemVolume < 0x000)
			gCurrentSystemVolume = 0;
		
		SetDefaultOutputVolume((gCurrentSystemVolume<<16)|gCurrentSystemVolume); // set system volume
	}

				/* SEE IF STREAMED MUSIC STOPPED - SO RESET */
				
	if (gResetSong)
	{
		gResetSong = false;
		if (gLoopSongFlag)							// see if stop song now or loop it
			PlaySong(-1,true);
		else
			KillSong();
	}
		
			/* UPDATE LAVA BUBBLE SOUND */
			
	if (gLavaSoundChannel != -1)
	{
		short	volume;
		
		volume = (1500.0 - gMinLavaDist) * .2;
		if (volume > FULL_CHANNEL_VOLUME)
			volume = FULL_CHANNEL_VOLUME;
		else
		if (volume < 0)
			volume = 0;
	
		ChangeChannelVolume(gLavaSoundChannel, volume);
	
		gMinLavaDist = 1000000;				// reset min for next loop	
	}		


			/* UPDATE STEAM SOUND */
			
	if (gSteamSoundChannel != -1)
	{
		short	volume;
		
		volume = (800.0 - gMinSteamDist) * .6;
		if (volume > FULL_CHANNEL_VOLUME)
			volume = FULL_CHANNEL_VOLUME;
		else
		if (volume < 0)
			volume = 0;
	
		ChangeChannelVolume(gSteamSoundChannel, volume);
	
		gMinSteamDist = 1000000;			// reset min for next loop	
	}		
	
}



/******************** FIND SILENT CHANNEL *************************/

static short FindSilentChannel(void)
{
short		theChan;
OSErr		myErr;
SCStatus	theStatus;

	for (theChan=0; theChan < gMaxChannels; theChan++)
	{
		myErr = SndChannelStatus(gSndChannel[theChan],sizeof(SCStatus),&theStatus);	// get channel info
		if (myErr)
			ShowSystemErr(myErr);
		if (!theStatus.scChannelBusy)					// see if channel not busy
		{
			return(theChan);
		}
	}
	
			/* NO FREE CHANNELS */

	return(-1);										
}


/***************** EMERGENCY FREE CHANNEL **************************/
//
// This is used by the music streamer when all channels are used.
// Since we must have a channel to play music, we arbitrarily kill some other channel.
//

static short EmergencyFreeChannel(void)
{
short	i;

	for (i = 0; i < gMaxChannels; i++)
	{
		if ((i != gLavaSoundChannel) &&				// try not to free one of the streaming channels
			(i != gSteamSoundChannel) &&
			(i != gAmbientEffect))
		{
		    short   x = i;
			StopAChannel(&x);
			return(i);
		}	
	}
	
		/* TOO BAD, GOTTA NUKE ONE OF THE STREAMING CHANNELS IT SEEMS */
			
	StopAChannel(0);
	return(0);
}

//=====================================================================================
//=====================================================================================
//=====================================================================================



/********************** START AMBIENT EFFECT **********************/

void StartAmbientEffect(void)
{
	gAmbientEffect = PlayEffect_Parms(EFFECT_AMBIENT,70,kMiddleC);				// ambient sound


}







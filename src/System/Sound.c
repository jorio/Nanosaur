/****************************/
/*     SOUND ROUTINES       */
/* (c)1994-98 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static short FindSilentChannel(void);
static void SongCompletionProc(SndChannelPtr chan);
static short EmergencyFreeChannel(void);


/****************************/
/*    CONSTANTS             */
/****************************/

// Source port note: this was 11 in "non-pro mode" (i.e. not Nanosaur Extreme).
#define		MAX_CHANNELS			14
#define		MAX_EFFECTS				30

typedef struct
{
	UInt16	effectNum;
	float	volumeAdjust;
}ChannelInfoType;


/**********************/
/*     VARIABLES      */
/**********************/

static	SndListHandle	gSndHandles[MAX_EFFECTS];		// handles to ALL sounds
static  long			gSndOffsets[MAX_EFFECTS];

static	SndChannelPtr	gSndChannel[MAX_CHANNELS];
static	SndChannelPtr	gMusicChannel=nil;
static short			gMaxChannels = 0;

static short			gCurrentMusicChannel = -1;
static short			gMusicFileRefNum = 0x0ded;

static	ChannelInfoType	gChannelInfo[MAX_CHANNELS];
static	short			gNumSndsInBank = 0;

static short				gCurrentSong = -1;

Boolean				gSongPlayingFlag = false;
Boolean				gResetSong = false;
Boolean				gLoopSongFlag = true;


long	gOriginalSystemVolume,gCurrentSystemVolume;

Boolean			gMuteMusicFlag = false;

		/*****************/
		/* EFFECTS TABLE */
		/*****************/

static const char*	kEffectNames[] =
{
	[EFFECT_HEATSEEK]	= "HeatSeek",
	[EFFECT_BLASTER]	= "Blaster",
	[EFFECT_SELECT]		= "Select",
	[EFFECT_EXPLODE]	= "Explode",
	[EFFECT_POWPICKUP]	= "POWPickup",
	[EFFECT_CRUNCH]		= "Crunch",
	[EFFECT_ALARM]		= "Alarm",
	[EFFECT_ENEMYDIE]	= "EnemyDie",
	[EFFECT_JETLOOP]	= "JetLoop",
	[EFFECT_JUMP]		= "Jump",
	[EFFECT_ROAR]		= "Roar",
	[EFFECT_FOOTSTEP]	= "Footstep",
	[EFFECT_DILOATTACK]	= "DiloAttack",
	[EFFECT_WINGFLAP]	= "WingFlap",
	[EFFECT_PORTAL]		= "Portal",
	[EFFECT_BUBBLES]	= "Bubbles",
	[EFFECT_CRYSTAL]	= "Crystal",
	[EFFECT_STEAM]		= "Steam",
	[EFFECT_ROCKSLAM]	= "RockSlam",
	[EFFECT_AMBIENT]	= "Ambient",
	[EFFECT_SHIELD]		= "Shield",
	[EFFECT_SONIC]		= "Sonic",
	[EFFECT_MENUCHANGE]	= "MenuChange",
};

short	gAmbientEffect = -1;


/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
OSErr		iErr;

			/* SET SYSTEM VOLUME INFO */
			
	GetDefaultOutputVolume(&gOriginalSystemVolume);		
	gOriginalSystemVolume &= 0xffff;	
	gCurrentSystemVolume = gOriginalSystemVolume;


	gMaxChannels = 0;

			/* INIT BANK INFO */

	gNumSndsInBank = 0;

	memset(gSndHandles, 0, sizeof(gSndHandles));
	memset(gSndOffsets, 0, sizeof(gSndOffsets));


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
}


/******************* LOAD A SOUND EFFECT ************************/

void LoadSoundEffect(int effectNum)
{
char path[256];
FSSpec spec;
short refNum;
OSErr err;

	GAME_ASSERT_MESSAGE(effectNum >= 0 && effectNum < NUM_EFFECTS, "illegal effect number");

	if (gSndHandles[effectNum])
	{
		// already loaded
		return;
	}

	snprintf(path, sizeof(path), ":Audio:SoundBank:%s.aiff", kEffectNames[effectNum]);

	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);
	GAME_ASSERT_MESSAGE(err == noErr, path);

	err = FSpOpenDF(&spec, fsRdPerm, &refNum);
	GAME_ASSERT_MESSAGE(err == noErr, path);

	gSndHandles[effectNum] = Pomme_SndLoadFileAsResource(refNum);
	GAME_ASSERT_MESSAGE(gSndHandles[effectNum], path);

	FSClose(refNum);

			/* GET OFFSET INTO IT */

	GetSoundHeaderOffset(gSndHandles[effectNum], &gSndOffsets[effectNum]);

			/* DECOMPRESS IT AHEAD OF TIME */

	Pomme_DecompressSoundResource(&gSndHandles[effectNum], &gSndOffsets[effectNum]);
}


/******************* DISPOSE OF A SOUND EFFECT ************************/

void DisposeSoundEffect(int effectNum)
{
	GAME_ASSERT_MESSAGE(effectNum >= 0 && effectNum < NUM_EFFECTS, "illegal effect number");

	if (gSndHandles[effectNum])
	{
		DisposeHandle((Handle) gSndHandles[effectNum]);
		gSndHandles[effectNum] = nil;
	}
}


/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(void)
{
	StopAllEffectChannels();

			/****************************/
			/* LOAD ALL EFFECTS IN BANK */
			/****************************/

	for (int i = 0; i < NUM_EFFECTS; i++)
	{
		LoadSoundEffect(i);
	}
}


/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(void)
{
short	i; 

	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

			/* FREE ALL SAMPLES */
			
	for (i=0; i < gNumSndsInBank; i++)
		DisposeHandle((Handle)gSndHandles[i]);


	gNumSndsInBank = 0;
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

	// Source port fix: Clear streaming sound channels
	gAmbientEffect = -1;
	gLavaSoundChannel = -1;
	gSteamSoundChannel = -1;
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

	if (!gGamePrefs.music)							// user doesn't want music
		return;

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
				OpenGameFile(":Audio:GameSong.aiff",&gMusicFileRefNum,errStr);
				break;

		case	1:
				OpenGameFile(":Audio:TitleSong.aiff",&gMusicFileRefNum,errStr);
				break;

		case	2:
				OpenGameFile(":Audio:Song_Pangea.aiff",&gMusicFileRefNum,errStr);
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
	iErr = SndStartFilePlay(
		gMusicChannel,
		gMusicFileRefNum,
		0,
		/*STREAM_BUFFER_SIZE*/ 0,
		/*gMusicBuffer*/ nil,
		nil,
		NewFilePlayCompletionProc(SongCompletionProc),
		true);
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

			/* SET LOOP FLAG ON STREAM (SOURCE PORT ADDITION) */
			/* So we don't need to re-read the file over and over. */

	mySndCmd.cmd = pommeSetLoopCmd;
	mySndCmd.param1 = loopFlag ? 1 : 0;
	mySndCmd.param2 = 0;
	iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
	if (iErr)
		DoFatalAlert("PlaySong: SndDoImmediate (pomme loop extension) failed!");
}


/***************** SONG COMPLETION PROC *********************/

static void SongCompletionProc(SndChannelPtr chan)
{
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
static	SndChannelPtr	chanPtr;
short					theChan;
OSErr	myErr;
	

			/* GET BANK & SOUND #'S FROM TABLE */

	GAME_ASSERT_MESSAGE(effectNum >= 0 && effectNum < NUM_EFFECTS, "illegal effect number");
	GAME_ASSERT_MESSAGE(gSndHandles[effectNum], "effect wasn't loaded!");


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
	mySndCmd.ptr = ((Ptr)*gSndHandles[effectNum]) + gSndOffsets[effectNum];	// pointer to SoundHeader
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

	if (GetNewNeedState(kNeed_ToggleMusic))
		ToggleMusic();


				/* SEE IF TOGGLE AMBIENT */

	if (GetNewNeedState(kNeed_ToggleAmbient))
	{
		if (gAmbientEffect == -1)
			StartAmbientEffect();
		else
		{
			StopAChannel(&gAmbientEffect);
			gAmbientEffect = -1;
		}	
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
	if (!gGamePrefs.ambientSounds)
		return;

	gAmbientEffect = PlayEffect_Parms(EFFECT_AMBIENT,70,kMiddleC);				// ambient sound


}







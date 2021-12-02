/****************************/
/*      MISC ROUTINES       */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;

unsigned long seed0 = 0, seed1 = 0, seed2 = 0;
unsigned long seed0_alt = 0, seed1_alt = 0, seed2_alt = 0;


/****************** DO SYSTEM ERROR ***************/

void ShowSystemErr(long err)
{
Str255		numStr;

	NumToStringC(err, numStr);
	DoAlert (numStr);
	CleanQuit();
}

/****************** DO SYSTEM ERROR : NONFATAL ***************/
//
// nonfatal
//
void ShowSystemErr_NonFatal(long err)
{
Str255		numStr;

	NumToStringC(err, numStr);
	DoAlert (numStr);
}

/*********************** DO ALERT *******************/

void DoAlert(const char* s)
{
	printf("NANOSAUR ALERT: %s\n", s);
	Enter2D();
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Nanosaur", s, NULL);
	Exit2D();
}

/*********************** DO ASSERT *******************/

void DoAssert(const char* msg, const char* file, int line)
{
	printf("NANOSAUR ASSERTION FAILED: %s - %s:%d\n", msg, file, line);
	static char alertbuf[1024];
	snprintf(alertbuf, 1024, "%s\n%s:%d", msg, file, line);
	Enter2D();
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Nanosaur: Assertion Failed!", alertbuf, NULL);
	ExitToShell();
}

/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* s)
{
	printf("NANOSAUR FATAL ALERT: %s\n", s);
	Enter2D();
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Nanosaur: Fatal Alert", s, NULL);
	CleanQuit();
}

/*********************** DO FATAL ALERT 2 *******************/

void DoFatalAlert2(const char* s1, const char* s2)
{
	static char alertbuf[1024];
	snprintf(alertbuf, 1024, "%s\n%s", s1, s2);
	printf("NANOSAUR FATAL ALERT: %s\n", alertbuf);
	Enter2D();
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Nanosaur: Fatal Alert", alertbuf, NULL);
	ExitToShell();
}


/************ CLEAN QUIT ***************/

void CleanQuit(void)
{
static	Boolean beenHere = false;

	if (!beenHere)
	{
		beenHere = true;
		
		UseResFile(gMainAppRezFile);
		
		StopAllEffectChannels();
		KillSong();

//		ShowBugdomAd();
		
//		if (gQD3DInitialized)
//			Q3Exit();
	}

	// Source port addition: save prefs before quitting if any setting was
	// changed without going through the settings screen (e.g. fullscreen mode)
	SavePrefs(&gGamePrefs);
	

	ExitToShell();
}





/******************** MY RANDOM LONG **********************/
//
// My own random number generator that returns a LONG
//
// NOTE: call this instead of MyRandomShort if the value is going to be
//		masked or if it just doesnt matter since this version is quicker
//		without the 0xffff at the end.
//

unsigned long MyRandomLong(void)
{
  return seed2 ^= (((seed1 ^= (seed2>>5)*1568397607UL)>>7)+
                   (seed0 = (seed0+1)*3141592621UL))*2435386481UL;
}


/************** RANDOM FLOAT ********************/
//
// returns a random float between 0 and 1
//

float RandomFloat(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;		
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f / (float)0xfff;					// get # between 0..1
	return(f);
} 
 


/**************** SET MY RANDOM SEED *******************/

void SetMyRandomSeed(unsigned long seed)
{
	seed0 = seed;
	seed1 = 0;
	seed2 = 0;	
	
	seed0_alt = seed;
	seed1_alt = 0;
	seed2_alt = 0;	
	
}



/****************** ALLOC HANDLE ********************/

Handle	AllocHandle(long size)
{
Handle	hand;
OSErr	err;

	hand = NewHandleClear(size);					// alloc in APPL
	if (hand == nil)
	{
		hand = TempNewHandle(size,&err);			// try TEMP mem
		if (hand == nil)
		{
			DoFatalAlert("AllocHandle: failed!");
			return(nil);
		}
		else
			return(hand);							// use TEMP
	}

	return(hand);									// use APPL	
}


/****************** ALLOC PTR ********************/

Ptr	AllocPtr(long size)
{
	return NewPtr(size);
}



#pragma mark -

/***************** APPLY FICTION TO DELTAS ********************/

void ApplyFrictionToDeltas(float f,TQ3Vector3D *d)
{
	if (d->x < 0.0f)
	{
		d->x += f;
		if (d->x > 0.0f)
			d->x = 0;
	}
	else
	if (d->x > 0.0f)
	{
		d->x -= f;
		if (d->x < 0.0f)
			d->x = 0;
	}

	if (d->z < 0.0f)
	{
		d->z += f;
		if (d->z > 0.0f)
			d->z = 0;
	}
	else
	if (d->z > 0.0f)
	{
		d->z -= f;
		if (d->z < 0.0f)
			d->z = 0;
	}
}










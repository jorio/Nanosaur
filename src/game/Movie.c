/****************************/
/*      MOVIE.C 	        */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include	"globals.h"
#include	"misc.h"
#include	"movie.h"
#include	<Movies.h>
#include 	<Sound.h>
#include 	<QuickTimeComponents.h>
#include	"input.h"
#include 	<Gestalt.h>
#include	"sound2.h"

extern	WindowPtr	gCoverWindow;


/****************************/
/*    PROTOTYPES            */
/****************************/

static Movie GetMovie(FSSpec *spec);

/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/


/************************** INIT QUICKTIME **************************/
//
// Called at launch time.
//

void InitQuickTime(void)
{
OSErr	iErr;
long	theResult;

			/* IS QUICKTIME AVAILABLE? */
			
	iErr = Gestalt(gestaltQuickTime,&theResult);
	if(iErr != noErr)
		DoFatalAlert("\pYou must have QuickTime 3.0 or newer installed for this program to run.");



}


/****************** PLAY A MOVIE *************************/

void PlayAMovie(FSSpec *spec)
{
Movie	aMovie;
Rect	movieBox;
Size	size;
OSErr	iErr;

	MaxMem(&size);

			/* START QUICKTIME */
			
	iErr = EnterMovies();
	if (iErr)
		DoFatalAlert("\pPlayAMovie: EnterMovies failed!");


	SetPort(gCoverWindow);
	BackColor(blackColor);
	EraseRect(&gCoverWindow->portRect);

	aMovie = GetMovie(spec);
	
	GetMovieBox(aMovie,&movieBox);
	OffsetRect(&movieBox,-movieBox.left, -movieBox.top);
	OffsetRect(&movieBox,0,320/4);
	SetMovieBox(aMovie,&movieBox);
	
	SetMovieGWorld(aMovie, (CGrafPtr)gCoverWindow, nil);
	
	GoToBeginningOfMovie(aMovie);
	StartMovie(aMovie);
	
	KillSong();										// stop any other music
	
	while (!IsMovieDone(aMovie))
	{
		MoviesTask(aMovie, 0);
		ReadKeyboard();
		if (GetKeyState_Real(KEY_SPACE))
			break;
	}
	
	StopMovie(aMovie);
	DisposeMovie(aMovie);

			/* QUIT QUICKTIME */
			
	ExitMovies();
}


/***************** GET MOVIE ***************************/

static Movie GetMovie(FSSpec *spec)
{
OSErr	err;
short	movieResFile,movieResID;
Str255	movieName;
Movie	aMovie = nil;
Boolean	wasChanged;

	err = OpenMovieFile(spec, &movieResFile, fsRdPerm);
	if (err)
		DoFatalAlert("\pGetMovie: OpenMovieFile failed!");

	movieResID = 0;								// want 1st movie

	err = NewMovieFromFile(&aMovie, movieResFile, &movieResID, movieName, newMovieActive, &wasChanged);
	if (err)
		DoFatalAlert("\pGetMovie: NewMovieFromFile failed!");
		
	CloseMovieFile(movieResFile);
	
	return(aMovie);
}

















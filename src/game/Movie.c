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
#include	"input.h"
#include	"sound2.h"

extern	WindowPtr	gCoverWindow;


/****************************/
/*    PROTOTYPES            */
/****************************/

#if 0
static Movie GetMovie(FSSpec *spec);
#endif

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
#if 0
OSErr	iErr;
long	theResult;

			/* IS QUICKTIME AVAILABLE? */
			
	iErr = Gestalt(gestaltQuickTime,&theResult);
	if(iErr != noErr)

		DoFatalAlert("You must have QuickTime 3.0 or newer installed for this program to run.");
#endif


}


/****************** PLAY A MOVIE *************************/

void PlayAMovie(FSSpec *spec)
{
#if 1
	TODO2(Pascal2C(spec->name));
#else
Movie	aMovie;
Rect	movieBox;
Size	size;
OSErr	iErr;

	MaxMem(&size);

			/* START QUICKTIME */
			
	iErr = EnterMovies();
	if (iErr)
		DoFatalAlert("PlayAMovie: EnterMovies failed!");


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
#endif
}


/***************** GET MOVIE ***************************/

#if 0
static Movie GetMovie(FSSpec *spec)
{
OSErr	err;
short	movieResFile,movieResID;
Str255	movieName;
Movie	aMovie = nil;
Boolean	wasChanged;

	err = OpenMovieFile(spec, &movieResFile, fsRdPerm);
	if (err)
		DoFatalAlert("GetMovie: OpenMovieFile failed!");

	movieResID = 0;								// want 1st movie

	err = NewMovieFromFile(&aMovie, movieResFile, &movieResID, movieName, newMovieActive, &wasChanged);
	if (err)
		DoFatalAlert("GetMovie: NewMovieFromFile failed!");
		
	CloseMovieFile(movieResFile);
	
	return(aMovie);
}
#endif

















/****************************/
/*  OPEN TRANSPORT: GATHER  */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "globals.h"
#include "ot_gather.h"
#include "windows.h"
#include "misc.h"
#include "NetSupport.h"

extern 	NSpGameReference	gGame;
extern 	Byte 				gNumPlayers,gNetPlayerCharacterTypes[],gCurrentLevel;
extern	Byte				gNetPlayerCharacterTypes[MAX_PLAYERS];
extern	Byte				gNumClientsStillPlaying;


/****************************/
/*    PROTOTYPES            */
/****************************/

static pascal Boolean GatherGameDialogCallback (DialogPtr dp,EventRecord *event, short *item);
static void ShowNamesOfJoinedPlayers(void);
static void InitGatherListBox(Rect *r, DialogPtr myDialog);


/****************************/
/*    VARIABLES             */
/****************************/

ListHandle		gTheList;
short			gNumRowsInList;
Str32			gListBuffer[MAX_PLAYERS];

static	short	gNumGatheredPlayers;

NSpPlayerID		gNetPlayerNSpPlayerID[MAX_PLAYERS];
Boolean			gNetPlayerIsActive[MAX_PLAYERS];

//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================


/********************* DO MY CUSTOM GATHER GAME DIALOG **********************/
//
// Displays dialog which shows all currently gathered players.
//
// OUTPUT: OSErr = noErr if all's well.
//

OSErr  DoMyCustomGatherGameDialog(void)
{
DialogPtr 		myDialog;
short			itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;
Boolean			dialogDone,cancelled = false;
UniversalProcPtr	myProc;

	gNumGatheredPlayers = 0;												// noone gathered yet

	myDialog = GetNewDialog(129,nil,MOVE_TO_FRONT);

			/* SET OUTLINE FOR USERITEM */
			
	GetDItem(myDialog,1,&itemType,(Handle *)&itemHandle,&itemRect);					// default button
//	SetDItem(myDialog, 2, userItem, (Handle)NewUserItemProc(DoBold), &itemRect);
			
	
			/* INIT LIST BOX */
			
	GetDItem(myDialog,4,&itemType,(Handle *)&itemHandle,&itemRect);					// player's box
	SetDItem(myDialog,4, userItem,(Handle)NewUserItemProc(DoOutline), &itemRect);
	InitGatherListBox(&itemRect,myDialog);											// create list manager list


				/*************/
				/* DO DIALOG */
				/*************/

	dialogDone = false;
	myProc = NewModalFilterProc(GatherGameDialogCallback);
	while(dialogDone == false)
	{
		ModalDialog(myProc, &itemHit);
		switch (itemHit)
		{
			case 	3:
					cancelled = true;
					dialogDone = true;
					break;
					
			case	1:									// see if PLAY
					if (gNumGatheredPlayers > 1)
					{
						dialogDone = true;
						gNumPlayers = gNumGatheredPlayers;
					}
					else
						SysBeep(0);
					break;					
		}
	}
	
		/* STOP ADVERTISING THIS GAME SINCE WE'RE ALL SET TO GO */
	
	if ((void *)NSpGetVersion != (void *)kUnresolvedCFragSymbolAddress)			// the GetVersion call only exists in 1.0.3 or better
		NSpGame_EnableAdvertising(gGame, nil, false);			// & 1.0.3 is the first version where Advertising doesnt crash
	
	
			/* CLEANUP */
			
	DisposeRoutineDescriptor(myProc);
	DisposDialog(myDialog);
			
	GameScreenToBlack();			
	return(cancelled);
}


/******************** INIT GATHER LIST BOX *************************/
//
// Creates the List Manager list box which will contain a list of all the joiners in this game.
//

static void InitGatherListBox(Rect *r, DialogPtr myDialog)
{
Rect	dataBounds;
Point	cSize;

	r->right -= 15;														// make room for scroll bars & outline
	r->top += 1;
	r->bottom -= 1;
	r->left += 1;

	gNumRowsInList = 0;
	SetRect(&dataBounds,0,0,1,gNumRowsInList);							// no entries yet
	cSize.h = cSize.v = 0;
	gTheList = LNew(r, &dataBounds, cSize, 0, myDialog, true, false, false, false);
}



/**************** GATHER GAME DIALOG CALLBACK *************************/

static pascal Boolean GatherGameDialogCallback (DialogPtr dp,EventRecord *event, short *item)
{
char 			c;
Point			eventPoint;
static	long	tick = 0;
NSpMessageHeader	*message;
	
				/* HANDLE DIALOG EVENTS */
				
	SetPort(dp);										// make sure we're drawing to this dialog

	switch(event->what)
	{
		case	keyDown:								// we have a key press
				c = event->message & 0x00FF;			// what character is it?
				break;
	
		case	mouseDown:								// mouse was clicked
				eventPoint = event->where;				// get the location of click
				GlobalToLocal (&eventPoint);			// got to make it local
				break;						
	}

			/*******************************************/
			/* CHECK FOR OTHER PLAYERS WANTING TO JOIN */
			/*******************************************/
		
	while ((message = NSpMessage_Get(gGame)) != NULL)	// read Net message
	{
		switch(message->what)
		{
			case	kNSpPlayerLeft:						// see if someone decided to un-join
					gNumGatheredPlayers--;
					ShowNamesOfJoinedPlayers();
					break;
					
			case 	kNSpPlayerJoined:					// see if we've got a new player joining
					gNumGatheredPlayers++;
					ShowNamesOfJoinedPlayers();
					break;
		}
		NSpMessage_Release(gGame, message);
	}
	
	return(false);
}


/***************** SHOW NAMES OF JOINED PLAYERS ************************/
//
// For the Gather Game dialog, it displays list of joined players by updating
// the List Manager list for this dialog.
//

static void ShowNamesOfJoinedPlayers(void)
{
short	i;
Cell	theCell;
NSpPlayerEnumerationPtr	players;
OSStatus	status;


	status = NSpPlayer_GetEnumeration(gGame, &players);
	if (status != noErr)
		return;
		
		/* COPY NBP NAMES INTO LIST BUFFER */
		
	for (i=0; i < players->count; i++)
	{
		CopyPStr(players->playerInfo[i]->name, gListBuffer[i]);
	}

	NSpPlayer_ReleaseEnumeration(gGame, players);
	
		/* DELETE ALL EXISTING ROWS IN LIST */
		
	LDelRow(0, 0, gTheList);
	gNumRowsInList = 0;


			/* ADD NAMES TO LIST */

	if (gNumGatheredPlayers > 0)
		LAddRow(gNumGatheredPlayers, 0, gTheList);		// create rows


	for (i=0; i < gNumGatheredPlayers; i++)
	{
		if (i == (gNumGatheredPlayers-1))				// reactivate draw on last cell
			LDoDraw(true,gTheList);							// turn on updating
		theCell.h = 0;
		theCell.v = i;
		LSetCell(&gListBuffer[i][1], gListBuffer[i][0], theCell, gTheList);
		gNumRowsInList++;		
	}
}


/*********************** SEND GAME CONFIGURATION INFO *******************************/
//
// Sends game parameters to other players.
//
// OUTPUT: OSErr = noErr if all is well.
//

OSErr SendGameConfigInfo(void)
{
OSStatus				status;
PFConfigMessageType			message;
NSpPlayerEnumerationPtr	playerList;
NSpPlayerID				hostID,clientID;
short					i;
short					pNum;
NSpPlayerInfoPtr		playerInfoPtr;

			/* GET PLAYER INFO */
			
	hostID = NSpPlayer_GetMyID(gGame);							// get my/host ID
	status = NSpPlayer_GetEnumeration(gGame, &playerList);
	gNumPlayers = playerList->count;							// get # players (should already have this, but get again to be sure)

	gNumClientsStillPlaying = gNumPlayers-1;					// all CLIENTS are still playing
			
			/*****************************************/
			/* CREATE LIST OF PLAYER CHARACTER TYPES */
			/*****************************************/

	for (i=0; i < gNumPlayers; i++)
	{
		playerInfoPtr =  playerList->playerInfo[i];				// point to player's info
		gNetPlayerCharacterTypes[i] = 
		message.playerCharType[i] =	playerInfoPtr->type;		// get player type (as defined when Joined)
		gNetPlayerNSpPlayerID[i] = playerInfoPtr->id;			// get NSp's playerID (for use when player leaves game)
		gNetPlayerIsActive[i] = true;							// this player is active
	}			

	message.arenaNum = gCurrentLevel;							// set arena # to play

	
			/***********************************************/
			/* SEND GAME CONFIGURATION INFO TO ALL PLAYERS */
			/***********************************************/
			
	pNum = 1;													// # to assign to each player (host is #0)
	for (i = 0; i < gNumPlayers; i++)
	{
		playerInfoPtr =  playerList->playerInfo[i];

		clientID = playerInfoPtr->id;							// get this player's ID
		if (clientID == hostID)									// don't send start info to myself/host
			continue;
			
			
				/* MAKE NEW MESSAGE */
					
		NSpClearMessageHeader(&message.h);
		message.h.to 			= clientID;						// send to this client
		message.h.what 			= kPFConfigureMessage;				// set message type
		message.h.messageLen 	= sizeof(message);				// set size of message
		message.numPlayers 		= gNumPlayers;					// set # players
		message.playerNum 		= pNum++;						// set player num
		status = NSpMessage_Send(gGame, &message.h, kNSpSendFlag_Registered);	// send message
	}

			/************/
			/* CLEAN UP */
			/************/
				
	NSpPlayer_ReleaseEnumeration(gGame,playerList);					// dispose of player list
		
	return(noErr);
}




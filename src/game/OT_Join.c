/****************************/
/*  OPEN TRANSPORT: JOIN    */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <NetSprocket.h>
#include "globals.h"
#include "misc.h"
#include "ot_join.h"
#include "windows.h"
#include "NetSupport.h"
#include "player_control.h"


extern NSpGameReference gGame;
extern	Byte			gMyPlayerNum,gNumClientsStillPlaying;
extern 	Byte			gNumPlayers,gCurrentLevel;
extern	Byte			gNetPlayerCharacterTypes[MAX_PLAYERS];

/****************************/
/*    PROTOTYPES            */
/****************************/

static pascal Boolean WaitForGameConfigInfoDialogCallback (DialogPtr dp,EventRecord *event, short *item);

static void HandleGameConfigMessage(PFConfigMessageType *inMessage);


/****************************/
/*    CONSTANTS             */
/****************************/




/******************** WAIT FOR GAME CONFIGURATION INFO *****************************/
//
// Waits for gatherer to tell me which player # I am et.al.
//
// OUTPUT:	OSErr == noErr if all went well, otherwise aborted.
//

OSErr WaitForGameConfigInfo(void)
{
DialogPtr	myDialog;
Boolean		dialogDone,cancelled;
short			itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;
UniversalProcPtr	myProc;

	myDialog = GetNewDialog(131,nil,MOVE_TO_FRONT);


			/* SET OUTLINE FOR USERITEM */
			
	GetDItem(myDialog,1,&itemType,(Handle *)&itemHandle,&itemRect);					// default button
	SetDItem(myDialog, 2, userItem, (Handle)NewUserItemProc(DoBold), &itemRect);


	dialogDone = cancelled = false;
	myProc = NewModalFilterProc(WaitForGameConfigInfoDialogCallback);
	while(dialogDone == false)
	{
		ModalDialog(myProc, &itemHit);
		switch (itemHit)
		{
			case 	1:
					cancelled = true;
					dialogDone = true;
					break;	
					
			case	100:				
					dialogDone = true;
					break;	
			default:
				dialogDone = false;
			break;
		}
	}

	DisposeRoutineDescriptor(myProc);
	DisposeDialog(myDialog);
	GameScreenToBlack();	
	return(cancelled);
}


/********************* WAIT FOR GAME CONFIG INFO: DIALOG CALLBACK ***************************/
//
// Returns true if game start info was received.  Upon return, "item" will be set to 100.
//

static pascal Boolean WaitForGameConfigInfoDialogCallback (DialogPtr dp,EventRecord *event, short *item)
{
NSpMessageHeader *message;
Boolean handled = false;
	
			/* HANDLE NET SPROCKET EVENTS */
			
	while ((message = NSpMessage_Get(gGame)) != nil)							// get message from Net
	{
		switch(message->what)													// handle message
		{
			case	kPFConfigureMessage:										// GOT GAME START INFO
					HandleGameConfigMessage((PFConfigMessageType *)message);
					*item = 100;
					handled = true;
					goto got_config;
					break;
					
			case 	kNSpGameTerminated:											// Host terminated the game :(
					*item = 1;
					handled = true;
					break;
					
			case	kNSpJoinApproved:
			case	kNSpPlayerJoined:
					break;
					
			case	kNSpError:
					DoFatalAlert("\pWaitForGameConfigInfoDialogCallback: message == kNSpError");
					break;
					
			default:
					DoFatalAlert("\pWaitForGameConfigInfoDialogCallback: Unexpected net message.");
					
		}
		NSpMessage_Release(gGame, message);										// dispose of message
	}
	
got_config:	
	
			/* HANDLE DIALOG EVENTS */
			
	switch (event->what)
	{
		case keyDown:
			switch (event->message & charCodeMask)
			{
				case 	0x03:  					// Enter
				case 	0x0D: 					// Return
						*item = 1;
						handled = true;
						break;
				
				case 	0x1B:  					// Escape
						*item = 1;
						handled = true;
						break;
				
				case 	'.':  					// Command-period
						if (event->modifiers & cmdKey)
						{
							*item = 1;
							handled = true;
						}
						break;
			}
	}
	return(handled);
}



/************************* HANDLE GAME CONFIGURATION MESSAGE *****************************/
//
// Called while polling in WaitForGameConfigInfoDialogCallback.
//

static void HandleGameConfigMessage(PFConfigMessageType *inMessage)
{
short	i;

	gNumPlayers = inMessage->numPlayers;
	gMyPlayerNum = inMessage->playerNum;
	gNumClientsStillPlaying = gNumPlayers-1;							// all players are still in game (NOTE: this value is actually only used by the host, but we're setting it anyway)

			/* GET EACH PLAYER'S CHARACTER TYPE */
			
	for (i=0; i < gNumPlayers; i++)
		gNetPlayerCharacterTypes[i] = inMessage->playerCharType[i];	
		
		
				/* GET ARENA/LEVEL TO PLAY */
				
	gCurrentLevel = inMessage->arenaNum;	
}










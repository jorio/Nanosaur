/****************************/
/*  	NETSUPPORT.C  		*/
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "NetSupport.h"
#include "OT_Gather.h"
#include "OT_Join.h"
#include "main.h"
#include "misc.h"
#include "qd3d_support.h"
#include "3dmf.h"
#include "objtypes.h"
#include "qd3d_geometry.h"
#include "windows.h"
#include "input.h"

#include <QD3DShader.h>
#include <QD3DStorage.h>
#include <InputSprocket.h>

extern	float			gFramesPerSecond,gFramesPerSecondFrac;
extern	Boolean			gAbortedFlag,gGameOverFlag,gExitLevelFlag,gNetPlayerIsActive[];
extern	TQ3Object		gCustomHeadGeometry[];
extern	TQ3SurfaceShaderObject	gCustomHeadTexture[];
extern	Byte			gMyPlayerNum,gPlayMode;
extern	KeyControlType	gPlayerControlBits[],gMyControlBits;
extern 	Byte			gNumPlayers,gMyCharacterType;
extern	TQ3Object		gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	NSpPlayerID		gNetPlayerNSpPlayerID[MAX_PLAYERS];


/****************************/
/*    PROTOTYPES            */
/****************************/

static void HandleSinglePlayerKeys(PFSinglePlayerKeysMessage *inMessage);
static void HandleAllPlayerKeys(PFAllPlayerKeysMessage *inMessage);


/****************************/
/*    CONSTANTS             */
/****************************/

NSpGameReference	gGame;


StringPtr	kNBPType = "\pWW10";		


/****************************/
/*    VARIABLES             */
/****************************/

unsigned long	gNetPacketCounter = 0;
Boolean	gNetSprocketExists = false;
Boolean	gCanDoNetwork = false;

Byte	gNumClientKeysReceived,gNumClientsStillPlaying;


/************************* INIT NETWORK DRIVER ****************************/

void Net_InitNetworkDriver(void)
{
OSStatus	status;
OSErr					iErr;
ConnectionID			connID;
Ptr		mainAddr;
Str255	errName;

			/* VERIFY LIBRARIES HAVE BEEN LOADED */

	iErr = GetSharedLibrary("\pNetSprocketLib",kAnyCFragArch,kFindCFrag,&connID,&mainAddr,errName);
	if (iErr)
	{
err:	
		gNetSprocketExists = false;
		gCanDoNetwork = false;
		return;
	}

			/* INIT NET SPROCKETS */
			
	status = NSpInitialize(kStandardMessageSize, kNetBufferSize, kQElements, kGameID, kTimeout);
	if (status != noErr)
		goto err;
		
	gNetSprocketExists = true;
	gCanDoNetwork = true;
}


/************************ DO MY NET PLAY DIALOGS ****************************/
//
// Does all of the dialog boxes and Net Sprocket stuff for doing
// a network game.
//
// OUTPUT: OSErr == error if need to abort to main menu
//

OSErr DoMyNetPlayDialogs(void)
{
	switch(gPlayMode)
	{
		case	PLAY_MODE_GATHER:
				GameScreenToBlack();							// erase BG in prep for dialog boxes
				if (DoGatherGame())								// do gathering
					goto bye;
					
				if (SendGameConfigInfo())						// send start info
					goto error;
				
				gPlayMode = PLAY_MODE_HOST;						// I am the host/server
				break;
				
		case	PLAY_MODE_JOIN:
				GameScreenToBlack();							// erase BG in prep for dialog boxes
				if (DoJoinGame())								// do joining
					goto error;
				
				gPlayMode = PLAY_MODE_CLIENT;
				break;
	}
	HideCursor();								 
	return(noErr);
	
			/* NEED TO ABORT OUT */
			
error:
	KillGame();
bye:
	HideCursor();								 	
	return(!noErr);
}






/******************************** DO GATHER GAME *****************************************/
//
// OUTPUT:	noErr = ok, otherwise error.
//

OSErr DoGatherGame(void)
{
OSStatus status;
Boolean	 okHit;
Str31	gameName;
Str31 	playerName;
Str31 	password;
StringHandle		userName;
NSpProtocolListReference theList;
NSpProtocolReference atRef;


		/* USE MACHINE NAME AS DEFAULT PLAYER NAME */
			
	userName = GetString(-16096);
	if (userName == NULL)
		CopyPStr("\pBozo",playerName);
	else
	{
		CopyPStr(*userName, playerName);
		ReleaseResource ((Handle) userName);
	}
	
	CopyPStr("\pWeekendWarrior", gameName);
	
			/* SET PROTOCOL LIST */
			
	atRef = NSpProtocol_CreateAppleTalk(gameName,kNBPType, 0,0);		// create appletalk protocol ref
	
	status = NSpProtocolList_New(NULL, &theList);						// create protocol list
	if (status != noErr)
	{
		DoAlert("\pDoGatherGame: NSpProtocolList_New failed!");
		return(status);
	}
		
	status = NSpProtocolList_Append(theList, atRef);					// append protocol refs
	if (status != noErr)
	{
		DoAlert("\pDoGatherGame: NSpProtocolList_Append failed!");
		goto abort_out;
	}
		
		
			/* DO NET SPROCKET HOST DIALOG */
			
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);						// flush before Jamie's code!
	password[0] = 0;
	okHit = NSpDoModalHostDialog(theList, gameName, playerName, password, NULL);
	GameScreenToBlack();
	if (!okHit)
		goto abort_out;
		
		
			/* HOST GAME & GET GAME REFERENCE */
		
	status = NSpGame_Host(&gGame, theList, MAX_PLAYERS, gameName, password, playerName,
								gMyCharacterType, kNSpClientServer, 0);			
	if (status != noErr)
	{	
		DoAlert("\pDoGatherGame: NSpGame_Host failed!");
		ShowSystemErr_NonFatal(status);
		goto abort_out;
	}
		
		
			/* GOT GAME REF, SO DISPOSE OF PROTOCOL REF&LIST */
			
	NSpProtocolList_Dispose(theList);
	
	
			/*********************************************/
			/* DO MY CUSTOM PILLOW FIGHTER GATHER DIALOG */
			/*********************************************/
				
	if (DoMyCustomGatherGameDialog())
	{
		KillGame();
		return(!noErr);
	}
	
	return(noErr);
	
	
				/* KOSHER CANCEL OUT */
				
abort_out:
	NSpProtocolList_Dispose(theList);									// be sure to dispose of this
	return(!noErr);
}


/************************ DO JOIN GAME **********************************/
//
// OUTPUT:	OSErr == noErr if all went well.
//

OSErr DoJoinGame(void)
{
NSpAddressReference		theAddr;
Str31 					playerName;
Str31 					password;
StringHandle			userName;
OSStatus				status;
Boolean 				cancelled;
	
			/* GET MACHINE NAME AS DEFAULT PLAYER NAME */
			
	userName = GetString (-16096);
	if (userName == NULL)
		CopyPStr("\pBozo", playerName);
	else
	{
		CopyPStr(*userName, playerName);
		ReleaseResource ((Handle) userName);
	}
	
			/* DO SPROCKETS JOIN DIALOG */
			
	password[0] = 0;												// no password
	theAddr = NSpDoModalJoinDialog(kNBPType, "\pWeekend Warrior Games",
								 playerName, password, NULL);
	GameScreenToBlack();
	if (theAddr == nil)												// see if cancelled or error
		return(!noErr);
								 
			/* ATTEMPT TO JOIN THE GAME */
			
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);					// flush before Jamie's code!
	status = NSpGame_Join(&gGame, theAddr, playerName, password,	// try to join the game
						 gMyCharacterType, NULL, 0, 0);
						 
	NSpReleaseAddressReference(theAddr);							// always dispose of this after _Join
						 
	if (status)														// join failed
		return(status);
	else
	{
			/* JOINED, SO WAIT FOR HOST TO SEND GAME START INFO */
			
		cancelled = WaitForGameConfigInfo();
		if (cancelled)
		{
			KillGame();
			return(!noErr);
		}
		else
		{
			return(noErr);
		}
	}
}


/************** SEND STATE TO HOST *************************/
//
// Send's local player's key state (gMyControlBits) to host machine.
//

OSStatus SendStateToHost(void)
{
OSStatus					status;
PFSinglePlayerKeysMessage	message;
	
			/* PREP A NEW MESSAGE */
			
	NSpClearMessageHeader(&message.h);
	
	message.h.to = kNSpHostOnly;									// send only to host
	message.h.what = kPFSinglePlayerKeys;							// set message type
	message.h.messageLen = sizeof(message);							// set size of message
	message.playerNum = gMyPlayerNum;								// set player #
	message.packetCounter = gNetPacketCounter++;					// set frame counter
	message.keys = gMyControlBits;									// set key map data
	
			/* SEND THE MESSAGE */
			
	status = NSpMessage_Send(gGame, &message.h, kNSpSendFlag_Registered);	
	if (status)
		DoFatalAlert("\pSendStateToHost: NSpMessage_Send failed");

	return(status);	
}


/************************** SEND STATE TO CLIENTS ************************/
//
// Send's all player's key states (gPlayerControlBits) to all players.
//
// Note: even tho players may have dropped out of game, the game
//		continues along without recieving new data from them, but their
//		character still appears on everyone's screen.
//

OSStatus SendStateToClients(void)
{
OSStatus				status;
PFAllPlayerKeysMessage	message;
int 					i;
	
			/* PREP A NEW MESSAGE */
			
	NSpClearMessageHeader(&message.h);
	
	message.h.to = kNSpAllPlayers;									// send to all players
	message.h.what = kPFAllPlayerKeys;								// set message type
	message.h.messageLen = sizeof(message)*gNumPlayers;				// set size of message
	
	message.packetCounter = gNetPacketCounter++;					// set frame counter
	message.framesPerSecond = gFramesPerSecond;						// set gFramesPerSecond
	message.framesPerSecondFrac = gFramesPerSecondFrac;				// set gFramesPerSecondFrac
	
	for (i = 0; i < gNumPlayers; i++)								// set message data (all players keys)
		message.keys[i] = gPlayerControlBits[i];

			/* SEND THE MESSAGE */
				
	status = NSpMessage_Send(gGame, &message.h, kNSpSendFlag_Registered);	// must be recieved
	if (status)
		DoFatalAlert("\pSendStateToClients: NSpMessage_Send failed");
	
	gNumClientKeysReceived = 0;										// reset this for next frame
	
	return(status);	
}


/*************************** PROCESS ALL NET DATA *******************************/
//
// Reads all pending messages in net receive queue and processes them.
//
// OUTPUT: true if got all keys from host, or if got enough keys from all clients
//

Boolean ProcessAllNetData(void)
{
NSpMessageHeader *message;

	if (gGameOverFlag)				// just double check this for safety
		return(true);	
	
	while ((message = NSpMessage_Get(gGame)) != nil)							// get next message
	{
		switch(message->what)													// see what type the message is
		{
			case	kNSpPlayerLeft:
					HandlePlayerLeft((NSpPlayerLeftMessage *)message);
					if (gNumClientsStillPlaying == 0)							// see if they're all gone
					{
						DoAlert("\pEveryone has left the game.");
						gGameOverFlag = true;
						gAbortedFlag = true;
						gExitLevelFlag = true;
						NSpMessage_Release(gGame, message);
						return(true);
					}
					break;
					
			case 	kNSpPlayerJoined:		// (note: joining in middle of game isnt allowed!)
					DoAlert("\pPlayer attempted to join game - shouldn't have happened, report bug to Brian!");
					break;
					
			case 	kPFConfigureMessage:
					break;
					
			case 	kPFStopMessage:
					break;
					
			case 	kPFSinglePlayerKeys:
					HandleSinglePlayerKeys((PFSinglePlayerKeysMessage *)message);
					NSpMessage_Release(gGame, message);
					return(gNumClientKeysReceived >= gNumClientsStillPlaying);
					break;
					
			case 	kPFAllPlayerKeys:
					HandleAllPlayerKeys((PFAllPlayerKeysMessage *)message);
					NSpMessage_Release(gGame, message);
					return(true);
					break;
					
			case	kNSpGameTerminated:
					DoAlert("\pThe host has terminated this network game.");
					gGameOverFlag = true;
					gAbortedFlag = true;
					gExitLevelFlag = true;
					return(true);
					break;
					
			default:
					DoAlert("\pProcessAllNetData: Unknown/unsupported message arrived!");
					ShowSystemErr(message->what);
		}
		NSpMessage_Release(gGame, message);										// dispose of this message
	}
	
	return(false);
}



/******************* HANDLE SINGLE PLAYER KEYS **************************/
//
// This is called when a kPFSinglePlayerKeys message is recieved from client by host
//

static void HandleSinglePlayerKeys(PFSinglePlayerKeysMessage *inMessage)
{
Byte	playerNum;

	if (inMessage->packetCounter != (gNetPacketCounter-1))					// make sure we're still in sync
		DoFatalAlert("\pHandleSinglePlayerKeys detects that gNetPacketCounter is out of sync!");

	playerNum = inMessage->playerNum;								// get player #

	if (!gNetPlayerIsActive[playerNum])								// see if this player left the game (then this packet is just lingering around)
		return;
		
	gPlayerControlBits[inMessage->playerNum] = inMessage->keys;		// get control bits from message
	gNumClientKeysReceived++;
}


/******************** HANDLE ALL PLAYER KEYS *****************************/
//
// This is called when a kPFAllPlayerKeys message is recieved from host by client
//

static void HandleAllPlayerKeys(PFAllPlayerKeysMessage *inMessage)
{
long i;

	if (inMessage->packetCounter != gNetPacketCounter)					// make sure we're still in sync
		DoFatalAlert("\pHandleAllPlayerKeys detects that gNetPacketCounter is out of sync!");

	gFramesPerSecond = inMessage->framesPerSecond;						// get gFramesPerSecond
	gFramesPerSecondFrac = inMessage->framesPerSecondFrac;				// get gFramesPerSecondFrac

	for (i = 0; i < gNumPlayers; i++)
		gPlayerControlBits[i] = inMessage->keys[i];
}


/******************** HANDLE PLAYER LEFT **************************/
//
// This is called when a kNSpPlayerLeft message is recieved.
// Handles a player leaving the game.
//
// When a player leaves, all that happens is that the gNumClientsStillPlaying is decremented
// so that the host knows how many CLIENT-players to still poll data for.
//

void HandlePlayerLeft(NSpPlayerLeftMessage *inMessage)
{
NSpPlayerID	id;
short		playerNum;

	if (gPlayMode != PLAY_MODE_HOST)						// only the host cares if players leave
		return;


		/* BASED ON PLAYERID, FIGURE WHICH PLAYER # LEFT */

	id = inMessage->playerID;									// get NSp's playerID value
	for (playerNum = 0; playerNum < gNumPlayers; playerNum++)	// find it in the list
		if (gNetPlayerNSpPlayerID[playerNum] == id)
			goto ok1;
	DoAlert("\pHandlePlayerLeft: playerID could not be resolved.");
ok1:
			/* CLEAR THIS PLAYER'S CONTROL BITS */
			
	gPlayerControlBits[playerNum] = 0;
	gNetPlayerIsActive[playerNum] = false;
	
			/* DEC COUNT OF PLAYING PLAYERS */
			
	gNumClientsStillPlaying--;											
}


/********************* KILL GAME *********************/
//
// Terminates a network game.
//

void KillGame(void)
{
OSStatus status;
	
	if (gNetSprocketExists)
	{
		if (gGame)
		{
			status = NSpGame_Dispose(gGame, kNSpGameFlag_ForceTerminateGame);	// stop game w/o negotiating for another host (if I'm host)
			gGame = nil;	
		}
	}
}



/********************* WAIT FOR GAME COMMENCE MESSAGE ****************************/
//
// This is called after all level art has been loaded & initialized and we're just
// waiting for all the other players to do the same.
//
// Clients send a ready message to the host, and when all players are ready, the host
// sends a commence message to all the players.
//

void WaitForGameCommenceMessage(void)
{
NSpMessageHeader 	*message;
NSpMessageHeader	outMessage;
short				numReadies;
Boolean				bye = false;
OSErr				iErr;

	gNetPacketCounter = 0;											// be sure to init this!!
	gNumClientKeysReceived = 0;
	
					/*************/
					/* DO CLIENT */
					/*************/
						
	if (gPlayMode == PLAY_MODE_CLIENT)			
	{
				/* TELL HOST IM READY */
				
		NSpClearMessageHeader(&outMessage);
		outMessage.to = kNSpHostOnly;									// send only to host
		outMessage.what = kPFWaitingForCommencement;					// set message type
		outMessage.messageLen = sizeof(NSpMessageHeader);				// set message size
		iErr = NSpMessage_Send(gGame, &outMessage, kNSpSendFlag_Registered);	// send message to host
		if (iErr)
			DoFatalAlert("\pWaitForGameCommenceMessage: NSpMessage_Send failed");

				
				
				/* WAIT FOR COMMENCEMENT */
				
		while(true)
		{
			while ((message = NSpMessage_Get(gGame)) != NULL)							
			{
				switch(message->what)													
				{
					case	kPFCommenceGame:							// ready to start playing	
							bye = true;
							break;

					default:
							DoFatalAlert("\pWaitForGameCommenceMessage: Unexpected net message.");						
				}
				NSpMessage_Release(gGame, message);
				if (bye)
					return;
			}
		}
	}
	
				/***********/
				/* DO HOST */
				/***********/
	else
	{
				/* WAIT FOR ALL PLAYERS TO TELL ME THEY'RE READY */
	
		numReadies = 0;			
		while(true)
		{
			while ((message = NSpMessage_Get(gGame)) != NULL)						
			{
				switch(message->what)											
				{
					case	kPFWaitingForCommencement:
							numReadies++;
							if (numReadies >= (gNumPlayers-1))			// see if that's all the players (-1 since not waiting for myself)
								bye = true;
							break;

					default:
							DoFatalAlert("\pWaitForGameCommenceMessage: Unexpected net message.");						
				}
				NSpMessage_Release(gGame, message);								
				
							/* TELL PLAYERS TO START */
							
				if (bye)
				{
					NSpClearMessageHeader(&outMessage);
					outMessage.to = kNSpAllPlayers;									// send to all players
					outMessage.what = kPFCommenceGame;								// set message type
					outMessage.messageLen = sizeof(NSpMessageHeader);				// set message size
					iErr = NSpMessage_Send(gGame, &outMessage, kNSpSendFlag_Registered);	// send message to host
					if (iErr)
						DoFatalAlert("\pWaitForGameCommenceMessage: NSpMessage_Send failed");
					return;
				}
			}
		}
	}	
}


/*********************** SEND CUSTOM HEAD DATA ********************************/
//
// Sends the local player's custom head info to all other players.
//

static void SendCustomHeadData(void)
{
OSStatus				status;
PFCustomHeadDataMessage	*messagePtr;
Boolean					hasCustomHeadFlag;
TQ3SurfaceShaderObject	shader;
TQ3ObjectType			oType;
TQ3Mipmap				mipmap;
long					width,height,i,x,y;
unsigned long			rowBytes,dataSize,sizeRead;
Ptr						dataPtr = nil,pixelPtr;
TQ3TextureObject		texture;



			/************************/
			/* EXTRACT DATA TO PASS */
			/************************/
			//
			// NOTE: at this point, any custom head is stored in SERVER_PLAYER_NUM slot by default.
			//

	if (gCustomHeadGeometry[SERVER_PLAYER_NUM])								// see if has custom head
	{
		hasCustomHeadFlag = true;
		shader = gCustomHeadTexture[SERVER_PLAYER_NUM];						// get illegal ref to texture

				/* GET TEXTURE OBJECT FROM SHADER */
				
		status = Q3TextureShader_GetTexture(shader, &texture);
		if (status != kQ3Success)
		{
			DoAlert("\pSendCustomHeadData: Q3TextureShader_GetTexture failed!");
			goto no_custom;
		}

		oType = Q3Texture_GetType(texture);									// make sure it's legal
		if (oType != kQ3TextureTypeMipmap)									// must be a mipmap
		{
			DoAlert("\pSendCustomHeadData: Wrong texture type");
			goto no_custom;
		}
		
	
				/* GET MIPMAP FROM TEXTURE */
				
		status = Q3MipmapTexture_GetMipmap(texture, &mipmap);
		if (status != kQ3Success)
		{
			DoAlert("\pSendCustomHeadData: Q3MipmapTexture_GetMipmap failed!");
			goto no_custom;
		}
		
				/* GET MIPMAP INFO */
				
		width = mipmap.mipmaps[0].width;
		height = mipmap.mipmaps[0].height;
		rowBytes = mipmap.mipmaps[0].rowBytes;
		
		if ((width != 64) || (height != 64))								// verify size
		{
			DoAlert("\pSendCustomHeadData: mipmap dimensions are not 64x64!");
			goto no_custom;
		}
			
		
		dataSize = 64*rowBytes;												// calc size of mipmap
		dataPtr = AllocPtr(dataSize);										// alloc buffer for mipmap data
		if (dataPtr == nil)
		{
			DoAlert("\pSendCustomHeadData: AllocPtr failed!");
			goto no_custom;
		}
		
				/* COPY PIXEL DATA INTO BUFFER */
				
		status = Q3Storage_GetData(mipmap.image, 0, dataSize, (unsigned char *)dataPtr, &sizeRead);		
		if (status != kQ3Success)
		{
			DoAlert("\pSendCustomHeadData: Q3Storage_GetData failed!");
			goto no_custom;
		}

			/* CLEANUP STUFF */
			
		Q3Object_Dispose(texture);
		Q3Object_Dispose(mipmap.image);
	}
	
				/* NO CUSTOM HEAD */
	else
	{
no_custom:
		hasCustomHeadFlag = false;
	}

	

			/*****************************/
			/* PASS TEXTURE THRU NETWORK */
			/*****************************/
			
	
			/* CREATE MEMORY FOR NEW MESSAGE */
			
	messagePtr = (PFCustomHeadDataMessage *)AllocPtr(sizeof(PFCustomHeadDataMessage));
	if (messagePtr == nil)
		DoFatalAlert("\pSendCustomHeadData: AllocPtr failed!");
	
	
			/* PREP A NEW MESSAGE */
			
	NSpClearMessageHeader(&messagePtr->h);
	
	messagePtr->h.to = kNSpAllPlayers;									// send to all players
	messagePtr->h.what = kPFCustomHeadData;								// set message type
	messagePtr->h.messageLen = sizeof(PFCustomHeadDataMessage);			// set size of message
	
	messagePtr->playerNum = gMyPlayerNum;								// set player #
	messagePtr->hasCustomHead = hasCustomHeadFlag;						// set flag

	if (hasCustomHeadFlag)
	{		
		pixelPtr = dataPtr;												// copy pixels from buffer to message
		i = 0;
		for (y = 0; y < 64; y++)
		{
			for (x = 0; x < (64*2); x++)
				messagePtr->pixels[i++] = pixelPtr[x];
			pixelPtr += rowBytes;
		}
	}

			/* SEND THE MESSAGE */
				
	status = NSpMessage_Send(gGame, &messagePtr->h, kNSpSendFlag_Registered);	// must be recieved
	if (status != noErr)
		DoFatalAlert("\pSendCustomHeadData: NSpMessage_Send failed!");

				/* CLEANUP */
				
	DisposePtr((Ptr)messagePtr);										// free message memory
	if (dataPtr)
		DisposePtr(dataPtr);											// free mipmap data memory

}


/***************** GET NET PLAYER NAME ************************/
//
// Returns the name string of the input player #
//

void GetNetPlayerName(Byte playerNum, Str31 nameString)
{
NSpPlayerEnumerationPtr	players;
OSStatus	status;

			/* GET ALL PLAYER INFO */
			
	status = NSpPlayer_GetEnumeration(gGame, &players);
	if (status != noErr)
		DoFatalAlert("\pGetNetPlayerName: NSpPlayer_GetEnumeration failed!");
		
		
		/* COPY NBP NAME INTO STRING */
		
	CopyPStr(players->playerInfo[playerNum]->name, nameString);

	NSpPlayer_ReleaseEnumeration(gGame, players);
	
}




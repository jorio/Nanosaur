/*
 	File:		InputSprocket.r
 
 	Contains:	Games Sprockets: InputSprocket interfaaces
 
 	Version:	Technology:	InputSprocket 1.4
 				Release:	InputSprocket SDK, Use 3.2 Universal Headers
 
 	Copyright:	© 1996-1998 by Apple Computer, Inc., all rights reserved.
 
 	Bugs?:		For bug reports, consult the following page on
 				the World Wide Web:
 
 					http://developer.apple.com/bugreporter/
 
*/

#ifndef __INPUTSPROCKET_R__
#define __INPUTSPROCKET_R__

#ifndef __CONDITIONALMACROS_R__
#include "ConditionalMacros.r"
#endif

#define kISpDeviceClass_SpeechRecognition  'talk'
#define kISpDeviceClass_Mouse 			'mous'
#define kISpDeviceClass_Keyboard 		'keyd'
#define kISpDeviceClass_Joystick 		'joys'
#define kISpDeviceClass_Wheel 			'whel'
#define kISpDeviceClass_Pedals 			'pedl'
#define kISpDeviceClass_Levers 			'levr'
#define kISpDeviceClass_Tickle 			'tckl'				/*  a device of this class requires ISpTickle */
#define kISpDeviceClass_Unknown 		'????'

#define kISpKeyboardID_Apple 			'appl'				/*  currently this applies to _all_ keyboards */

/*	Rez interfaces for InputSprocket.
 *
 *	Specifically, this file is intended to make the development process 
 *	easier. Now the default keyboard settings can be specified in a rez
 *	file which should be easy to modify when new needs are added and/or
 *	the order of needs are changed.
 *	
 *	Note: When the ISp keyboard driver undergoes a major revision, it will
 *	almost certainly use a newer format to store it's settings. It will,
 *	however, continue to be able to read this format, but it will no longer
 *	write it.
 *	
 *	Currently, only a template is provided for keyboard settings, since
 *	keyboard is the one that is the most difficult to recreate, and since
 *	each driver uses a private format for its settings. For all other 
 *	devices, you should continue to include the 'tset' resources copied
 *	from your prefs file. Make sure you include references to them in
 * 	the set list resource ('setl').
 *
 *	The 'setl' resource contains a length value of the 'tset' is refers to.
 *	This obviously won't do for use in Rez, so ISp 1.3 and later has been
 *	modified to allow a zero value (in which case, it cannot check to
 *	verify that it is the correct length.) Because of this change:
 *	applications that use this template will _require_ InputSprocket 1.3
 *	or later... you have been warned.
 *
 */


/*----------------------------isap ¥ InputSprocket application resource ----------------*/
type 'isap'
{
		flags:
			fill bit[24];
			fill bit[6];
			boolean		doesNotCallISpInit, callsISpInit;				
			boolean		notUseInputSprocket, usesInputSprocket;

		fill long[3];
};


/*----------------------------setl ¥ a set list resource -------------------------------*/
type 'setl'
{
	unsigned longint	currentVersion = 2;
	
	unsigned longint = $$Countof(Needs);
	array Needs
	{
		pstring[63];										/* the name of the set */
		unsigned longint length;							/* the length of the set */
		literal longint	deviceClass;						/* the device class for the set */
		literal longint	deviceIdentifier;					/* the device identifier for the set */
	
		flags:
			fill bit[24];
			fill bit[3];
			fill bit;										/* set from custom (not supported) */
			fill bit;										/* set from driver (not supported) */
			boolean		notApplSet, isApplSet;				/* true if the set is from application and not a default */
			boolean		notDefaultSet, isDefaultSet;		/* true if this is the default for its device */
			fill bit;
		
		fill long[3];
		integer	resourceID;									/* the resource ID of the set */
		fill word;
	};
};



/*----------------------------tset ¥ a saved set ---------------------------------------*/
/*----------------------------only valid for keyboard tsets ----------------------------*/
type 'tset'
{
	unsigned longint	supportedVersion = 1;
	
	unsigned longint = $$Countof(ExpandedNeeds);
	wide array ExpandedNeeds
	{
keyCode:
		unsigned hex integer noKey = 0x8000,				/* virtual key code		*/
			/* NOTE: names of keys refer to the Apple Extended English Keyboard	 	*/
			/* Non-english keyboards have slightly different keycodes				*/
			/* In all cases, the actual virtual key code is stored for that device	*/
			tildeKey = 0x0032, n1Key = 0x0012, n2Key = 0x0013, n3Key = 0x0014, n4Key = 0x0015,
			n5Key = 0x0017, n6Key = 0x0016, n7Key = 0x001A, n8Key = 0x001C, n9Key = 0x0019,
			n0Key = 0x001D, minusKey = 0x001B, plusKey = 0x0018, deleteKey = 0x0033,
			
			tabKey = 0x0030, aKey = 0x0000, zKey = 0x000D, eKey = 0x000E, rKey = 0x000F, 
			tKey = 0x0011, yKey = 0x0010, uKey = 0x0020, iKey = 0x0022, oKey = 0x001F,
			pKey = 0x0023, lBraceKey = 0x0021, rBraceKey = 0x001E, backslashKey = 0x002A,
			
			capsKey = 0x0039, aKey = 0x0000, sKey = 0x0001, dKey = 0x0002, fKey = 0x0003, 
			gKey = 0x0005, hKey = 0x0004, jKey = 0x0026, kKey = 0x0028, lKey = 0x0025, 
			colonKey = 0x0029, quoteKey = 0x0027, returnKey = 0x0024,
			
			shiftKey = 0x0038, zKey = 0x0006, xKey = 0x0007, cKey = 0x0008, vKey = 0x0009,
			bKey = 0x0001,  nKey = 0x002D, mKey = 0x002E, lessThanKey = 0x002B, 
			greaterThanKey = 0x002F, slashKey = 0x002C,
			
			controlKey = 0x003B, optionKey = 0x003A, commandKey = 0x0037, spaceKey = 0x0031,
			rShiftKey = 0x003C, rOptionKey = 0x003D, rControlKey = 0x003E, 

		
			escKey = 0x0035, f1Key = 0x007a, f2Key = 0x0078, f3Key = 0x0063, f4Key = 0x0076,
			f5Key = 0x0060, f6Key = 0x0061, f7Key = 0x0062, f8Key = 0x0064,
			f9Key = 0x0065, f10Key = 0x006D, f11Key = 0x0067, f12Key = 0x006F, 
			f13Key = 0x0069, f14Key = 0x006B, f15Key = 0x0071,
			
			helpKey = 0x0072, homeKey = 0x0073, pageUpKey = 0x0074,
			delKey = 0x0075, endKey = 0x0077, pageDownKey = 0x0079
			
			upKey = 0x007E, leftKey = 0x007B, downKey = 0x007D, rightKey = 0x007C,
			
			kdpClearKey = 0x0047, kpdEqualKey = 0x0051, kpdSlashKey = 0x004B, kpdStarKey = 0x0043,
			kpd7Key = 0x0059, kpd8Key = 0x005B, kpd9Key = 0x005C, kpdMinusKey = 0x004E,
			kpd4Key = 0x0056, kpd5Key = 0x0057, kpd6Key = 0x0058, kpdPlusKey = 0x0045,
			kpd1Key = 0x0053, kpd2Key = 0x0054, kpd3Key = 0x0055, kpdEnterKey = 0x004C,
			kpd0Key = 0x0052, kpdDecimalKey = 0x0041;
		
		/* Modifiers required to be down */
		boolean		rControlOff, rControlOn;
		boolean		rOptionOff, rOptionOn;
		boolean		rShiftOff, rShiftOn;
		boolean		controlOff, controlOn;
		boolean		optionOff, optionOn;
		fill bit;											/* capsLockOff, capsLockOn*/
		boolean		shiftOff, shiftOn;
		boolean		commandOff, commandOn;

		fill byte;											
	};
};


#endif /* __INPUTSPROCKET_R__ */


//
//
// ISpPresets.r
//
//

#include "InputSprocket.r"

resource 'isap' (128)
{
	callsISpInit,
	usesInputSprocket
};

resource 'tset' (128, "Default")
{
	supportedVersion,
	{	
		commandKey, 			// Jump
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		spaceKey,				// Fire
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		shiftKey,				// Select Weapon
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		aKey,					// Jet Up
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		zKey,					// Jet Down
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		upKey,					// Forward
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		downKey,				// Backward
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		leftKey,				// Turn Left
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		rightKey,				// Turn Right
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		escKey,					// Pause
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		lessThanKey,			// Swivel Cam Left
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		greaterThanKey,			// Swivel Cam Right
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		n1Key,					// Zoom In
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		n2Key,					// Zoom Out
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		tabKey,					// Camera Mode
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		optionKey,				// Pickup / Throw
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		mKey,					// Toggle Music
		 rControlOff, rOptionOff, rShiftOff, controlOn, optionOff, shiftOff, commandOff,
		 
		bKey,					// Toggle Ambient
		 rControlOff, rOptionOff, rShiftOff, controlOn, optionOff, shiftOff, commandOff,

		plusKey,				// Raise Volume
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,

		minusKey,				// Lower Volume
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,
		 
		gKey,					// Toggle GPS
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOff,

		0x0c,					// Quit App
		 rControlOff, rOptionOff, rShiftOff, controlOff, optionOff, shiftOff, commandOn
	};
};


resource 'setl' (1000)
{
	currentVersion,
	{
		"Default", 0, kISpDeviceClass_Keyboard, kISpKeyboardID_Apple, notApplSet, isDefaultSet, 128
	};
};


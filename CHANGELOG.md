# Nanosaur changelog

- **v1.4.4 (Feb. 5, 2023)**
	- Fixed physics for very high framerates
	- Ironed out problems with first-person camera
	- New settings: aspect ratio (#25), sky color, deinonychus dentistry fix
	- Pin GPS to upper-right corner of 3D viewport in widescreen mode (#33)
	- Fix frustum culling inconsistencies
	- HiDPI (Retina) support
	- Fix faraway T-Rexes appear to skate over the terrain
	- Other stability, performance and cosmetic fixes
	- SDL 2.26.2

- **v1.4.3 (Feb. 28, 2022)**
	- Seamless terrain texturing.
	- Stability fixes.
	- macOS: Builds are now notarized.
	- Windows/Linux: New video settings.

- **v1.4.2 (Feb. 19, 2021)**
	- Full rewrite of the 3D renderer. Fixes performance on Apple Silicon and other systems (Issue #8).
	- Keyboard inputs can now be remapped (Issue #2).
	- Basic gamepad support.
	- Switching between Extreme and Easy modes is now performed in the game's settings screen (Issue #9, thanks @shpface).
	- New options in game settings (music, ambient, debug title bar).
	- Better fade in/out implementation.

- **v1.4.1 (Dec. 31, 2020)**
	- Nanosaur Extreme is now playable.
	- Note: this is the last version of Nanosaur that uses Quesa, i.e. the last version containing "true" QuickDraw 3D code.

- **v1.4.0 (Nov. 13, 2020)**
	- Updated the game to run on today's systems at arbitrary resolutions.

---

- **v1.3.4 (Oct. 2000)** Last official release for Mac OS 8-9.

- **v1.3.3**

- **v1.3.2**

- **v1.3.1** Fixed yet more problems with QT 4.1’s sound.

- **v1.3**   The game is now shareware. We had to start charging money because hosting the files was becoming extremely expensive for a game that we were giving away for free.

- **v1.2**   Implemented a work-around for a Sound Manager bug in Quicktime 4.1. Apparently QT 4.1 cannot play more than 4 or 5 sound effects simultaneously without running into errors, whereas, QT 4.0 and older could play a dozen or so.

- **v1.1.9** Fixed problem on machines with multiple monitors. The status bar sprites would not get drawn in the correct location.

- **v1.1.8** I can't remember what got fixed in this rev.

- **v1.1.7** Fixed “Type 2” crash problem with iBooks and 2nd Generation iMacs.

- **v1.1.6** Recompiled with CW Pro 5

- **v1.1.5** Tweaked some Draw Sprocket code.

- **v1.1.4** Fixed a memory leak.

- **v1.1.3** Fixed bug where game would quit if Q key was pressed while entering a high score name. Fixed memory leak problem with QuickDraw 3D 1.6.

- **v1.1.2** Fixed the Toggle Ambient & Music key settings to the correct defaults.

- **v1.1.1** Added “Bugdom” plug.

- **v1.1**   Added Input Sprocket Support so the game can be played with any input device and the keys can be reassigned.

- **v1.0.9** Fixed more minor bugs and had minor performance increases.

- **v1.0.8** Now gives a meaningful error if Data folder is missing. Increased stack size of application to prevent some reported crashing. Fixed bug related to picking up eggs and putting them in the teleporter. When fog is disabled, clear color is black instead of white on the 4MB version of the game. Fixed problem with player skidding when the game’s frame rate exceeded ~50 fps. Fixed problem with enemies being culled when they were actually visible. Other Misc bug fixes.

- **v1.0.7** Minor internal tweaks for iMac and Powerbooks.

- **v1.0.6** Fixed more assorted crash bugs, and fixed some sound anomalies and other minor things.

- **v1.0.5** Fixed crash bug where game would occasionally lock up after playing the win or lose movies.

- **v1.0.4** Incredibly minor tweak to RAVE blending modes so that shadows will appear correct with new ATI 4.30 drivers. Also tweaked memory check so only looks for 1.5 free Megs of VRAM instead of 2 free Megs. That should help out people with Powerbooks and non-multisync monitors.

- **v1.0.3** Fixed clipping bugs which were causing software renderer to crash. 3D accelerator cards might run a little faster now as well.

- **v1.0.2** Fixed a few crash bugs and memory leaks.

- **v1.0.1** Minor fix to ATI version detection to disable fog on old drivers. Fixed Type 12 crash bug.

- **v1.0 (Apr. 1998)** We shipped it April 6, 1998!

# Experimental Nanosaur mobile web port

This builds the original C/C++ game with SDL 3.2.8 and Emscripten 4.0.20.
The browser runtime in [`common/`](common/README.md) is identical across Bugdom,
Bugdom 2, Nanosaur and Nanosaur 2. Game identity and action labels live in
`game.js`; engine-specific mappings and camera hooks stay in the game sources.

## Build and run

Install CMake 3.21+, Python 3.9+ and Emscripten 4.0.20, activate its environment,
then run:

```sh
git submodule update --init --recursive
git clone --depth 1 --branch release-3.2.8 https://github.com/libsdl-org/SDL.git extern/SDL
bash web/build.sh
python3 web/serve.py --port 8000
```

Open `http://localhost:8000`; a phone on the same Wi-Fi can use the computer's
LAN IP, and an Android emulator can use `http://10.0.2.2:8000`. Rebuild and reload
after source edits. PWA installation/offline play needs HTTPS or localhost;
see the [PWA/TWA hosting and signing instructions](common/PWA.md).

## Controls

Tap Play, then Continue to skip intros or select the highlighted menu entry.
The left thumbstick moves/steers; right-side dragging controls the camera
(or flight steering in Nanosaur 2), and pinching changes camera distance.
The right-hand action column contains **Fire, Jetpack, Jump, Grab/drop, Weapon**. The toolbar toggles touch
controls, master mute, fullscreen and pause. Mute persists in this browser.
Original keyboard bindings and SDL gamepads remain available; `game.js` lists
additional browser keyboard aliases. Touch controls appear during gameplay,
release on interruptions, and avoid pointer lock.

Preferences, high scores and existing save slots use IndexedDB, with a
session-only fallback if browser storage is blocked. Each game uses a distinct
storage mount and mute key. Clearing browser data removes local saves. The
port preserves the original game's save opportunities and does not add autosave.

## Tests

```sh
bash web/test.sh
python3 -m venv /tmp/pangea-web-tests
/tmp/pangea-web-tests/bin/pip install playwright==1.65.2
/tmp/pangea-web-tests/bin/playwright install chromium
/tmp/pangea-web-tests/bin/python web/tests/browser_test.py
python3 web/common/check_sync.py ../Bugdom ../Bugdom2 ../Nanosaur ../Nanosaur2
```

The fast suite covers gesture ownership and sanitized texture conversion. The
browser suite checks first-level startup, phone layouts, pause, held action
ownership, resize cancellation, mute, saved files, blocked storage, runtime
failure UI, manifest metadata and offline startup. It starts its own localhost
server and uses temporary browser profiles. Native builds retain their existing
renderer and controls; web-only changes are conditional on Emscripten.

## Limits

This is an experimental compatibility port using legacy OpenGL emulation.
CSS-pixel rendering limits phone GPU load. Sphere-reflection texture coordinates
are generated on the CPU because WebGL has no fixed-function texture generation.
All levels, prolonged sessions, Safari and multiplayer need device playtesting.
Hardware shutter-glasses stereo is not supported in WebGL. Context loss requires
a reload. Keep the repository's original credits and license with deployments.

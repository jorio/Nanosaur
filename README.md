# Nanosaur source port

This is _Nanosaur_ for modern operating systems (macOS, Windows, Linux). This version, at https://github.com/jorio/nanosaur, is approved by Pangea Software.

- **Get builds for macOS and Windows here:** https://github.com/jorio/nanosaur/releases
- Arch Linux users, get [`nanosaur` from the AUR](https://aur.archlinux.org/packages/nanosaur).
- Other systems: please read [BUILD.md](BUILD.md) to build the game yourself. 

![Screenshot](docs/screenshot.png)

## About this port

### Context

Nanosaur is a 1998 Macintosh game by Pangea Software. In it, you’re a cybernetic dinosaur from the future who’s sent back in time 20 minutes before a giant asteroid hits the Earth. And you get to shoot at T-Rexes with nukes.

Nanosaur was bundled with the original iMac and ran on Mac OS 8. It’s also notable for being a prominent showcase of QuickDraw 3D’s capabilities, which was Apple’s high-level 3D graphics API during the 90s.

In 1999, Pangea released [Nanosaur’s source code](http://www.pangeasoft.net/nano/nanosource.html) to the public. This port is based on that release.

### Port philosophy

I took a conservative approach to port the game, in a similar spirit to “Chocolate Doom”. I aimed to accurately maintain the behavior of the original game. The port has some minor quality-of-life improvements such as support for arbitrary resolutions. Modifications to the gameplay or presentation are out of the scope of this project.

To make it easier to port the game, I wrote an implementation of parts of the Macintosh Toolbox API, which I called “[Pomme](https://github.com/jorio/Pomme)”. You can think of Pomme as a cross-platform reimagining of [Carbon](https://en.wikipedia.org/wiki/Carbon_(API)), albeit at a much smaller scope.

The first release of this port used a [custom fork](https://github.com/jorio/Quesa) of [Quesa](https://github.com/jwwalker/Quesa) to render the game’s 3D graphics. Quesa is an independent implementation of QuickDraw 3D; it was incredibly useful to get the game up and running initially. Nanosaur has switched to a tailor-made renderer as of version 1.4.2.

### Credits

Nanosaur™ © 1998 Pangea Software, Inc.
- Programming: Brian Greenstone
- Artwork: Scott Harper 
- Music: Mike Beckett, Jens Nilsson
- Cinematics: Chris Ashton
- Enhanced update: Iliyas Jorio

Nanosaur was updated and re-released here (https://github.com/jorio/nanosaur) with permission by Pangea Software.

### See also: Bugdom

If you like Pangea games, check out [my remastered version of the original Bugdom](https://github.com/jorio/Bugdom).
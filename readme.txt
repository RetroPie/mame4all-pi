=============================================================
MAME4ALL for Pi 1.0 (01 March, 2013) by Squid
=============================================================


INTRODUCTION
------------

This is a MAME Raspberry Pi port based on Franxis MAME4ALL which is itself based on the
MAME 0.37b5 emulator by Nicola Salmoria.
To see MAME license see the end of this document (chapter 18).
It emulates all arcade games supported by original MAME 0.37b5 plus some additional
games from newer MAME versions.

This version emulates 2270 different romsets.

Although this is an old version of MAME it plays much faster than the newer versions
and as the Pi is rather CPU underpowered it was chosen to get as many games working
at full speed as possible (full speed means 100% with no frame skip). It also 
plays most of the games I'm interested in playing!

This is a highly optimised version for the Raspberry Pi, using dispmanx for graphics, ALSA 
for sound and SDL for input. 

Official web page for news, source, additional information:
http://code.google.com/p/mame4all-pi/

(No asking for ROMS, problems with ROMS)


CONTROLS
--------

Controls are currently only available for the keyboard (No joystick support yet).
These are the standard MAME key definitions as follows (not all MAME keys are supported):
- Keys LControl, LAlt, Space, LShift are the fire buttons
- Cursors Keys for up, down, left and right
- Keys 1 & 2 to start 1 or 2 player games
- Keys 5 & 6 Insert credits for 1P or 2P
- Key Escape to quit
- Key TAB to bring up the MAME menu
- Function Keys: F11 show fps, F10 toggle throttle, F5 cheats, Shift F11 show profiler
- Key P to pause

NOTE: To type OK when MAME requires it, press LEFT and then RIGHT.


Pi EMULATION SPEED AND SOUND
----------------------------

I highly recommend overclocking your Raspberry Pi to gain maximum performance
as MAME is very CPU intensive and overclocking will make most games run at full speed.

My overclocking settings which work well, (/boot/config.txt)
arm_freq=950
core_freq=350
sdram_freq=500

NOTE: This is with a recent 512MB Model B.

I'd also recommend a minimum of 64MB for the GPU RAM allocation (gpu_mem=64).

If your sound is too quiet then do the following to fix that:
First get the playback device, type "amixer controls"
This will show the numid for the playback device, probably 3.
Now set the volume, type "amixer cset numid=3 90%".
Then reboot to make it permanent.


INSTALLATION
------------

mame        -> MAME and frontend.
cheat.dat   -> Cheats definition file
hiscore.dat -> High Scores definition file
artwork/    -> Artwork directory
cfg/        -> MAME configuration files directory
frontend/   -> Frontend configuration files
hi/         -> High Scores directory
inp/        -> Game recordings directory
memcard/    -> Memory card files directory
nvram/      -> NVRAM files directory
roms/       -> ROMs directory
samples/    -> Samples directory
skins/      -> Frontend skins directory
snap/       -> Screen snapshots directory
sta/        -> Save states directory

To run MAME simple run the "mame" executable. At the command line "./mame".
This runs the GUI frontend by default. To simply run MAME without the GUI
enter "./mame -nogui {gamerom}" where "{gamerom}" is the game to run.

It will work in X-Windows or in the Console. I'd recommend running in the Console.


SUPPORTED GAMES
---------------

Folder names or ZIP file names are listed on "gamelist.txt" file.
Romsets have to be MAME 0.37b5 ones (July 2000).
Additionaly there are additional romsets from newer MAME versions.

Please use "clrmame.dat" file to convert romsets from other MAME versions to the ones used by
this version, using ClrMAME Pro utility, available in next webpage:

http://mamedev.emulab.it/clrmamepro/

NOTE: File and directory names in Linux are case-sensitive. Put all file and directory names
using lower case!.


SOUND SAMPLES
-------------

The sound samples are used to get complete sound in some of the oldest games.
They are placed into the 'samples' directory compressed into ZIP files.
The directory and the ZIP files are named using low case!.

The sound samples collection can be downloaded in the following link:
http://dl.openhandhelds.org/cgi-bin/gp2x.cgi?0,0,0,0,5,2511

You can also use "clrmame.dat" file with ClrMAME Pro utility to get the samples pack.


ARTWORK
-------

Artwork is used to improve the visualization for some of the oldest games. Download it here:
http://dl.openhandhelds.org/cgi-bin/gp2x.cgi?0,0,0,0,5,2512


ORIGINAL CREDITS
----------------

- MAME 0.37b5 original version by Nicola Salmoria and the MAME Team (http://www.mame.net).

- Z80 emulator Copyright (c) 1998 Juergen Buchmueller, all rights reserved.

- M6502 emulator Copyright (c) 1998 Juergen Buchmueller, all rights reserved.

- Hu6280 Copyright (c) 1999 Bryan McPhail, mish@tendril.force9.net

- I86 emulator by David Hedley, modified by Fabrice Frances (frances@ensica.fr)

- M6809 emulator by John Butler, based on L.C. Benschop's 6809 Simulator V09.

- M6808 based on L.C. Benschop's 6809 Simulator V09.

- M68000 emulator Copyright 1999 Karl Stenerud.  All rights reserved.

- 80x86 M68000 emulator Copyright 1998, Mike Coates, Darren Olafson.

- 8039 emulator by Mirko Buffoni, based on 8048 emulator by Dan Boris.

- T-11 emulator Copyright (C) Aaron Giles 1998

- TMS34010 emulator by Alex Pasadyn and Zsolt Vasvari.

- TMS9900 emulator by Andy Jones, based on original code by Ton Brouwer.

- Cinematronics CPU emulator by Jeff Mitchell, Zonn Moore, Neil Bradley.

- Atari AVG/DVG emulation based on VECSIM by Hedley Rainnie, Eric Smith and Al Kossow.

- TMS5220 emulator by Frank Palazzolo.

- AY-3-8910 emulation based on various code snippets by Ville Hallik, Michael Cuddy,
  Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.

- YM-2203, YM-2151, YM3812 emulation by Tatsuyuki Satoh.

- POKEY emulator by Ron Fries (rfries@aol.com). Many thanks to Eric Smith, Hedley Rainnie and Sean Trowbridge.

- NES sound hardware info by Jeremy Chadwick and Hedley Rainne.

- YM2610 emulation by Hiromitsu Shioya.


PORT CREDITS
------------

- Ported to Raspberry Pi by Squid.

- Original MAM4ALL port to GP2X, WIZ and CAANOO by Franxis (franxism@gmail.com) based on source code MAME 0.37b5 (dated on july 2000).

- ALSA sound code is borrowed from RetroArch (http://themaister.net/retroarch.html)


DEVELOPMENT
-----------

March 01, 2013:
- Version 1.0: First release

TODO
----
- Add joystick support
- Add rotation support
- Add configurables


KNOWN PROBLEMS
--------------

- Not perfect sound or incomplete in some games. Sometimes simply quiting a game and 
  restarting can fix the sound - believe this is due to ALSA Pi driver bugs.

- Make sure nothing is running in the background. Best to run in the console instead
  of X-Windows.

- The SDL input drivers are a little buggy, if input suddenly stops, reboot the Pi.

- Slow playability in modern games.

- Memory leaks. In case of errors/crashes reboot your Pi.


THANKS TO
---------
tecboy for testing and patience!


INTERESTING WEBPAGES ABOUT MAME
-------------------------------

- http://www.mame.net/
- http://www.mameworld.net/


SKINS
---------

The frontend graphic skin used in the emulator is located in these files:
skins/rpisplash.bmp   -> Intro screen
skins/rpimenu.bmp     -> Screen to select games

Bitmaps have to be 640x480 pixels - 256 colors (8 bit). In the game selection screen, the
text is drawn with color 255 of the palette with a right-under border with color 0.


MAME LICENSE
----------------

http://www.mame.net
http://www.mamedev.com

Copyright 1997-2009, Nicola Salmoria and the MAME team. All rights reserved. 

Redistribution and use of this code or any derivative works are permitted provided
that the following conditions are met: 

* Redistributions may not be sold, nor may they be used in a commercial product or activity. 

* Redistributions that are modified from the original source must include the complete source
code, including the source code for all components used by a binary built from the modified
sources. However, as a special exception, the source code distributed need not include 
anything that is normally distributed (in either source or binary form) with the major 
components (compiler, kernel, and so on) of the operating system on which the executable
runs, unless that component itself accompanies the executable. 

* Redistributions must reproduce the above copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
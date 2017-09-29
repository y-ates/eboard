eboard README
-------------

eboard is a chess board interface for ICS (Internet Chess
Servers) and chess engines.

for quick compilation / installation instructions, see the INSTALL file.

Currently it supports the following protocols / engines:

- Generic XBoard protocol v2-compliant engines
   Almost full support. Engines that behave on the edge of
   protocol may fail. Works with GNU Chess 5.

- Particular Engine Support (most of them comply with the
  XBoard protocol, but eboard supports additional features):

  - GNU Chess 4
  - Crafty
  - Sjeng (multi-variant engine)

  See the eboard site for links to get them.

- Crafty
  Full support.
  Tested with 18.9 thru 18.14, won't work with versions earlier
  than 18.x Crafty is *NOT* distributed with eboard, see
  Documentation/Crafty.txt for information on installing Crafty and
  the game books.

- Direct play across a network - one eboard connects to another
  eboard over a TCP/IP network (like the Internet).

- FICS
  Support most features.
  Current status is:
  It will allow you to play regular, suicide, losers, atomic,
  crazyhouse and bughouse chess games. 
  Wild variants are supported (tested with Fischer Random and Wild/5,
  but weird castlings are not directly supported and you may have to
  type in the castling moves by hand)
  It will observe games.
  It will examine games (but the move list may not be retrieved correctly)
  Supports premove and drag-and-drop.
  Nice, customizable, colorization of FICS output.
  bsetup mode not supported by the interface yet (but you can
  enter bsetup and add pieces with FICS commands)

  Known issues: simuls are not yet supported.
  If you set nowrap on on FICS (this is not the default), some
  very long lines (like 'in 1') can make eboard crash. Just don't
  mess with that FICS variable and you'll be fine.

  FICS is a no-charge service, operating since 1995, and over
  these years it has fostered the building of a huge community
  of chess enthusiasts. It's a great place to play chess and
  to make friends. Besides regular chess, it supports bughouse,
  crazyhouse, suicide, losers and several wild variants (in wild
  rules are the same as regular chess but the starting position
  isn't)

  FICS recently introduced thematic games - you don't get the initial
  moves but rather an ECO code. You can play and watch those with eboard,
  but PGN saving and game browsing (moving back and forth while watching)
  are still quite nuts. This should get fixed soon.

  For timeseal support see Documentation/FICS-Timeseal.txt
  For information on getting an account to play at FICS, visit
  FICS at http://www.freechess.org

  If you have problems with firewalls, the best approach is to use
  a third-party TCP port-forwarding tool. I have a nice experience
  with portfwd (http://portfwd.sourceforge.net), but many other
  packages like this exist.

  For automatic login scripts for FICS see Documentation/Scripts.txt

Features:
---------

- Scroll locking on text pane: if you scroll up in the text
  pane, it won't auto-scroll to the bottom when new output
  comes from the server.

- Input history. The Up and Down arrow keys work like the
  bash history in the text entry box.

- The board can be resized on the fly.

- The piece set can be changed on the fly.

- Scripting

- PGN reading/writing

- Seek Graph (FICS)

- Multiple text panes (Windows|Detached Console)

- National Language Support (portuguese, german, spanish, czech and 
  italian so far)

To change the language, make sure you performed 'make install' and
set the environment variable LANGUAGE to the language code
(de for German, pt_BR for brazilian Portuguese, es for Spanish, etc.)
Under bash (the most common shell on Linux), it can be done with

export LANGUAGE=de

(eboard checks the LC_MESSAGES, LC_ALL, LANGUAGE and LANG
environment variables for language settings, in this order)

You can use, modify and redistribute eboard under the terms of
the GNU General Public License, version 2 or any later version
published by the Free Software Foundation. The license is
included in the COPYING file. 

To compile/install it:

./configure
make
(become root)
make install

For further instructions see the INSTALL file. You'll need:

- the GTK+ library, version 2.2.0

- the C++ standard library (libstdc++)

eboard is developed by Felipe Bergo <fbergo at gmail.com>.

The eboard web site is at

  http://eboard.sourceforge.net

There is an alternative mirror for the source at

  ftp://ftp.seul.org/pub/chess/eboard

and the project page (for bug reports, mailing list, CVS
access) is

  http://sourceforge.net/projects/eboard

Have fun!

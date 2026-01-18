ssf - simple system fetcher
===============================

ssf prints a small set of system facts next to a coloured ASCII logo to stdout.
The code keeps to plain POSIX interfaces so it runs on Linux and OpenBSD.

Requirements
------------

A POSIX environment with a C99 compiler and make(1).

Installation
------------

Edit config.mk to match your local setup (ssf installs into /usr/local
by default). Afterwards run:

	make
	make install

Usage
-----

Simply run ssf with no arguments for the standard display. Optional
flags mimic the original interface:

	-m  print values without labels
	-s  print a short subset of fields
	-k  print only the value for a specific key
	-h  show usage help
	-v  show the version string

Customisation
-------------

ASCII definitions live in the ascii file and can be edited to taste.
Feature toggles, compiler flags, and install paths are configured through
config.mk.

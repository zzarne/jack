### jack_globals: Global storage space for
### jack - extract audio from a CD and encode it using 3rd party software
### Copyright (C) 2002  Arne Zellentin <zarne@users.sf.net>

### This program is free software; you can redistribute it and/or modify
### it under the terms of the GNU General Public License as published by
### the Free Software Foundation; either version 2 of the License, or
### (at your option) any later version.

### This program is distributed in the hope that it will be useful,
### but WITHOUT ANY WARRANTY; without even the implied warranty of
### MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
### GNU General Public License for more details.

### You should have received a copy of the GNU General Public License
### along with this program; if not, write to the Free Software
### Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

from jack_constants import *
from jack_config import cf
from jack_generic import info, error, warning, debug, DEBUG

# globals
dae_queue = []                      # This stores the tracks to rip
enc_queue = []                      # WAVs go here to get some codin'
enc_running = 0                     # what is going on?
dae_running = 0                     # what is going on?
progress_changed = 0                # nothing written to progress, yet
revision = 0                        # initial revision of freedb data

#misc stuff
from ID3 import ID3
tmp = ID3("/dev/null")
id3genres = tmp.genres
del tmp


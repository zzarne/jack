# Based on:
# cdrommodule.c
# Python extension module for reading in audio CD-ROM data
#
# Please port me to other OSes besides Linux, Solaris, OpenBSD,
# and FreeBSD!
#
# See the README for info.
#
# Written 17 Nov 1999 by Ben Gertzfield <che@debian.org>
# This work is released under the GNU GPL, version 2 or later.
#
# FreeBSD support by Michael Yoon <michael@yoon.org>
# OpenBSD support added by Alexander Guy <alex@andern.org>
# Darwin/MacOS X support added by Andre Beckedorf <andre@beckedorf.net>
#
# Thanks to Viktor Fougstedt <viktor@dtek.chalmers.se> for info
# on the <sys/cdio.h> include file to make this work on Solaris!
#
# ported from C to python by Arne Zellentin
# non-portable -- 64 bit linux ioctls only

import io
import fcntl
import struct

# #define CDROMREADTOCHDR         0x5305 /* Read TOC header
#                                          (struct cdrom_tochdr) */
CDROMREADTOCHDR =       0x5305
CDDB_READ_TOC_HEADER_FLAG = CDROMREADTOCHDR

# #define CDROMREADTOCENTRY       0x5306 /* Read TOC entry
#                                            (struct cdrom_tocentry) */
CDROMREADTOCENTRY =     0x5306
CDDB_READ_TOC_ENTRY_FLAG = CDROMREADTOCENTRY

# /* The leadout track is always 0xAA, regardless of # of tracks on disc */
# #define CDROM_LEADOUT           0xAA
CDROM_LEADOUT =         0xAA
CDDB_CDROM_LEADOUT = CDROM_LEADOUT

# #define CDROM_MSF 0x02 /* "minute-second-frame": binary, not bcd here! */
CDROM_MSF =               0x02

# /* This struct is used by the CDROMREADTOCENTRY ioctl */
# struct cdrom_tocentry
# {
        # __u8    cdte_track;
        # __u8    cdte_adr        :4; /* uses only 4 bits of the __u8 */
        # __u8    cdte_ctrl       :4;
        # __u8    cdte_format;
        # union cdrom_addr cdte_addr;
        # __u8    cdte_datamode;
# }; // pack: BBB (BBB|l) B
cdrom_tocentry = "BBB BBBBBBBB B"

# /* Address in either MSF or logical format */
# union cdrom_addr
# {
        # struct cdrom_msf0       msf;
        # int                     lba;
# }; // pack: BBB|l

# /* Address in MSF format */
# struct cdrom_msf0
# {
        # __u8    minute;
        # __u8    second;
        # __u8    frame;
# }; // BBB

def toc_header(f):
    cdrom_fd = f.fileno()

    # struct CDDB_TOC_HEADER_STRUCT hdr -> cdrom_tochdr
    # struct cdrom_tochdr
    # {
            # __u8    cdth_trk0;      /* start track */
            # __u8    cdth_trk1;      /* end track */
    # };

    hdr = bytearray(2)

    fcntl.ioctl(cdrom_fd, CDDB_READ_TOC_HEADER_FLAG, hdr)

    return list(hdr)

def toc_entry(f, track):
    cdrom_fd = f.fileno()

    #     #define CDDB_TRACK_FIELD cdte_track
    #     #define CDDB_MSF_FORMAT CDROM_MSF

    #     #define CDDB_FORMAT_FIELD cdte_format

    #     entry.CDDB_TRACK_FIELD = track;
    #     entry.CDDB_FORMAT_FIELD = CDDB_MSF_FORMAT;

    entry = bytearray(struct.pack(cdrom_tocentry,
            track, 0, CDROM_MSF,
            0, 0, 0, 0, 0, 0, 0, 0,
            0))

    fcntl.ioctl(cdrom_fd, CDDB_READ_TOC_ENTRY_FLAG, entry)

    return struct.unpack(cdrom_tocentry, entry)[4:7]

def leadout(f):
    return toc_entry(f, CDDB_CDROM_LEADOUT)

def open(cdrom_device, cdrom_open_flags=0):
    if not cdrom_open_flags == 0:
        raise IOError("open: unsupported cdrom_open_flags=" + repr(cdrom_open_flags))

    return io.open(cdrom_device)

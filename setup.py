#!/usr/bin/env python3

"""Setup script for the jack module distribution."""

from setuptools import setup

setup( # Distribution meta-data
    name = "jack",
    version = "3.1.1",
    description = "A frontend for several cd-rippers and mp3 encoders",
    author = "Arne Zellentin",
    author_email = "zarne@users.sf.net",
    url = "https://github.com/zzarne/jack",

    python_requires='>3',

    # Description of the modules and packages in the distribution
    py_modules = [
        'jack_CDTime', 'jack_TOC', 'jack_TOCentry', 'jack_argv',
        'jack_checkopts', 'jack_children', 'jack_config', 'jack_constants',
        'jack_display', 'jack_encstuff', 'jack_freedb', 'jack_functions',
        'jack_generic', 'jack_globals', 'jack_helpers', 'jack_init',
        'jack_m3u', 'jack_main_loop', 'jack_misc', 'jack_mp3',
        'jack_playorder', 'jack_plugins', 'jack_prepare', 'jack_progress',
        'jack_rc', 'jack_ripstuff', 'jack_status', 'jack_t_curses',
        'jack_t_dumb', 'jack_tag', 'jack_targets', 'jack_term', 'jack_utils',
        'jack_version', 'jack_workers', 'CDDB', 'DiscID', 'cdrom',
    ],

    install_requires=[
        'eyed3',
    ]
)

print("If you have installed the modules, copy jack to some place in your $PATH,")
print("like /usr/local/bin/.")

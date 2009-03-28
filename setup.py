#!/usr/bin/env python

from distutils.core import setup

setup(name="gfceux",
		version="2.1",
		scripts = ['gfceux'],
		packages = ['gfceux'],
    		package_dir = {'gfceux': 'src'},
		data_files=[
		    ('share/gfceux/',
		    ['data/gfceux.xml', 'data/gfceux_big.png', 'data/gfceux.png',
		    'COPYING']),
            ('share/pixmaps/',
            ['data/gfceux.png']),
            ('share/man/man1/',
            ['data/gfceux.1']),
            ('share/applications/',
            ['data/gfceux.desktop'])],
		author = "Lukas Sabota",
		author_email = "ltsmooth42 _at_ gmail.com",
		url = "http://dietschnitzel.com/gfceu"
		)


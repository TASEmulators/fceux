#!/usr/bin/env python
from distutils.core import setup
setup(name="gfceux",
		version="2.0.4",
		scripts = ['gfceux'],
		data_files=[('share/gfceux/',['gfceux.xml', 'gfceux_big.png', 'gfceux.png']),
                ('share/pixmaps/', ['gfceux.png']),
                ('share/man/man1/', ['gfceux.1']),
      			    ('share/applications/', ['gfceux.desktop'])],
		author = "Lukas Sabota",
		author_email = "ltsmooth42 _at_ gmail.com",
		url = "http://dietschnitzel.com/gfceu"
		)


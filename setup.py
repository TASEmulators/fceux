#!/usr/bin/env python2
from distutils.core import setup
setup(name="gfceu",
		version="0.6.1",
		scripts = ['gfceu'],
		data_files=[('share/gfceu/',['gfceu.glade', 'gfceu_big.png', 'gfceu.png']),
                ('share/pixmaps/', ['gfceu.png']),
                ('share/man/man1/', ['gfceu.1']),
      			    ('share/applications/', ['gfceu.desktop'])],
		author = "Lukas Sabota",
		author_email = "ltsmooth42 _at_ gmail.com",
		url = "http://fceux.com"

		)


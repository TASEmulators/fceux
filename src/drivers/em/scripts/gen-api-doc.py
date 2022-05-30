#!/usr/bin/env python3
import re

head = """\
# em-fceux API\
"""

print(head)

with open('index.d.ts') as f:
  recording = False
  comment = []
  for line in f.readlines():
    s = line.strip()
    if s.startswith('/*'):
      comment = [ '' ]
      recording = True
    elif s.startswith('*/'):
      recording = False
    elif recording:
      comment.append(s)
      if len(comment) == 3:
        comment.insert(2, '')
    elif len(comment) > 0 and '=>' in s:
      comment.insert(0, '\n\n## ' + re.sub(r'<[^>]*>', '', s[0:-1]))
      print('\n'.join(comment))
      comment = []

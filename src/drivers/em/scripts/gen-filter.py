#!/usr/bin/env python
import sys, math
class M: pass

if len(sys.argv) < 3:
  print 'tool to generate gaussian filter kernel samplers\n'
  print 'usage: gen-filter.py <radius> <threshold>\n'
  exit(1)

# gaussian filter radius
R = float(sys.argv[1])
# taps w/ weight under this threshold are eliminated (reduces processing time)
THRESHOLD = float(sys.argv[2])

class Filter:

  def normalize(self):
    t = sum(self.m)
    self.m = [v / t for v in self.m]

  # Generate a 1D gaussian filter of specified radius.
  # Uses sigma = radius / 2 just like Mathematica/Wolfram:
  #   http://reference.wolfram.com/language/ref/GaussianMatrix.html
  # Result is normalized.
  def gaussian(self, r):
    self.w = s = int(math.ceil(2 * r))
    self.h = 1
    self.cx = cx = (s-1) / 2.0
    self.cy = 0.0
    a = r / 2.0
    a2 = -2.0 * a*a
    self.m = [math.exp(((i-cx) ** 2.0) / a2) for i in range(s)]
    self.normalize()

  # Generate 1D 16-samples wide sinc Lancoz2 filter for 1:4 decimation.
  # Filter is specified on p.11 (the 1/2-phase version) in
  #   Turkowski, K., 'Filters for Common Resampling Tasks'
  # Result is normalized.
  def sincLancoz2Dec4(self):
    self.w = w = 16
    self.h = 1
    self.cx = (w-1) / 2.0 
    self.cy = 0.0
    self.m = [ -0.001, -0.01, -0.02, -0.015, 0.025, 0.099, 0.183, 0.239 ]
    self.m += reversed(self.m)
    self.normalize()

  # Generate 1D sqrt(2)-gaussian 1:4 decimation filter 16 wide.
  # Filter is specified on p.8 (the 1/2-phase version) in
  #   Turkowski, K., 'Filters for Common Resampling Tasks'
  # Result is normalized.
  def gaussianSqrt2Dec4(self):
    self.w = w = 16
    self.h = 1
    self.cx = (w-1) / 2.0 
    self.cy = 0.0
    self.m = [ 0.001, 0.004, 0.012, 0.029, 0.058, 0.097, 0.137, 0.163 ]
    self.m += reversed(self.m)
    self.normalize()

  # Generate 1D Laplacian of Gaussian sharpening filter of 9x1 w/ sigma=sqrt(2).
  # Result is normalized.
  def laplacianOfGaussian9(self):
    self.w = w = 9
    self.h = 1
    self.cx = (w-1) / 2.0 
    self.cy = 0.0
    self.m = [-0.0018176366993004163, -0.007572327365550231, -0.005295961178694143, 0.03521390903683393, 0.9589440324134216, 0.03521390903683393, -0.005295961178694143, -0.007572327365550231, -0.0018176366993004163]
    #self.m += reversed(self.m)
    self.normalize()

  # Creates 2D filter by convolving 1D filter with itself. Original filter must have h=1.
  # Result is normalized.
  def convolve(self):
    if self.h != 1: return
    m = self.m
    self.w = self.h = s = len(m)
    self.cx = self.cy = (s-1) / 2.0
    self.m = [0 for _ in range(s*s)]
    for j in range(s):
      for i in range(s):
        self.m[j*s+i] = m[i] * m[j]
    self.normalize()

  # Inverse convolutes 1D filter from a 2D one. Filter must be separable.
  # Result is normalized.
  def deconvolve(self):
    w = self.w
    h2 = self.h / 2
    self.h = 1
    self.cy = 0.0
    self.m = [self.m[h2*w+i] for i in range(w)]
    self.normalize()

  def printFilter(self):
    print 'kernel:'
    w = self.w; h = self.h
    print '  ',
    for i in range(w):
      print '%-8d' % (i),
    print ''
    for j in range(h):
      print '%-2d' % (j),
      for i in range(w):
        print '%-8.5f' % (self.m[j*w+i]),
      print ''
  
  def calcTaps(self, threshold):
    w2 = (self.w + 1) / 2; h2 = (self.h + 1) / 2
    w = 2 * w2; h = 2 * h2
    m = [0 for _ in range(w*h)]
    for j in range(h):
      for i in range(w):
        if j < self.h and i < self.w:
          m[j*w + i] = self.m[j*self.w + i]
    taps = [M() for _ in range(w2*h2)]
    for j in range(h2):
      for i in range(w2):
        p = j*w2 + i
        k = 2 * (j*w + i)
        a = m[k]
        b = m[k+1]
        c = m[k+w]
        d = m[k+w+1]
        mag = taps[p].mag = a + b + c + d
        a = a / mag
        b = b / mag
        c = c / mag
        d = d / mag
        dx = b + d
        dy = c + d
        taps[p].x = 2*i - self.cx + dx
        taps[p].y = 2*j - self.cy + dy
    Filter.normalizeTaps(taps)
    taps = [t for t in taps if t.w > threshold]
    Filter.normalizeTaps(taps)
    return taps

  @staticmethod
  def normalizeTaps(taps):
    mag_sum = sum(t.mag for t in taps)
    for t in taps:
      t.w = t.mag / mag_sum
  
  @staticmethod
  def printTaps(taps):
    print 'taps: (%d)' % (len(taps))
    for t in taps:
      print '(%10.6f,%10.6f) w:%9.6f mag:%9.6f' % (t.x, t.y, t.w, t.mag)

  @staticmethod
  def printTapsCode(taps):
    """
    print 'code: (%d)' % (len(taps))
    for i, t in enumerate(taps):
      print '\t"UV(%2d, vec2(%9f, %9f))\\n"' % (i, t.x, t.y)
    for i, t in enumerate(taps):
      print '\t"SAMPLE(%2d, %f)\\n"' % (i, t.w)
    """
    print 'static const float s_downsample_offs[] = {',
    for i, t in enumerate(taps):
      print '%f,' % (t.x),
    print '};'
    print 'static const float s_downsample_ws[] = {',
    for i, t in enumerate(taps):
      print '%f,' % (t.w),
    print '};'

g = Filter()
g.gaussian(R)
#g.sincLancoz2Dec4()
#g.gaussianSqrt2Dec4()
#g.laplacianOfGaussian9()

g.printFilter()
taps = g.calcTaps(THRESHOLD)
Filter.printTaps(taps)
Filter.printTapsCode(taps)


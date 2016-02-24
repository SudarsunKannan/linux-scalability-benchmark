#!/usr/bin/python

import os, re
from subprocess import *

corelist=[1]
pagelist = [16000, 32000, 64000, 128000, 256000, 512000]
repeat = 2

def warmup():
	print('Warming up...')
	for i in xrange(3):
		p = Popen('./mmapbench 1', shell=True, stdout=PIPE)
		os.waitpid(p.pid, 0)

def test():
	print('Begin testing...')
	pattern = re.compile(r'usec: (\d+)')
	for n in pagelist:
		total = 0
		for i in xrange(repeat):
			p = Popen('./mmapbench %d' % n, shell=True, stdout=PIPE)
			os.waitpid(p.pid, 0)
			output = p.stdout.read().strip()
			usec = int(pattern.search(output).group(1))
			total += usec
		print('Pages #%d: %d' % (n, total / repeat))

#We do not want to warm up. Applications do not warm up, athletes do
#warmup()
test()

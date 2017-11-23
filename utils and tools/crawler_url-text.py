# -*- coding: utf-8 -*-
import urllib2
from bs4 import BeautifulSoup
import re
import os
import errno
import time
import subprocess

def gen_urls():
	url_prev = "http://so.gushiwen.org/view_"
	url_post = ".aspx"
	for i in xrange(72352, 72370):
		url = url_prev+str(i)+url_post
		yield (url, str(i))

def get_folder():
	return "古文观止卷十二"

def is_start_delimit(line):
	if "句子吧" in line: return True
	else: return False

def is_end_delimit(line):
	if ("译文" in line or 
       "document" in line or
	   "下一章" in line): return True
	else: return False

def url_get_text(url):
	try:
		page = urllib2.urlopen(url).read()
	except:
		print url, " open fails"
		return None
	soup = BeautifulSoup(page)
	text = soup.get_text().encode('utf-8')
	line = text.splitlines()
	return line

def start_delimit(lines, s=0):
	nline = len(lines)
	for i in xrange(s, nline):
		if is_start_delimit(lines[i]):
			return i+1
	return 0

def end_delimit(lines, s=0):
	nline = len(lines)
	for i in xrange(s, nline):
		if is_end_delimit(lines[i]):
			return i-1
	return nline-1

def flush_line(line, f=None):
	if len(line) <3: return
	lines = re.split(r'(。|！|？)', line)
	nl = len(lines)
	for i in xrange(0, nl, 2):
		if i+1 >= nl: l = lines[i]
		else: l = lines[i] + lines[i+1]
		if len(l)>3: f.write(l+'\n')

def process_url(url, folder, fname):
	lines = url_get_text(url)
	if lines == None: return
	s = start_delimit(lines)
	e = end_delimit(lines, s)
	path = './'+folder+'/'+fname
	if not os.path.exists(os.path.dirname(path)):
		os.makedirs(os.path.dirname(path))
	f = open(path, 'w')
	print s, e
	for i in xrange(s, e):
		flush_line(lines[i], f)
	f.close

def run_crawler():
	folder = get_folder()
	urls = gen_urls()
	for url, i in urls:
		process_url(url, folder, i)
		time.sleep(20)

zis = ["瑭",	 "璋", "珅", "珣", "琛", "琮", "琏", "琨", "琬", "瓒", "琰", "珽",
	   "琎", "瑨", "琭", "珑", "瑱", "瑄", "珪", "璜", "琥", "瑾", "瑜", "玕", 
	   "玙", "琯", "珏", "珺", "玮", "琤", "玠", "玱", "玞", "璟", "珥", "珙", "璠"]

def search_zi():
	for i in xrange(0, len(zis)):
		cmd = "grep -r \"" + zis[i] + "\" . > ../re/" + zis[i]
#		print cmd
		subprocess.check_output(cmd,shell=True)
		time.sleep(1)

#run_crawler()
search_zi()



#!/usr/bin/python3

import sys
import streamlink

def main():
	try:
		url = sys.argv[1]
	except IndexError:
		sys.exit(1)
	try:
		streams = str(list(streamlink.streams(url).keys()))
	except streamlink.exceptions.NoPluginError:
		sys.exit(1)
	streams2 = []
	if len(streams) <= 2:
		print("Stream offline")
		sys.exit(1)

	print("Stream is live")
	sys.exit(0)

main()

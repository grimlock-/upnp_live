#!/usr/bin/python3

import sys
import streamlink
from streamlink import Streamlink

def main():
	try:
		url = sys.argv[1]
	except IndexError:
		sys.exit(1)
	try:
		sl = Streamlink()
		#args = {"disable-hosting": ""}
		sl.set_plugin_option("twitch", "disable-hosting", "true")
		sl.set_plugin_option("twitch", "disable-reruns", "true")
		streams = str(list(sl.streams(url).keys()))
	except streamlink.exceptions.NoPluginError:
		sys.exit(1)
	streams2 = []
	if len(streams) <= 2:
		print("Stream offline")
		sys.exit(1)

	print("Stream is live")
	sys.exit(0)

main()

#!/usr/bin/python3
# This script can only include commas in it's output when it successfully
# obtains the available quality levels for a stream

import sys
import streamlink

def main():
	try:
		url = sys.argv[1]
	except IndexError:
		print("Error: No URL provided")
		return
	try:
		streams = str(list(streamlink.streams(url).keys()))
	except streamlink.exceptions.NoPluginError:
		print("Error: No plugin for the given URL")
		return
	streams2 = []
	if len(streams) <= 2:
		print("Error: Stream offline")
		return
	i = streams.find("\'", 0)
	j = streams.find("\'", i+1)
	while (i != -1 and j != -1):
		sub = streams[i+1:j]
		if sub != "audio_only" and sub != "best" and sub != "worst":
			streams2.append(streams[i+1:j])
		i = streams.find("\'", j+1)
		j = streams.find("\'", i+1)
	streams2 = str(streams2)
	streams2 = streams2.replace(" ", "")
	streams2 = streams2.replace("[", "")
	streams2 = streams2.replace("]", "")
	streams2 = streams2.replace("\'", "")
	sys.stdout.write(streams2)
	sys.stdout.flush()

main()

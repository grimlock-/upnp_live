#pragma once
#include <mupnp/mUPnP.h>

class ContentDirectoryTree : public Tree
{
	public:
		ContentDirectoryTree()
		ContentDirectoryTree(const char*)
		~ContentDirectoryTree()
		addStreamURL(const char*);
		addStreamItem(<stream objectID>);
}

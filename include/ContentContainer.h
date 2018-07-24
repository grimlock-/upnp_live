#pragma once
#include "ContentObject.h"

namespace upnp_live {
class ContentContainer : public ContentObject
{
	public:
		ContentContainer();
		~ContentContainer();
		unsigned int getChildCount();
	private:
		unsigned int childCount;
};
}

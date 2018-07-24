#pragma once
#include "ContentObject.h"

namespace upnp_live {
class ContentItem : public ContentObject
{
	public:
		ContentItem();
		~ContentItem();
		int getURIID();
		std::string getResourceURI();
	private:
		int uriId;
		std::string resourceUri;
};
}

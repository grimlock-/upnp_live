#pragma once
#include "ContentObject.h"

namespace upnp_live {
	
class ContentItem : public ContentObject
{
	public:
		int GetUriId();
		std::string GetResourceURI();
	private:
		int uriId;
		std::string resourceUri;
};

}

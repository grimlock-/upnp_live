#pragma once
#include <string>

namespace upnp_live {
class ContentObject
{
	public:
		virtual ~ContentObject();
		
		std::string Id;
		std::string ParentId;
		std::string Title;
		std::string UpnpClass;
		bool Restricted = true;
};
}

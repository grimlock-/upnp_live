#pragma once
#include <string>

namespace upnp_live {
class ContentObject
{
	public:
		ContentObject();
		virtual ~ContentObject();
		//Set functions
		void setID(int);
		void setParentID(int);
		void setTitle(std::string);
		void setUPNPClass(std::string);
		//Get functions
		int getObjectID();
		int getParentID();
		bool isRestricted();
		std::string getTitle();
		std::string getUPNPClass();
	private:
		int objectId, parentId;
		//bool restricted;
		std::string title, upnpClass;
};
}

#pragma once
#include "ContentObject.h"

namespace upnp_live {
class ContentContainer : public ContentObject
{
	public:
		virtual unsigned int GetChildCount() = delete;
};
}

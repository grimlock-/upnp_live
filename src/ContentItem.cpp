#include "ContentItem.h"
using namespace upnp_live;

ContentItem::ContentItem()
{
}

ContentItem::~ContentItem()
{
}

int ContentItem::getURIID() {return uriId;}
std::string ContentItem::getResourceURI() {return resourceUri;}

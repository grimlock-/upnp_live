#include "ContentContainer.h"
using namespace upnp_live;

ContentContainer::ContentContainer() : ContentObject(), childCount(0) {}

unsigned int ContentContainer::GetChildCount() {return childCount;}

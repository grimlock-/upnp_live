#include "ContentContainer.h"
using namespace upnp_live;

ContentContainer::ContentContainer() : ContentObject(), childCount(0) {}
ContentContainer::~ContentContainer() {}
unsigned int ContentContainer::getChildCount() {return childCount;}

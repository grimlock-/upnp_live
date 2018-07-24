#include "ContentObject.h"
using namespace upnp_live;

ContentObject::ContentObject() {}
ContentObject::~ContentObject() {}

//Set functions
void ContentObject::setID(int id) {objectId = id;}
void ContentObject::setParentID(int id) {parentId = id;}
void ContentObject::setTitle(std::string newTitle) {title = newTitle;}
void ContentObject::setUPNPClass(std::string newClass) {upnpClass = newClass;}

//Get functions
int ContentObject::getObjectID() {return objectId;}
int ContentObject::getParentID() {return parentId;}
bool ContentObject::isRestricted() {return true;}
std::string ContentObject::getTitle() {return title;}
std::string ContentObject::getUPNPClass() {return upnpClass;}

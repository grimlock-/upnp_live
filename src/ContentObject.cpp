#include "ContentObject.h"
using namespace upnp_live;

ContentObject::~ContentObject(){}

//Setters
void ContentObject::SetID(std::string id) {objectId = id;}
void ContentObject::SetParentID(std::string id) {parentId = id;}
void ContentObject::SetTitle(std::string newTitle) {title = newTitle;}
void ContentObject::SetUPNPClass(std::string newClass) {upnpClass = newClass;}

//Getters and Isses
bool ContentObject::IsRestricted() {return true;}
std::string ContentObject::GetObjectID() {return objectId;}
std::string ContentObject::GetParentID() {return parentId;}
std::string ContentObject::GetTitle() {return title;}
std::string ContentObject::GetUpnpClass() {return upnpClass;}

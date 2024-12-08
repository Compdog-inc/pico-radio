// Standard headers
#include <stdlib.h>
#include <string>
#include <cstdarg>
#include <cstring>
#include <algorithm>

#include "nt/nttopic.h"

using namespace std::literals;

NTTopic::NTTopic(NetworkTableInstance *nt) : nt(nt), name(), id(-1), type(NTDataType::Bool), properties(NetworkTableInstance::TopicProperties_DEFAULT)
{
}

NTTopic::NTTopic(NetworkTableInstance *nt, const NetworkTableInstance::AnnouncedTopic &announcedTopic) : nt(nt), name(announcedTopic.name), id(announcedTopic.id), type(announcedTopic.type), properties(announcedTopic.properties)
{
}

NTTopic::~NTTopic()
{
}

void NTTopic::setProperties(NetworkTableInstance::TopicProperties properties)
{
    this->properties = nt->setProperties(name, properties);
}
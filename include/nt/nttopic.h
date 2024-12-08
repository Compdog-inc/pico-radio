#ifndef _NT_TOPIC_H_
#define _NT_TOPIC_H_

#include <string>
#include "ntinstance.h"

class NTTopic
{
public:
    NTTopic(NetworkTableInstance *nt);
    NTTopic(NetworkTableInstance *nt, const NetworkTableInstance::AnnouncedTopic &announcedTopic);
    ~NTTopic();

    inline std::string getName() const { return name; }
    inline int64_t getId() const { return id; }
    inline NTDataType getType() const { return type; }
    inline NetworkTableInstance::TopicProperties getProperties() const { return properties; }
    inline bool isValid() const { return id != -1; }

    void setProperties(NetworkTableInstance::TopicProperties properties);

private:
    NetworkTableInstance *nt;

    std::string name;
    int64_t id;
    NTDataType type;
    NetworkTableInstance::TopicProperties properties;
};

#endif
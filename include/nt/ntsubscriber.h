#ifndef _NT_SUBSCRIBER_H_
#define _NT_SUBSCRIBER_H_

#include <string>
#include "ntinstance.h"
#include "nttopic.h"

class NTSubscriber
{
public:
    NTSubscriber();
    NTSubscriber(NetworkTableInstance *nt, std::string topic);
    ~NTSubscriber();

    NTTopic &getTopic() { return topic; }
    bool close();

    NTDataValue get();
    bool getBoolean(bool defaultValue);
    double getDouble(double defaultValue);
    float getFloat(float defaultValue);
    int64_t getInt(int64_t defaultValue);
    uint64_t getUInt(uint64_t defaultValue);
    std::string getString(std::string defaultValue);
    std::vector<bool> getBooleanArray(std::vector<bool> defaultValue);
    std::vector<double> getDoubleArray(std::vector<double> defaultValue);
    std::vector<float> getFloatArray(std::vector<float> defaultValue);
    std::vector<int64_t> getIntArray(std::vector<int64_t> defaultValue);
    std::vector<std::string> getStringArray(std::vector<std::string> defaultValue);
    std::vector<uint8_t> getRaw(std::vector<uint8_t> defaultValue);

    void _assignTopic(const NetworkTableInstance::AnnouncedTopic &topic);
    void _unassignTopic();

private:
    NetworkTableInstance *nt;
    NTTopic topic;
    int32_t subuid;
};

#endif
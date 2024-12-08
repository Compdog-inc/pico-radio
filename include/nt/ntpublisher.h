#ifndef _NT_PUBLISHER_H_
#define _NT_PUBLISHER_H_

#include <string>
#include "ntinstance.h"
#include "nttopic.h"

class NTPublisher
{
public:
    NTPublisher();
    NTPublisher(NetworkTableInstance *nt, std::string topic, NTDataValue defaultValue);
    NTPublisher(NetworkTableInstance *nt, std::string topic, NTDataValue defaultValue, uint64_t time);
    NTPublisher(NetworkTableInstance *nt, std::string topic, NTDataValue defaultValue, NetworkTableInstance::TopicProperties properties);
    NTPublisher(NetworkTableInstance *nt, std::string topic, NTDataValue defaultValue, uint64_t time, NetworkTableInstance::TopicProperties properties);
    ~NTPublisher();

    NTTopic &getTopic() { return topic; }
    bool close();

    bool set(NTDataValue value);
    bool set(NTDataValue value, uint64_t time);
    bool setBoolean(bool value);
    bool setBoolean(bool value, uint64_t time);
    bool setDouble(double value);
    bool setDouble(double value, uint64_t time);
    bool setFloat(float value);
    bool setFloat(float value, uint64_t time);
    bool setInt(int64_t value);
    bool setInt(int64_t value, uint64_t time);
    bool setUInt(uint64_t value);
    bool setUInt(uint64_t value, uint64_t time);
    bool setString(std::string value);
    bool setString(std::string value, uint64_t time);
    bool setBooleanArray(std::vector<bool> value);
    bool setBooleanArray(std::vector<bool> value, uint64_t time);
    bool setDoubleArray(std::vector<double> value);
    bool setDoubleArray(std::vector<double> value, uint64_t time);
    bool setFloatArray(std::vector<float> value);
    bool setFloatArray(std::vector<float> value, uint64_t time);
    bool setIntArray(std::vector<int64_t> value);
    bool setIntArray(std::vector<int64_t> value, uint64_t time);
    bool setStringArray(std::vector<std::string> value);
    bool setStringArray(std::vector<std::string> value, uint64_t time);
    bool setRaw(std::vector<uint8_t> value);
    bool setRaw(std::vector<uint8_t> value, uint64_t time);

private:
    NetworkTableInstance *nt;
    NTTopic topic;
    int32_t pubuid;
};

#endif
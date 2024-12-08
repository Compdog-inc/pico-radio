#ifndef _NT_ENTRY_H_
#define _NT_ENTRY_H_

#include <string>
#include "ntsubscriber.h"
#include "ntpublisher.h"

class NTEntry
{
public:
    NTEntry(NetworkTableInstance *nt, std::string topic);
    NTEntry(NetworkTableInstance *nt, std::string topic, NTDataValue defaultValue);
    NTEntry(NetworkTableInstance *nt, std::string topic, NTDataValue defaultValue, NetworkTableInstance::TopicProperties properties);
    NTEntry(NetworkTableInstance *nt, std::string topic, NetworkTableInstance::TopicProperties properties);
    ~NTEntry();

    NTTopic &getTopic() { return sub.getTopic(); }
    inline bool isPublishing() { return publishing; }
    bool unpublish();
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
    bool publishing;
    NetworkTableInstance::TopicProperties pubProperties = NetworkTableInstance::TopicProperties_DEFAULT;
    NTSubscriber sub;
    NTPublisher pub;
};

#endif
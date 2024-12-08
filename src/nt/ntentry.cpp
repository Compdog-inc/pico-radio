// Standard headers
#include <stdlib.h>
#include <string>
#include <cstdarg>
#include <cstring>
#include <algorithm>

#include "nt/ntentry.h"

using namespace std::literals;

NTEntry::NTEntry(NetworkTableInstance *nt, std::string topic) : nt(nt), publishing(false), sub(nt, topic), pub()
{
}

NTEntry::NTEntry(NetworkTableInstance *nt, std::string topic, NTDataValue defaultValue) : nt(nt), publishing(true), sub(nt, topic), pub(nt, topic, defaultValue)
{
}

NTEntry::NTEntry(NetworkTableInstance *nt, std::string topic, NTDataValue defaultValue, NetworkTableInstance::TopicProperties properties) : nt(nt), publishing(true), pubProperties(properties), sub(nt, topic), pub(nt, topic, defaultValue, properties)
{
}

NTEntry::NTEntry(NetworkTableInstance *nt, std::string topic, NetworkTableInstance::TopicProperties properties) : nt(nt), publishing(false), pubProperties(properties), sub(nt, topic), pub()
{
}

NTEntry::~NTEntry()
{
    close();
}

bool NTEntry::unpublish()
{
    if (publishing)
    {
        publishing = false;
        return pub.close();
    }

    return false;
}

bool NTEntry::close()
{
    unpublish();
    return sub.close();
}

NTDataValue NTEntry::get()
{
    return sub.get();
}

bool NTEntry::getBoolean(bool defaultValue)
{
    return sub.getBoolean(defaultValue);
}

double NTEntry::getDouble(double defaultValue)
{
    return sub.getDouble(defaultValue);
}

float NTEntry::getFloat(float defaultValue)
{
    return sub.getFloat(defaultValue);
}

int64_t NTEntry::getInt(int64_t defaultValue)
{
    return sub.getInt(defaultValue);
}

uint64_t NTEntry::getUInt(uint64_t defaultValue)
{
    return sub.getUInt(defaultValue);
}

std::string NTEntry::getString(std::string defaultValue)
{
    return sub.getString(defaultValue);
}

std::vector<bool> NTEntry::getBooleanArray(std::vector<bool> defaultValue)
{
    return sub.getBooleanArray(defaultValue);
}

std::vector<double> NTEntry::getDoubleArray(std::vector<double> defaultValue)
{
    return sub.getDoubleArray(defaultValue);
}

std::vector<float> NTEntry::getFloatArray(std::vector<float> defaultValue)
{
    return sub.getFloatArray(defaultValue);
}

std::vector<int64_t> NTEntry::getIntArray(std::vector<int64_t> defaultValue)
{
    return sub.getIntArray(defaultValue);
}

std::vector<std::string> NTEntry::getStringArray(std::vector<std::string> defaultValue)
{
    return sub.getStringArray(defaultValue);
}

std::vector<uint8_t> NTEntry::getRaw(std::vector<uint8_t> defaultValue)
{
    return sub.getRaw(defaultValue);
}

bool NTEntry::set(NTDataValue value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), value, pubProperties);
        return true;
    }
    else
    {
        return pub.set(value);
    }
}

bool NTEntry::set(NTDataValue value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), value, time, pubProperties);
        return true;
    }
    else
    {
        return pub.set(value, time);
    }
}

bool NTEntry::setBoolean(bool value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setBoolean(value);
    }
}

bool NTEntry::setBoolean(bool value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setBoolean(value, time);
    }
}

bool NTEntry::setDouble(double value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setDouble(value);
    }
}

bool NTEntry::setDouble(double value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setDouble(value, time);
    }
}

bool NTEntry::setFloat(float value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setFloat(value);
    }
}

bool NTEntry::setFloat(float value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setFloat(value, time);
    }
}

bool NTEntry::setInt(int64_t value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setInt(value);
    }
}

bool NTEntry::setInt(int64_t value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setInt(value, time);
    }
}

bool NTEntry::setUInt(uint64_t value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setUInt(value);
    }
}

bool NTEntry::setUInt(uint64_t value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setUInt(value, time);
    }
}

bool NTEntry::setString(std::string value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setString(value);
    }
}

bool NTEntry::setString(std::string value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setString(value, time);
    }
}

bool NTEntry::setBooleanArray(std::vector<bool> value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setBooleanArray(value);
    }
}

bool NTEntry::setBooleanArray(std::vector<bool> value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setBooleanArray(value, time);
    }
}

bool NTEntry::setDoubleArray(std::vector<double> value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setDoubleArray(value);
    }
}

bool NTEntry::setDoubleArray(std::vector<double> value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setDoubleArray(value, time);
    }
}

bool NTEntry::setFloatArray(std::vector<float> value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setFloatArray(value);
    }
}

bool NTEntry::setFloatArray(std::vector<float> value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setFloatArray(value, time);
    }
}

bool NTEntry::setIntArray(std::vector<int64_t> value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setIntArray(value);
    }
}

bool NTEntry::setIntArray(std::vector<int64_t> value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setIntArray(value, time);
    }
}

bool NTEntry::setStringArray(std::vector<std::string> value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setStringArray(value);
    }
}

bool NTEntry::setStringArray(std::vector<std::string> value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setStringArray(value, time);
    }
}

bool NTEntry::setRaw(std::vector<uint8_t> value)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, pubProperties);
        return true;
    }
    else
    {
        return pub.setRaw(value);
    }
}

bool NTEntry::setRaw(std::vector<uint8_t> value, uint64_t time)
{
    if (!publishing)
    {
        publishing = true;
        new (&pub) NTPublisher(nt, getTopic().getName(), {value}, time, pubProperties);
        return true;
    }
    else
    {
        return pub.setRaw(value, time);
    }
}
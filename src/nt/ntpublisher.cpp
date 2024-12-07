// Standard headers
#include <stdlib.h>
#include <string>
#include <cstdarg>
#include <cstring>
#include <algorithm>

#include "nt/ntpublisher.h"

using namespace std::literals;

static int32_t NEXT_PUBLISHER_ID = 0;

NTPublisher::NTPublisher(NetworkTableInstance *nt, std::string topic, NTDataValue defaultValue) : nt(nt),
                                                                                                  topic(
                                                                                                      nt,
                                                                                                      nt->publish(
                                                                                                          topic,
                                                                                                          NEXT_PUBLISHER_ID++,
                                                                                                          defaultValue.type,
                                                                                                          NetworkTableInstance::TopicProperties_DEFAULT))
{
    set(defaultValue);
}

NTPublisher::NTPublisher(NetworkTableInstance *nt, std::string topic, NTDataValue defaultValue, NetworkTableInstance::TopicProperties properties) : nt(nt),
                                                                                                                                                    topic(
                                                                                                                                                        nt,
                                                                                                                                                        nt->publish(
                                                                                                                                                            topic,
                                                                                                                                                            NEXT_PUBLISHER_ID++,
                                                                                                                                                            defaultValue.type,
                                                                                                                                                            properties))
{
    set(defaultValue);
}

NTPublisher::~NTPublisher()
{
    close();
}

bool NTPublisher::close()
{
    nt->unpublish(topic.getId());
    return true;
}

bool NTPublisher::set(NTDataValue value)
{
    if (topic.getType() != value.getAPIType())
        return false;

    nt->updateTopic(topic.getId(), value);
    return true;
}

bool NTPublisher::set(NTDataValue value, uint64_t time)
{
    if (topic.getType() != value.getAPIType())
        return false;

    nt->updateTopic(topic.getId(), value, time);
    return true;
}

bool NTPublisher::setBoolean(bool value)
{
    if (topic.getType() != NTDataType::Bool)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setBoolean(bool value, uint64_t time)
{
    if (topic.getType() != NTDataType::Bool)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setDouble(double value)
{
    if (topic.getType() != NTDataType::Float64)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setDouble(double value, uint64_t time)
{
    if (topic.getType() != NTDataType::Float64)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setFloat(float value)
{
    if (topic.getType() != NTDataType::Float32)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setFloat(float value, uint64_t time)
{
    if (topic.getType() != NTDataType::Float32)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setInt(int64_t value)
{
    if (topic.getType() != NTDataType::Int)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setInt(int64_t value, uint64_t time)
{
    if (topic.getType() != NTDataType::Int)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setUInt(uint64_t value)
{
    if (topic.getType() != NTDataType::Int && topic.getType() != NTDataType::UInt)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setUInt(uint64_t value, uint64_t time)
{
    if (topic.getType() != NTDataType::Int && topic.getType() != NTDataType::UInt)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setString(std::string value)
{
    if (topic.getType() != NTDataType::Str && topic.getType() != NTDataType::Json)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setString(std::string value, uint64_t time)
{
    if (topic.getType() != NTDataType::Str && topic.getType() != NTDataType::Json)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setBooleanArray(std::vector<bool> value)
{
    if (topic.getType() != NTDataType::BoolArray)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setBooleanArray(std::vector<bool> value, uint64_t time)
{
    if (topic.getType() != NTDataType::BoolArray)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setDoubleArray(std::vector<double> value)
{
    if (topic.getType() != NTDataType::Float64Array)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setDoubleArray(std::vector<double> value, uint64_t time)
{
    if (topic.getType() != NTDataType::Float64Array)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setFloatArray(std::vector<float> value)
{
    if (topic.getType() != NTDataType::Float32Array)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setFloatArray(std::vector<float> value, uint64_t time)
{
    if (topic.getType() != NTDataType::Float32Array)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setIntArray(std::vector<int64_t> value)
{
    if (topic.getType() != NTDataType::IntArray)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setIntArray(std::vector<int64_t> value, uint64_t time)
{
    if (topic.getType() != NTDataType::IntArray)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setStringArray(std::vector<std::string> value)
{
    if (topic.getType() != NTDataType::StrArray)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setStringArray(std::vector<std::string> value, uint64_t time)
{
    if (topic.getType() != NTDataType::StrArray)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}

bool NTPublisher::setRaw(std::vector<uint8_t> value)
{
    if (topic.getType() != NTDataType::Bin)
        return false;

    nt->updateTopic(topic.getId(), {value});
    return true;
}

bool NTPublisher::setRaw(std::vector<uint8_t> value, uint64_t time)
{
    if (topic.getType() != NTDataType::Bin)
        return false;

    nt->updateTopic(topic.getId(), {value}, time);
    return true;
}
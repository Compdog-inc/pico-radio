// Standard headers
#include <stdlib.h>
#include <string>
#include <cstdarg>
#include <cstring>
#include <queue>
#include <unordered_map>
#include <algorithm>

#include "nt/ntsubscriber.h"

using namespace std::literals;

static int32_t NEXT_SUBSCRIBER_ID = 0;

struct TopicCallbackData
{
    struct AnnounceData
    {
        std::queue<NTSubscriber *> announce = {};
        std::queue<NTSubscriber *> unannounce = {};
    };

    std::unordered_map<std::string, AnnounceData> topics = {};

    struct UpdateData
    {
        NTDataValue value = NTDataValue(NTDataType::Unassigned);
        uint64_t timestamp;
    };

    std::unordered_map<int64_t, UpdateData> updates = {};
};

static std::unordered_map<NetworkTableInstance *, TopicCallbackData> NT_topicCallbacks = {};

static bool NT_topicAnnouncedCallback(
    NetworkTableInstance *nt,
    const NetworkTableInstance::AnnouncedTopic &topic,
    void *args)
{
    auto &queue = NT_topicCallbacks[nt].topics[topic.name].announce;
    while (!queue.empty())
    {
        queue.front()->_assignTopic(topic);
        queue.pop();
    }

    return true;
}

static bool NT_topicUnAnnouncedCallback(
    NetworkTableInstance *nt,
    const std::string &name,
    int64_t id,
    void *args)
{
    auto &queue = NT_topicCallbacks[nt].topics[name].unannounce;
    while (!queue.empty())
    {
        queue.front()->_unassignTopic();
        queue.pop();
    }

    return true;
}

static bool NT_topicUpdateCallback(
    NetworkTableInstance *nt,
    int64_t id,
    uint64_t timestamp,
    const NTDataValue &value,
    void *args)
{
    NT_topicCallbacks[nt].updates[id] = {value, timestamp};
    return true;
}

NTSubscriber::NTSubscriber() : nt(nullptr), topic(nullptr)
{
}

NTSubscriber::NTSubscriber(NetworkTableInstance *nt, std::string topic) : nt(nt), topic(nt)
{
    assert(nt != nullptr);
    if (!NT_topicCallbacks.contains(nt))
    {
        NT_topicCallbacks[nt] = {
            {{topic, {}}}};

        nt->topicAnnouncedCallback = NT_topicAnnouncedCallback;
        nt->topicUnAnnouncedCallback = NT_topicUnAnnouncedCallback;
        nt->topicUpdateCallback = NT_topicUpdateCallback;
    }
    else if (!NT_topicCallbacks[nt].topics.contains(topic))
    {
        NT_topicCallbacks[nt].topics[topic] = {};
    }

    NT_topicCallbacks[nt].topics[topic].announce.push(this);
    NT_topicCallbacks[nt].topics[topic].unannounce.push(this);

    nt->subscribe(
        {topic},
        subuid = NEXT_SUBSCRIBER_ID++,
        NetworkTableInstance::SubscriptionOptions_DEFAULT);
}

NTSubscriber::~NTSubscriber()
{
    close();
}

void NTSubscriber::_assignTopic(const NetworkTableInstance::AnnouncedTopic &topic)
{
    this->topic = NTTopic(nt, topic);
}

void NTSubscriber::_unassignTopic()
{
    this->topic = NTTopic(nt);
}

bool NTSubscriber::close()
{
    assert(nt != nullptr);
    nt->unsubscribe(subuid);
    return true;
}

NTDataValue NTSubscriber::get()
{
    assert(nt != nullptr);
    auto &updates = NT_topicCallbacks[nt].updates;
    if (!updates.contains(topic.getId()))
    {
        return NTDataValue(NTDataType::Unassigned);
    }

    return updates[topic.getId()].value;
}

bool NTSubscriber::getBoolean(bool defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.getAPIType() != NTDataType::Bool)
        return defaultValue;
    else
        return value.b;
}

double NTSubscriber::getDouble(double defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.getAPIType() != NTDataType::Float64)
        return defaultValue;
    else
        return value.f64;
}

float NTSubscriber::getFloat(float defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.getAPIType() != NTDataType::Float32)
        return defaultValue;
    else
        return value.f32;
}

int64_t NTSubscriber::getInt(int64_t defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.type != NTDataType::Int)
        return defaultValue;
    else
        return value.i;
}

uint64_t NTSubscriber::getUInt(uint64_t defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.type != NTDataType::UInt)
        return defaultValue;
    else
        return value.ui;
}

std::string NTSubscriber::getString(std::string defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.getAPIType() != NTDataType::Str)
        return defaultValue;
    else
        return value.str;
}

std::vector<bool> NTSubscriber::getBooleanArray(std::vector<bool> defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.getAPIType() != NTDataType::BoolArray)
        return defaultValue;
    else
        return value.bArray;
}

std::vector<double> NTSubscriber::getDoubleArray(std::vector<double> defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.getAPIType() != NTDataType::Float64Array)
        return defaultValue;
    else
        return value.f64Array;
}

std::vector<float> NTSubscriber::getFloatArray(std::vector<float> defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.getAPIType() != NTDataType::Float32Array)
        return defaultValue;
    else
        return value.f32Array;
}

std::vector<int64_t> NTSubscriber::getIntArray(std::vector<int64_t> defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.getAPIType() != NTDataType::IntArray)
        return defaultValue;
    else
        return value.iArray;
}

std::vector<std::string> NTSubscriber::getStringArray(std::vector<std::string> defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.getAPIType() != NTDataType::StrArray)
        return defaultValue;
    else
        return value.strArray;
}

std::vector<uint8_t> NTSubscriber::getRaw(std::vector<uint8_t> defaultValue)
{
    assert(nt != nullptr);
    auto value = get();
    if (!value.isValid() || value.getAPIType() != NTDataType::Bin)
        return defaultValue;
    else
        return value.bin;
}
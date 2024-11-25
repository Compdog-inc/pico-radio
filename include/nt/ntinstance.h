#ifndef _NT_INSTANCE_H_
#define _NT_INSTANCE_H_

#include <string>
#include <unordered_map>

#include "../msgpack/msgpack.hpp"

#include "../wsserver.h"
#include "../websocket.h"

enum class NTDataType : uint8_t
{
    Bool = 0,
    Float64 = 1,
    Int = 2,
    Float32 = 3,
    Str = 4,
    Bin = 5,
    UInt = 6,      // implementation based (not part of api)
    Json = 7,      // implementation based (not part of api)
    Raw = 8,       // implementation based (not part of api)
    Msgpack = 9,   // implementation based (not part of api)
    Protobuf = 10, // implementation based (not part of api)
    BoolArray = 16,
    Float64Array = 17,
    IntArray = 18,
    Float32Array = 19,
    StrArray = 20
};

struct NTDataValue
{
    NTDataType type;
    union
    {
        bool b;
        double f64;
        int64_t i;
        float f32;
        std::string str;
        std::vector<uint8_t> bin;
        uint64_t ui;
        std::list<bool> bArray;
        std::list<double> f64Array;
        std::list<int64_t> iArray;
        std::list<float> f32Array;
        std::list<std::string> strArray;
    };

    void unpack(msgpack::Unpacker &unpacker);
    void pack(msgpack::Packer &packer);

    NTDataValue(NTDataType type, msgpack::Unpacker &unpacker);
    NTDataValue(bool b);
    NTDataValue(double f64);
    NTDataValue(int64_t i);
    NTDataValue(float f32);
    NTDataValue(std::string str);
    NTDataValue(std::vector<uint8_t> bin);
    NTDataValue(uint64_t ui);
    NTDataValue(std::list<bool> bArray);
    NTDataValue(std::list<double> f64Array);
    NTDataValue(std::list<int64_t> iArray);
    NTDataValue(std::list<float> f32Array);
    NTDataValue(std::list<std::string> strArray);
    NTDataValue(const NTDataValue &other);
    ~NTDataValue();
};

class NetworkTableInstance
{
public:
    enum class NetworkMode
    {
        Starting,
        Client,
        Server
    };

    NetworkTableInstance();
    ~NetworkTableInstance();

    void startClient(std::string_view url);
    void startServer();

    void stop();
    void close();

    bool isConnected();
    NetworkMode getNetworkMode();

    int64_t getServerTimeOffset();
    uint64_t getServerTime();

    void _handleFrame(const Guid &guid, const WebSocketFrame &frame);

    bool sendRTT();

private:
    WsServer *server;
    WebSocket *client;
    NetworkMode networkMode;

    struct SubscriptionOptions
    {
        int32_t periodic;
        bool all;
        bool topicsonly;
        bool prefix;
    };

    struct TopicProperties
    {
        bool persistent;
        bool retained;
        bool cached;
    };

    static constexpr SubscriptionOptions SubscriptionOptions_DEFAULT = {
        100, false, false, false};

    static constexpr TopicProperties TopicProperties_DEFAULT = {
        false, false, true};

    struct Subscription
    {
        int32_t uid;
        std::vector<std::string> topics;
        SubscriptionOptions options = SubscriptionOptions_DEFAULT;
    };

    struct Publisher
    {
        int32_t uid;
        std::string topic;
    };

    struct Topic
    {
        std::string name;
        NTDataValue value;
        uint32_t publisherCount;
        TopicProperties properties = TopicProperties_DEFAULT;

        Topic(std::string name, NTDataValue value) : name(name), value(value), publisherCount(0)
        {
        }

        ~Topic() {}
    };

    struct ClientData
    {
        std::string name;
        std::unordered_map<int32_t, Subscription *> subscriptions;
        std::unordered_map<int32_t, Publisher *> publishers;
        std::unordered_map<std::string, int64_t> topicIds;
    };

    std::unordered_map<std::string, Topic *> topics;
    std::unordered_map<Guid, ClientData *> clients;
    ClientData thisClient;

    int64_t serverTimeOffset;

    bool isSubscribed(const std::unordered_map<int32_t, Subscription *> &subscriptions, std::string name);

    bool announceTopic(const Guid &guid, const Topic *topic);
    bool announceTopic(const Topic *topic);
    bool announceTopics(const Guid &guid);
};

#endif
#ifndef _NT_INSTANCE_H_
#define _NT_INSTANCE_H_

#include <string>
#include <unordered_map>

#include "../msgpack/msgpack.hpp"

#include "../wsserver.h"
#include "../websocket.h"

static constexpr std::size_t MAX_CLIENT_TEXT_CACHE_LENGTH = 512;
static constexpr std::size_t MAX_CLIENT_BINARY_CACHE_LENGTH = 512;

enum class NTDataType : uint8_t
{
    Bool = 0,
    Float64 = 1,
    Int = 2,
    Float32 = 3,
    Str = 4,
    Bin = 5,
    UInt = 6,      // [Int] implementation based (not part of api)
    Json = 7,      // [Str] implementation based (not part of api)
    Raw = 8,       // [Bin] implementation based (not part of api)
    Msgpack = 9,   // [Bin] implementation based (not part of api)
    Protobuf = 10, // [Bin] implementation based (not part of api)
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
        uint64_t ui;
    };
    std::list<bool> bArray;
    std::list<double> f64Array;
    std::list<int64_t> iArray;
    std::list<float> f32Array;
    std::list<std::string> strArray;
    std::vector<uint8_t> bin;

    NTDataType getAPIType() const;

    void unpack(msgpack::Unpacker<false> &unpacker);
    void pack(msgpack::Packer<false> &packer) const;

    void assign(const NTDataValue &other);

    NTDataValue(NTDataType type, msgpack::Unpacker<false> &unpacker);
    /// @brief Creates a NTDataValue of a type and default/empty value
    NTDataValue(NTDataType type);
    NTDataValue(bool b);
    NTDataValue(double f64);
    NTDataValue(int64_t i);
    NTDataValue(float f32);
    NTDataValue(std::string str);
    NTDataValue(NTDataType type, std::string str);
    NTDataValue(std::vector<uint8_t> bin);
    NTDataValue(NTDataType type, std::vector<uint8_t> bin);
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

    struct TopicProperties
    {
        bool persistent;
        bool retained;
        bool cached;
    };

    struct SubscriptionOptions
    {
        int32_t periodic;
        bool all;
        bool topicsonly;
        bool prefix;

        template <class T>
        void pack(T &pack)
        {
            pack(nvp(periodic), nvp(all), nvp(topicsonly), nvp(prefix));
        }
    };

    static constexpr SubscriptionOptions SubscriptionOptions_DEFAULT = {
        100, false, false, false};

    static constexpr TopicProperties TopicProperties_DEFAULT = {
        false, false, true};

    struct AnnouncedTopic
    {
        std::string name;
        int64_t id;
        NTDataType type;
        TopicProperties properties;

        AnnouncedTopic(std::string name, int64_t id, NTDataType type, TopicProperties properties) : name(name), id(id), type(type), properties(properties)
        {
        }
    };

private:
    WsServer *server;
    WebSocket *client;
    NetworkMode networkMode;

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

    struct Subscription
    {
        int32_t uid;
        std::vector<std::string> topics;
        SubscriptionOptions options;

        Subscription(int32_t uid, std::vector<std::string> topics, SubscriptionOptions options) : uid(uid), topics(topics), options(options)
        {
        }

        template <class T>
        void pack(T &pack)
        {
            pack(nvp(uid), nvp(topics), nvp(options));
        }
    };

    struct TopicSubscription
    {
        std::string client;
        int32_t subuid;
        SubscriptionOptions options;

        TopicSubscription(std::string client, int32_t subuid, SubscriptionOptions options) : client(client), subuid(subuid), options(options)
        {
        }

        template <class T>
        void pack(T &pack)
        {
            pack(nvp(client), nvp(subuid), nvp(options));
        }
    };

    struct TopicPublisher
    {
        std::string client;
        int32_t pubuid;

        TopicPublisher(std::string client, int32_t pubuid) : client(client), pubuid(pubuid)
        {
        }

        template <class T>
        void pack(T &pack)
        {
            pack(nvp(client), nvp(pubuid));
        }
    };

    struct Publisher
    {
        int32_t uid;
        std::string topic;

        Publisher(int32_t uid, std::string topic) : uid(uid), topic(topic)
        {
        }

        template <class T>
        void pack(T &pack)
        {
            pack(nvp(uid), nvp(topic));
        }
    };

    struct ClientTopicData
    {
        int64_t id;
        bool initialPublish;
    };

    struct ClientData
    {
        Guid guid;
        std::string name;

        std::unordered_map<int32_t, Subscription *> subscriptions;
        std::unordered_map<int32_t, Publisher *> publishers;
        std::unordered_map<std::string, ClientTopicData> topicData;

        int64_t nextTopicIdAssigned = 0;

        std::string textCache = {};
        std::vector<uint8_t> binaryCache = {};
    };

    std::unordered_map<std::string, Topic *> topics = {};
    std::unordered_map<Guid, ClientData *> clients = {};
    ClientData thisClient;
    int64_t serverTimeOffset;

    inline bool isSelf(ClientData *client)
    {
        return client == &thisClient;
    }

    /* =========================== SERVER FUNCTIONS =========================== */
    inline Topic *getOrCreateTopic(std::string name, NTDataValue value, TopicProperties properties = TopicProperties_DEFAULT)
    {
        if (topics.contains(name))
        {
            Topic *topic = topics[name];
            return topic;
        }
        else
        {
            Topic *topic = new Topic(name, value);
            topic->properties = properties;
            topics[name] = topic;
            return topic;
        }
    }

    bool isSubscribed(Subscription *subscription, std::string name);
    bool isSubscribed(const std::unordered_map<int32_t, Subscription *> &subscriptions, std::string name, bool requireNotTopicsOnly = false, Subscription **out_subscription = nullptr);

    AnnouncedTopic announceTopicSelfSync(const Topic *topic, bool *out_success);
    bool announceTopicSelf(const Topic *topic);
    bool announceTopic(const Guid &guid, const Topic *topic);
    bool announceTopic(const Guid &guid, const Topic *topic, int32_t pubuid);
    bool announceTopic(const Topic *topic);
    bool announceTopic(const Topic *topic, const Guid &publisherGuid, int32_t pubuid);
    bool announceCachedTopicsSelf();
    bool announceCachedTopics(const Guid &guid);

    bool unannounceTopicSelf(const Topic *topic);
    bool unannounceTopic(const Guid &guid, const Topic *topic);
    bool unannounceTopic(const Topic *topic);

    bool sendTopicUpdateSelf(const Topic *topic);
    bool sendTopicUpdate(const Topic *topic);
    bool sendTopicUpdate(const Guid &guid, const Topic *topic);
    void publishInitialValuesSelf();
    void publishInitialValues(const Guid &guid);

    bool publishTopic(std::string name, NTDataValue value, TopicProperties properties = TopicProperties_DEFAULT);
    AnnouncedTopic publishTopicSelfSync(std::string name, NTDataValue value, TopicProperties properties, bool *out_success);
    bool publishTopic(std::string name, NTDataValue value, const Guid &publisherGuid, int32_t pubuid, TopicProperties properties = TopicProperties_DEFAULT);

    bool sendPropertyUpdateSelf(const Topic *topic);
    bool sendPropertyUpdate(const Guid &guid, const Topic *topic, bool ack);

    bool updateTopicProperties(const Topic *topic);
    bool updateTopicProperties(const Topic *topic, const Guid &updaterGuid);

    size_t nextClientWithName(std::string_view name);

    void flushText(ClientData *client)
    {
        flushText(client, MAX_CLIENT_TEXT_CACHE_LENGTH);
    }

    void flushText(ClientData *client, std::size_t uncachedSize);

    void flushText()
    {
        for (auto client : clients)
        {
            if (!isSelf(client.second))
                flushText(client.second);
        }
    }

    void flushBinary(ClientData *client)
    {
        flushBinary(client, MAX_CLIENT_BINARY_CACHE_LENGTH);
    }

    void flushBinary(ClientData *client, std::size_t uncachedSize);

    void flushBinary()
    {
        for (auto client : clients)
        {
            if (!isSelf(client.second))
                flushBinary(client.second);
        }
    }

    void updateClientsMetaTopic();
    void updateServerSubMetaTopic();
    void updateServerPubMetaTopic();
    void updateClientSubMetaTopic(const Guid &guid);
    void updateClientPubMetaTopic(const Guid &guid);
    void updateTopicSubMetaTopic(std::string name);
    void updateTopicPubMetaTopic(std::string name);

    /* =========================== CLIENT FUNCTIONS =========================== */

public:
    /* =========================== HYBRID FUNCTIONS =========================== */
    void subscribe(std::vector<std::string> topics, int32_t subuid, SubscriptionOptions options);
    void unsubscribe(int32_t subuid);
    AnnouncedTopic publish(std::string name, int32_t pubuid, NTDataType type, TopicProperties properties);
    void unpublish(int32_t pubuid);
    TopicProperties setProperties(std::string name, TopicProperties update);
    void updateTopic(int32_t id, NTDataValue value);
    void flush();

    /// @brief Custom args for the NetworkTable callbacks, set by the user
    void *callbackArgs = nullptr;

    /// @brief Callback for topic updates, contains the NetworkTable instance, id of the topic, timestamp, and value
    typedef bool (*NTTopicUpdateCallback)(NetworkTableInstance *nt, int64_t id, uint64_t timestamp, const NTDataValue &value, void *args);
    /// @brief Called whenever a topic is updated
    NTTopicUpdateCallback topicUpdateCallback = nullptr;
    /// @brief Callback for topic announcements, contains the NetworkTable instance, and the announced topic
    typedef bool (*NTTopicAnnouncedCallback)(NetworkTableInstance *nt, const AnnouncedTopic &topic, void *args);
    /// @brief Called whenever a topic is announced
    NTTopicAnnouncedCallback topicAnnouncedCallback = nullptr;
    /// @brief Callback for topic unannouncements, contains the NetworkTable instance, name of the topic, and the id of the topic
    typedef bool (*NTTopicUnAnnouncedCallback)(NetworkTableInstance *nt, const std::string &name, int64_t id, void *args);
    /// @brief Called whenever a topic is unannounced
    NTTopicUnAnnouncedCallback topicUnAnnouncedCallback = nullptr;
    /// @brief Callback for topic property updates, contains the NetworkTable instance, name of the topic, and the new properties
    typedef bool (*NTTopicPropertiesUpdateCallback)(NetworkTableInstance *nt, const std::string &name, TopicProperties properties, void *args);
    /// @brief Called whenever a topic property is updated
    NTTopicPropertiesUpdateCallback topicPropertiesUpdateCallback = nullptr;
};

#endif
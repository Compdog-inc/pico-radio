// Standard headers
#include <stdlib.h>
#include <string>
#include <cstdarg>
#include <cstring>
#include <algorithm>

// Hardware headers
#include <pico/time.h>

#include "msgpack/msgpack.hpp"
#include "nt/ntjson.hpp"

#include "nt/ntinstance.h"

using namespace std::literals;

enum class MessageMethod
{
    Unknown,
    Publish,
    UnPublish,
    SetProperties,
    Subscribe,
    UnSubscribe,
    Announce,
    UnAnnounce,
    Properties
};

MessageMethod parseMessageMethod(const std::string_view &str)
{
    if (str == "publish"sv)
    {
        return MessageMethod::Publish;
    }
    else if (str == "unpublish"sv)
    {
        return MessageMethod::UnPublish;
    }
    else if (str == "setproperties"sv)
    {
        return MessageMethod::SetProperties;
    }
    else if (str == "subscribe"sv)
    {
        return MessageMethod::Subscribe;
    }
    else if (str == "unsubscribe"sv)
    {
        return MessageMethod::UnSubscribe;
    }
    else if (str == "announce"sv)
    {
        return MessageMethod::Announce;
    }
    else if (str == "unannounce"sv)
    {
        return MessageMethod::UnAnnounce;
    }
    else if (str == "properties"sv)
    {
        return MessageMethod::Properties;
    }
    else
    {
        return MessageMethod::Unknown;
    }
}

std::string serializeMessageMethod(MessageMethod method)
{
    switch (method)
    {
    case MessageMethod::Publish:
        return "publish"s;
    case MessageMethod::UnPublish:
        return "unpublish"s;
    case MessageMethod::SetProperties:
        return "setproperties"s;
    case MessageMethod::Subscribe:
        return "subscribe"s;
    case MessageMethod::UnSubscribe:
        return "unsubscribe"s;
    case MessageMethod::Announce:
        return "announce"s;
    case MessageMethod::UnAnnounce:
        return "unannounce"s;
    case MessageMethod::Properties:
        return "properties"s;
    default:
        return ""s;
    }
}

std::string serializeDataType(NTDataType type)
{
    switch (type)
    {
    case NTDataType::Bool:
        return "boolean"s;
    case NTDataType::Float64:
        return "double"s;
    case NTDataType::Int:
    case NTDataType::UInt:
        return "int"s;
    case NTDataType::Float32:
        return "float"s;
    case NTDataType::Str:
        return "string"s;
    case NTDataType::Json:
        return "json"s;
    case NTDataType::Bin:
    case NTDataType::Raw:
        return "raw"s;
    case NTDataType::Msgpack:
        return "msgpack"s;
    case NTDataType::Protobuf:
        return "protobuf"s;
    case NTDataType::BoolArray:
        return "boolean[]"s;
    case NTDataType::Float64Array:
        return "double[]"s;
    case NTDataType::IntArray:
        return "int[]"s;
    case NTDataType::Float32Array:
        return "float[]"s;
    case NTDataType::StrArray:
        return "string[]"s;
    default:
        return ""s;
    }
}

NTDataType NTDataValue::getAPIType() const
{
    if (type == NTDataType::UInt)
        return NTDataType::Int;

    if (type == NTDataType::Json)
        return NTDataType::Str;

    if (type == NTDataType::Raw)
        return NTDataType::Bin;
    if (type == NTDataType::Msgpack)
        return NTDataType::Bin;
    if (type == NTDataType::Protobuf)
        return NTDataType::Bin;

    return type;
}

void NTDataValue::unpack(msgpack::Unpacker<false> &unpacker)
{
    switch (type)
    {
    case NTDataType::Bool:
        unpacker.process(b);
        break;
    case NTDataType::Float64:
        unpacker.process(f64);
        break;
    case NTDataType::Int:
        unpacker.process(i);
        break;
    case NTDataType::Float32:
        unpacker.process(f32);
        break;
    case NTDataType::Str:
    case NTDataType::Json:
        unpacker.process(str);
        break;
    case NTDataType::Bin:
    case NTDataType::Raw:
    case NTDataType::Msgpack:
    case NTDataType::Protobuf:
        unpacker.process(bin);
        break;
    case NTDataType::UInt:
        unpacker.process(ui);
        break;
    case NTDataType::BoolArray:
        unpacker.process(bArray);
        break;
    case NTDataType::Float64Array:
        unpacker.process(f64Array);
        break;
    case NTDataType::IntArray:
        unpacker.process(iArray);
        break;
    case NTDataType::Float32Array:
        unpacker.process(f32Array);
        break;
    case NTDataType::StrArray:
        unpacker.process(strArray);
        break;
    default:
        break;
    }
}

void NTDataValue::pack(msgpack::Packer<false> &packer) const
{
    switch (type)
    {
    case NTDataType::Bool:
        packer.process(b);
        break;
    case NTDataType::Float64:
        packer.process(f64);
        break;
    case NTDataType::Int:
        packer.process(i);
        break;
    case NTDataType::Float32:
        packer.process(f32);
        break;
    case NTDataType::Str:
    case NTDataType::Json:
        packer.process(str);
        break;
    case NTDataType::Bin:
    case NTDataType::Raw:
    case NTDataType::Msgpack:
    case NTDataType::Protobuf:
        packer.process(bin);
        break;
    case NTDataType::UInt:
        packer.process(ui);
        break;
    case NTDataType::BoolArray:
        packer.process(bArray);
        break;
    case NTDataType::Float64Array:
        packer.process(f64Array);
        break;
    case NTDataType::IntArray:
        packer.process(iArray);
        break;
    case NTDataType::Float32Array:
        packer.process(f32Array);
        break;
    case NTDataType::StrArray:
        packer.process(strArray);
        break;
    default:
        break;
    }
}

NTDataValue::NTDataValue(NTDataType type, msgpack::Unpacker<false> &unpacker) : type(type)
{
    if (type == NTDataType::Int && unpacker.is_next_unsigned())
        this->type = NTDataType::UInt;
    unpack(unpacker);
}

NTDataValue::NTDataValue(bool b) : type(NTDataType::Bool), b(b) {}
NTDataValue::NTDataValue(double f64) : type(NTDataType::Float64), f64(f64) {}
NTDataValue::NTDataValue(int64_t i) : type(NTDataType::Int), i(i) {}
NTDataValue::NTDataValue(float f32) : type(NTDataType::Float32), f32(f32) {}
NTDataValue::NTDataValue(std::string str) : type(NTDataType::Str), str(str) {}
NTDataValue::NTDataValue(NTDataType type, std::string str) : type(type), str(str) {}
NTDataValue::NTDataValue(std::vector<uint8_t> bin) : type(NTDataType::Bin), bin(bin) {}
NTDataValue::NTDataValue(NTDataType type, std::vector<uint8_t> bin) : type(type), bin(bin) {}
NTDataValue::NTDataValue(uint64_t ui) : type(NTDataType::UInt), ui(ui) {}
NTDataValue::NTDataValue(std::list<bool> bArray) : type(NTDataType::BoolArray), bArray(bArray) {}
NTDataValue::NTDataValue(std::list<double> f64Array) : type(NTDataType::Float64Array), f64Array(f64Array) {}
NTDataValue::NTDataValue(std::list<int64_t> iArray) : type(NTDataType::IntArray), iArray(iArray) {}
NTDataValue::NTDataValue(std::list<float> f32Array) : type(NTDataType::Float32Array), f32Array(f32Array) {}
NTDataValue::NTDataValue(std::list<std::string> strArray) : type(NTDataType::StrArray), strArray(strArray) {}

NTDataValue::NTDataValue(const NTDataValue &other) : type(other.type)
{
    switch (other.type)
    {
    case NTDataType::Bool:
        b = other.b;
        break;
    case NTDataType::Float64:
        f64 = other.f64;
        break;
    case NTDataType::Int:
        i = other.i;
        break;
    case NTDataType::Float32:
        f32 = other.f32;
        break;
    case NTDataType::Str:
        str = other.str;
        break;
    case NTDataType::Bin:
        bin = std::vector<uint8_t>(other.bin);
        break;
    case NTDataType::UInt:
        ui = other.ui;
        break;
    case NTDataType::BoolArray:
        bArray = other.bArray;
        break;
    case NTDataType::Float64Array:
        f64Array = other.f64Array;
        break;
    case NTDataType::IntArray:
        iArray = other.iArray;
        break;
    case NTDataType::Float32Array:
        f32Array = other.f32Array;
        break;
    case NTDataType::StrArray:
        strArray = other.strArray;
        break;
    default:
        break;
    }
}

NTDataValue::~NTDataValue()
{
}

static constexpr int NT4_SERVER_PORT = 5810;
static constexpr std::string_view NT_PROTOCOL = "v4.1.networktables.first.wpi.edu"sv;

NetworkTableInstance::NetworkTableInstance() : networkMode(NetworkMode::Starting), serverTimeOffset(0)
{
}

NetworkTableInstance::~NetworkTableInstance()
{
    close();
}

void NetworkTableInstance::startClient(std::string_view url)
{
    stop();
    networkMode = NetworkMode::Client;
    client = new WebSocket(url, {std::string(NT_PROTOCOL)});
    client->callbackArgs = this;
}

void NetworkTableInstance::startServer()
{
    stop();
    serverTimeOffset = 0;
    server = new WsServer(NT4_SERVER_PORT);
    server->callbackArgs = this;
    thisClient = {};

    // Accept NT_PROTOCOL only
    server->protocolCallback = [](const std::vector<std::string> &requestedProtocols, void *args) -> std::string_view
    {
        if (std::find(requestedProtocols.begin(), requestedProtocols.end(), NT_PROTOCOL) != requestedProtocols.end())
            return NT_PROTOCOL;
        else
            return ""sv;
    };

    server->clientConnected.Add([](WsServer *server, const WsServer::ClientEntry *entry, void *args)
                                { NetworkTableInstance *inst = (NetworkTableInstance *)args; 
                                ClientData* data = new ClientData();
                                data->guid = entry->guid;
                                std::size_t nameStart = entry->requestedPath.find("/nt/"sv) + 4;
                                auto name = entry->requestedPath.substr(nameStart);
                                data->name = name + "@"s + std::to_string(inst->nextClientWithName(name));
                                data->publishers = {};
                                data->subscriptions = {};
                                data->topicData = {};
                                inst->clients[entry->guid] = data;

                                inst->publishTopic("$clientsub$"s + data->name, NTDataValue(NTDataType::Msgpack, std::vector<uint8_t>{}), {.retained = true, .cached = true});
                                inst->publishTopic("$clientpub$"s + data->name, NTDataValue(NTDataType::Msgpack, std::vector<uint8_t>{}), {.retained = true, .cached = true});
                                inst->updateClientsMetaTopic();
                                inst->updateClientSubMetaTopic(entry->guid);
                                inst->updateClientPubMetaTopic(entry->guid);
                                inst->flushText(data);
                                inst->publishInitialValues(entry->guid);
                                inst->flushBinary(data); });

    server->clientDisconnected.Add([](WsServer *server, const Guid &guid, WebSocketStatusCode statusCode, const std::string_view &reason, void *args)
                                   {
                                       NetworkTableInstance *inst = (NetworkTableInstance *)args;
                                       ClientData *data = inst->clients[guid];
                                       inst->clients.erase(guid);
                                       for (auto o : data->subscriptions)
                                           delete o.second;
                                       for (auto o : data->publishers)
                                           delete o.second;
                                       delete data;
                                       inst->updateClientsMetaTopic();
                                       inst->flushBinary(); });

    server->messageReceived.Add([](WsServer *server, const Guid &guid, const WebSocketFrame &frame, void *args)
                                {
                                    if (!frame.isFragment)
                                    {
                                        NetworkTableInstance *inst = (NetworkTableInstance *)args;
                                        inst->_handleFrame(guid, frame);
                                    } });

    server->start();
    networkMode = NetworkMode::Server;

    publishTopic("$clients"s, NTDataValue(NTDataType::Msgpack, std::vector<uint8_t>{}), {.retained = true, .cached = true});
    publishTopic("$serversub"s, NTDataValue(NTDataType::Msgpack, std::vector<uint8_t>{}), {.retained = true, .cached = true});
    publishTopic("$serverpub"s, NTDataValue(NTDataType::Msgpack, std::vector<uint8_t>{}), {.retained = true, .cached = true});

    updateClientsMetaTopic();
    updateServerSubMetaTopic();
    updateServerPubMetaTopic();

    publishTopic("/SmartDashboard/testvalue", {(int64_t)5}, {.retained = true, .cached = true});

    flushText();
    flushBinary();
}

void NetworkTableInstance::_handleFrame(const Guid &guid, const WebSocketFrame &frame)
{
    switch (frame.opcode)
    {
    case WebSocketOpCode::TextFrame:
    {
        printf("%.*s\n", frame.payloadLength, frame.payload);

        std::vector<std::string> topics{};

        auto unpacker = json::Unpacker();
        unpacker.set_data(frame.payload, frame.payloadLength);
        unpacker.unpack_array();
        json::DataType nextType;

        do
        {
            unpacker.unpack_object(); // the message
            unpacker.unpack_key();
            auto method = unpacker.unpack_string();
            unpacker.unpack_key();
            unpacker.unpack_object(); // message params
            switch (parseMessageMethod(method))
            {
            case MessageMethod::Subscribe:
            {
                if (networkMode != NetworkMode::Server)
                    break;

                topics.clear();
                int32_t subuid;
                SubscriptionOptions options = SubscriptionOptions_DEFAULT; // TODO: replace default with already set options (if exist)
                for (int i = 0; i < 3; i++)
                {
                    auto key = unpacker.unpack_key();
                    if (key == "topics"sv)
                    {
                        unpacker.unpack_array();
                        json::DataType type;
                        while (
                            unpacker.peek_type(&type) != json::UnpackerError::OutOfRange &&
                            type != json::ArrayEnd)
                        {
                            topics.push_back(std::string(unpacker.unpack_string()));
                        }
                        unpacker.unpack_array_end();
                    }
                    else if (key == "subuid"sv)
                    {
                        subuid = unpacker.unpack_int();
                    }
                    else if (key == "options"sv)
                    {
                        unpacker.unpack_object();
                        json::DataType type;
                        while (
                            unpacker.peek_type(&type) != json::UnpackerError::OutOfRange &&
                            type != json::ObjectEnd)
                        {
                            auto mkey = unpacker.unpack_key();
                            if (mkey == "periodic"sv && unpacker.peek_type() == json::Int)
                            {
                                options.periodic = unpacker.unpack_int();
                            }
                            else if (mkey == "all"sv && unpacker.is_bool(unpacker.peek_type()))
                            {
                                options.all = unpacker.unpack_bool();
                            }
                            else if (mkey == "topicsonly"sv && unpacker.is_bool(unpacker.peek_type()))
                            {
                                options.topicsonly = unpacker.unpack_bool();
                            }
                            else if (mkey == "prefix"sv && unpacker.is_bool(unpacker.peek_type()))
                            {
                                options.prefix = unpacker.unpack_bool();
                            }
                        }
                        unpacker.unpack_object_end();
                    }
                }

                if (!clients[guid]->subscriptions[subuid])
                {
                    clients[guid]->subscriptions[subuid] = new Subscription(subuid, topics, options);
                }
                else
                {
                    clients[guid]->subscriptions[subuid]->uid = subuid;
                    clients[guid]->subscriptions[subuid]->topics = topics;
                    clients[guid]->subscriptions[subuid]->options = options;
                }

                updateClientSubMetaTopic(guid);

                for (auto topic : this->topics)
                {
                    if (!topic.first.starts_with('$')) // don't update meta-topic subscriptions
                    {
                        if (isSubscribed(clients[guid]->subscriptions[subuid], topic.first))
                        {
                            updateTopicSubMetaTopic(topic.first);
                        }
                    }
                }

                announceCachedTopics(guid);
                break;
            }
            default:
                break;
            }
            unpacker.unpack_object_end(); // message params
            unpacker.unpack_object_end(); // the message
        } while (unpacker.ec != json::UnpackerError::OutOfRange && unpacker.peek_type(&nextType) != json::UnpackerError::OutOfRange && nextType != json::ArrayEnd);

        unpacker.unpack_array_end();

        flushText(clients[guid]);
        publishInitialValues(guid);
        flushBinary();
        break;
    }
    case WebSocketOpCode::BinaryFrame:
    {
        // All valid binary frames are message pack arrays
        auto unpacker = msgpack::Unpacker();
        unpacker.set_data(frame.payload, frame.payloadLength);
        do
        {
            std::size_t array_size = unpacker.unpack_array_header();
            if (array_size == 4)
            {
                int64_t id;
                uint64_t timestamp;
                uint8_t _type;
                unpacker.process(id);
                unpacker.process(timestamp);
                unpacker.process(_type);

                NTDataValue data = {(NTDataType)_type, unpacker};

                if (id == -1) // RTT measurement
                {
                    switch (networkMode)
                    {
                    case NetworkMode::Server:
                    {
                        auto packer = msgpack::Packer();
                        packer.pack_array_header(4);
                        packer.process(id);
                        timestamp = getServerTime();
                        packer.process(timestamp);
                        packer.process(_type);
                        data.pack(packer);
                        server->send(guid, packer.vector());
                        break;
                    }
                    case NetworkMode::Client:
                    {
                        if (data.type == NTDataType::UInt)
                        {
                            uint64_t rtt = get_absolute_time() - data.ui;
                            uint64_t serverTime = timestamp + rtt / 2;
                            uint64_t time = get_absolute_time();
                            if (serverTime > time)
                                serverTimeOffset = (int64_t)(serverTime - time);
                            else
                                serverTimeOffset = -(int64_t)(time - serverTime);
                        }
                        else if (data.type == NTDataType::Int)
                        {
                            int64_t rtt = (int64_t)get_absolute_time() - data.i;
                            uint64_t serverTime = timestamp + (uint64_t)(rtt / 2);
                            uint64_t time = get_absolute_time();
                            if (serverTime > time)
                                serverTimeOffset = (int64_t)(serverTime - time);
                            else
                                serverTimeOffset = -(int64_t)(time - serverTime);
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }
                else
                {
                    auto it = std::find_if(server->clients.begin(), server->clients.end(),
                                           [&guid](auto &&p)
                                           { return p->guid == guid; });

                    if (it != server->clients.end())
                    {
                        std::string_view path = (*it)->requestedPath;
                        printf("Binary/%.*s@%i at %u / %u\n", path.length(), path.data(), id, timestamp, _type);
                    }
                }
            }
        } while (unpacker.ec != msgpack::UnpackerError::OutOfRange);
        break;
    }
    default:
        break;
    }
}

bool NetworkTableInstance::sendRTT()
{
    assert(networkMode == NetworkMode::Client); // can only get rtt if client

    auto packer = msgpack::Packer();
    int64_t id = -1;
    uint64_t timestamp = 0;
    uint8_t _type = (uint8_t)NTDataType::Int;
    uint64_t data = get_absolute_time();
    packer.pack_array_header(4);
    packer.process(id);
    packer.process(timestamp);
    packer.process(_type);
    packer.process(data);
    return client->send(packer.vector());
}

bool NetworkTableInstance::announceTopic(const Guid &guid, const Topic *topic)
{
    assert(networkMode == NetworkMode::Server);

    ClientData *client = clients[guid];
    assert(client != nullptr);

    if (!client->topicData.contains(topic->name))
    {
        client->topicData[topic->name] = {
            client->nextTopicIdAssigned++,
            false};
    }
    else
    {
        // TODO: properly handle re-announcements
        return true;
    }

    int64_t id = client->topicData[topic->name].id;
    std::string text = "{\"method\":\"announce\",\"params\":{\"name\":\""s +
                       topic->name +
                       "\",\"id\":"s +
                       std::to_string(id) +
                       ",\"type\":\""s +
                       serializeDataType(topic->value.type) +
                       "\",\"properties\":{\"persistent\":"s +
                       (topic->properties.persistent
                            ? "true"s
                            : "false"s) +
                       ",\"retained\":"s +
                       (topic->properties.retained
                            ? "true"s
                            : "false"s) +
                       ",\"cached\":"s +
                       (topic->properties.cached
                            ? "true"s
                            : "false"s) +
                       "}}}"s;
    flushText(client, text.length());
    if (client->textCache.length() > 0)
        client->textCache.append(","sv); // comma separator
    client->textCache.append(text);
    return true;
}

bool NetworkTableInstance::isSubscribed(Subscription *subscription, std::string name)
{
    for (auto top : subscription->topics)
    {
        if (subscription->options.prefix)
        {
            if (name.starts_with('$'))
            {
                if (top.starts_with('$') && name.starts_with(top))
                {
                    return true;
                }
            }
            else if (top.length() == 0 || name.starts_with(top))
            {
                return true;
            }
        }
        else if (name == top)
        {
            return true;
        }
    }

    return false;
}

bool NetworkTableInstance::isSubscribed(const std::unordered_map<int32_t, Subscription *> &subscriptions, std::string name, Subscription **out_subscription)
{
    for (auto sub : subscriptions)
    {
        for (auto top : sub.second->topics)
        {
            if (sub.second->options.prefix)
            {
                if (name.starts_with('$'))
                {
                    if (top.starts_with('$') && name.starts_with(top))
                    {
                        if (out_subscription != nullptr)
                            *out_subscription = sub.second;
                        return true;
                    }
                }
                else if (top.length() == 0 || name.starts_with(top))
                {
                    if (out_subscription != nullptr)
                        *out_subscription = sub.second;
                    return true;
                }
            }
            else if (name == top)
            {
                if (out_subscription != nullptr)
                    *out_subscription = sub.second;
                return true;
            }
        }
    }

    return false;
}

bool NetworkTableInstance::announceTopic(const Topic *topic)
{
    assert(networkMode == NetworkMode::Server);
    bool ok = true;
    for (auto client : clients)
    {
        if (isSubscribed(client.second->subscriptions, topic->name))
        {
            if (!announceTopic(client.first, topic))
                ok = false;
        }
    }
    return ok;
}

bool NetworkTableInstance::announceCachedTopics(const Guid &guid)
{
    assert(networkMode == NetworkMode::Server);
    bool ok = true;
    ClientData *client = clients[guid];
    assert(client != nullptr);
    for (auto topic : topics)
    {
        if (topic.second->properties.cached && isSubscribed(client->subscriptions, topic.second->name))
        {
            if (!announceTopic(guid, topic.second)) // announce topic if cached and subscribed
                ok = false;
        }
    }
    return ok;
}

bool NetworkTableInstance::sendTopicUpdate(const Guid &guid, const Topic *topic)
{
    assert(networkMode == NetworkMode::Server);

    auto packer = msgpack::Packer();
    auto client = clients[guid];
    assert(client != nullptr);

    int64_t id = client->topicData[topic->name].id;
    uint64_t timestamp = getServerTime();
    uint8_t _type = (uint8_t)topic->value.getAPIType();
    packer.clear();
    packer.pack_array_header(4);
    packer.process(id);
    packer.process(timestamp);
    packer.process(_type);
    topic->value.pack(packer);
    client->topicData[topic->name].initialPublish = true;

    auto bin = packer.vector();
    flushBinary(client, bin.size());
    client->binaryCache.insert(std::end(client->binaryCache), std::begin(bin), std::end(bin));
    return true;
}

bool NetworkTableInstance::sendTopicUpdate(const Topic *topic)
{
    assert(networkMode == NetworkMode::Server);

    for (auto client : clients)
    {
        if (client.second->topicData.contains(topic->name))
        {
            auto t = client.second->topicData[topic->name];
            if (t.initialPublish)
            {
                if (!sendTopicUpdate(client.first, topic))
                    return false;
            }
        }
    }

    return true;
}

bool NetworkTableInstance::publishTopic(std::string name, NTDataValue value, TopicProperties properties)
{
    Topic *topic = getOrCreateTopic(name, value, properties);
    if (!announceTopic(topic))
        return false;

    if (!name.starts_with('$'))
    {
        if (!publishTopic("$sub$"s + name, NTDataValue(NTDataType::Msgpack, std::vector<uint8_t>{}), {.retained = true, .cached = true}))
            return false;
        if (!publishTopic("$pub$"s + name, NTDataValue(NTDataType::Msgpack, std::vector<uint8_t>{}), {.retained = true, .cached = true}))
            return false;
        updateTopicSubMetaTopic(name);
        updateTopicPubMetaTopic(name);
    }

    return true;
}

size_t NetworkTableInstance::nextClientWithName(std::string_view name)
{
    size_t num = 1;

    for (auto client : clients)
    {
        if (client.second->name.starts_with(name) && client.second->name.find('@') == name.length())
        {
            size_t i;
            if (std::from_chars(client.second->name.data() + name.length() + 1, client.second->name.data() + client.second->name.size(), i).ec != std::errc::invalid_argument)
            {
                num = std::max(num, i + 1);
            }
        }
    }

    return num;
}

void NetworkTableInstance::publishInitialValues(const Guid &guid)
{
    for (auto topic : clients[guid]->topicData)
    {
        if (!topic.second.initialPublish)
        {
            sendTopicUpdate(guid, topics[topic.first]);
        }
    }
}

void NetworkTableInstance::stop()
{
    switch (networkMode)
    {
    case NetworkMode::Client:
    {
        client->close();
        delete client;
        client = nullptr;
        break;
    }
    case NetworkMode::Server:
    {
        server->stop();
        delete server;
        server = nullptr;
        break;
    }
    default:
        break;
    }

    networkMode = NetworkMode::Starting;
}

void NetworkTableInstance::close()
{
    stop();
}

bool NetworkTableInstance::isConnected()
{
    switch (networkMode)
    {
    case NetworkMode::Client:
        return client->isConnected();
    case NetworkMode::Server:
        return server->isListening();
    default:
        return false;
    }
}

NetworkTableInstance::NetworkMode NetworkTableInstance::getNetworkMode()
{
    return networkMode;
}

int64_t NetworkTableInstance::getServerTimeOffset()
{
    return serverTimeOffset;
}

uint64_t NetworkTableInstance::getServerTime()
{
    if (serverTimeOffset > 0)
        return get_absolute_time() + (uint64_t)serverTimeOffset;
    else
        return get_absolute_time() - (uint64_t)(-serverTimeOffset);
}

void NetworkTableInstance::flushText(ClientData *client, std::size_t uncachedSize)
{
    client->textCache.reserve(MAX_CLIENT_TEXT_CACHE_LENGTH);
    if (client->textCache.length() > 0 && client->textCache.length() + uncachedSize + 2 /* '[' around ']' */ > MAX_CLIENT_TEXT_CACHE_LENGTH)
    {
        server->send(client->guid, "["s + client->textCache + "]"s /* encase messages in json array */);
        client->textCache.clear();
    }
}

void NetworkTableInstance::flushBinary(ClientData *client, std::size_t uncachedSize)
{
    client->binaryCache.reserve(MAX_CLIENT_BINARY_CACHE_LENGTH);
    if (client->binaryCache.size() > 0 && client->binaryCache.size() + uncachedSize > MAX_CLIENT_BINARY_CACHE_LENGTH)
    {
        server->send(client->guid, client->binaryCache);
        client->binaryCache.clear();
    }
}

struct ClientsMetaTopic
{
    std::string id;
    std::string conn;

    template <class T>
    void pack(T &pack)
    {
        pack(nvp(id), nvp(conn));
    }
};

void NetworkTableInstance::updateClientsMetaTopic()
{
    auto packer = msgpack::Packer<true>();
    packer.pack_array_header(clients.size());
    for (auto client : server->clients)
    {
        if (clients.contains(client->guid))
        {
            auto addr = client->ws->getSocketAddress();
            std::string host = ip4addr_ntoa((ip4_addr_t *)&addr.sin_addr);
            ClientsMetaTopic topic = {clients[client->guid]->name, host + ":"s + std::to_string(addr.sin_port)};
            topic.pack(packer);
        }
    }

    auto t = topics["$clients"s];
    assert(t != nullptr);
    t->value.bin = packer.vector();
    sendTopicUpdate(t);
}

void NetworkTableInstance::updateServerSubMetaTopic()
{
    auto packer = msgpack::Packer<true>();
    packer.pack_array_header(thisClient.subscriptions.size());
    for (auto sub : thisClient.subscriptions)
    {
        sub.second->pack(packer);
    }

    auto t = topics["$serversub"s];
    assert(t != nullptr);
    t->value.bin = packer.vector();
    sendTopicUpdate(t);
}

void NetworkTableInstance::updateServerPubMetaTopic()
{
    auto packer = msgpack::Packer<true>();
    packer.pack_array_header(thisClient.publishers.size());
    for (auto pub : thisClient.publishers)
    {
        pub.second->pack(packer);
    }

    auto t = topics["$serverpub"s];
    assert(t != nullptr);
    t->value.bin = packer.vector();
    sendTopicUpdate(t);
}

void NetworkTableInstance::updateClientSubMetaTopic(const Guid &guid)
{
    auto packer = msgpack::Packer<true>();
    ClientData *client = clients[guid];

    packer.pack_array_header(client->subscriptions.size());
    for (auto sub : client->subscriptions)
    {
        sub.second->pack(packer);
    }

    auto t = topics["$clientsub$"s + client->name];
    assert(t != nullptr);
    t->value.bin = packer.vector();
    sendTopicUpdate(t);
}

void NetworkTableInstance::updateClientPubMetaTopic(const Guid &guid)
{
    auto packer = msgpack::Packer<true>();
    ClientData *client = clients[guid];

    packer.pack_array_header(client->publishers.size());
    for (auto pub : client->publishers)
    {
        pub.second->pack(packer);
    }

    auto t = topics["$clientpub$"s + client->name];
    assert(t != nullptr);
    t->value.bin = packer.vector();
    sendTopicUpdate(t);
}

void NetworkTableInstance::updateTopicSubMetaTopic(std::string name)
{
    std::vector<TopicSubscription> subs = {};

    for (auto client : clients)
    {
        Subscription *sub;
        if (isSubscribed(client.second->subscriptions, name, &sub))
        {
            subs.push_back(TopicSubscription(client.second->name, sub->uid, sub->options));
        }
    }

    auto packer = msgpack::Packer<true>();
    packer.pack_array_header(subs.size());
    for (auto sub : subs)
    {
        sub.pack(packer);
    }

    auto t = topics["$sub$"s + name];
    assert(t != nullptr);
    t->value.bin = packer.vector();
    sendTopicUpdate(t);
}

void NetworkTableInstance::updateTopicPubMetaTopic(std::string name)
{
    std::vector<TopicPublisher> pubs = {};

    for (auto client : clients)
    {
        for (auto pub : client.second->publishers)
        {
            if (pub.second->topic == name)
            {
                pubs.push_back(TopicPublisher(client.second->name, pub.second->uid));
            }
        }
    }

    auto packer = msgpack::Packer<true>();
    packer.pack_array_header(pubs.size());
    for (auto pub : pubs)
    {
        pub.pack(packer);
    }

    auto t = topics["$pub$"s + name];
    assert(t != nullptr);
    t->value.bin = packer.vector();
    sendTopicUpdate(t);
}

void NetworkTableInstance::setTestValue(int64_t value)
{
    auto t = topics["/SmartDashboard/testvalue"s];
    assert(t != nullptr);
    t->value.i = value;
    sendTopicUpdate(t);
    flushBinary();
}
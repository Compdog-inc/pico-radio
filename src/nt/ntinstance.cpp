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

void NTDataValue::unpack(msgpack::Unpacker &unpacker)
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
        unpacker.process(str);
        break;
    case NTDataType::Bin:
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

void NTDataValue::pack(msgpack::Packer &packer)
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
        packer.process(str);
        break;
    case NTDataType::Bin:
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

NTDataValue::NTDataValue(NTDataType type, msgpack::Unpacker &unpacker) : type(type)
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
NTDataValue::NTDataValue(std::vector<uint8_t> bin) : type(NTDataType::Bin), bin(bin) {}
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

static const std::vector<uint8_t> _clients = std::vector<uint8_t>{
    0x91, 0x82, 0xA2, 0x69, 0x64, 0xA5, 0x67, 0x6C, 0x61, 0x73, 0x73, 0xA4, 0x63, 0x6F, 0x6E, 0x6E, 0xB1, 0x31, 0x30, 0x2E, 0x36, 0x37, 0x2E, 0x33, 0x31, 0x2E, 0x31, 0x36, 0x3A, 0x35, 0x31, 0x38, 0x32, 0x33};
static const std::vector<uint8_t> glass = std::vector<uint8_t>{0x92, 0x83, 0xA3, 0x75, 0x69, 0x64, 0xCE, 0x16, 0x00, 0x00, 0x00, 0xA6, 0x74, 0x6F, 0x70, 0x69, 0x63, 0x73, 0x92, 0xA0, 0xA1, 0x24, 0xA7, 0x6F, 0x70, 0x74, 0x69, 0x6F, 0x6E, 0x73, 0x81, 0xA6, 0x70, 0x72, 0x65, 0x66, 0x69, 0x78, 0xC3, 0x83, 0xA3, 0x75, 0x69, 0x64, 0xCE, 0x16, 0x00, 0x00, 0x01, 0xA6, 0x74, 0x6F, 0x70, 0x69, 0x63, 0x73, 0x91, 0xA0, 0xA7, 0x6F, 0x70, 0x74, 0x69, 0x6F, 0x6E, 0x73, 0x82, 0xAA, 0x74, 0x6F, 0x70, 0x69, 0x63, 0x73, 0x6F, 0x6E, 0x6C, 0x79, 0xC3, 0xA6, 0x70, 0x72, 0x65, 0x66, 0x69, 0x78, 0xC3};
static const std::vector<uint8_t> serverSub = std::vector<uint8_t>{0x92, 0x83, 0xA3, 0x75, 0x69, 0x64, 0xCE, 0x16, 0x00, 0x00, 0x00, 0xA6, 0x74, 0x6F, 0x70, 0x69, 0x63, 0x73, 0x92, 0xA0, 0xA1, 0x24, 0xA7, 0x6F, 0x70, 0x74, 0x69, 0x6F, 0x6E, 0x73, 0x81, 0xA6, 0x70, 0x72, 0x65, 0x66, 0x69, 0x78, 0xC3, 0x83, 0xA3, 0x75, 0x69, 0x64, 0xCE, 0x16, 0x00, 0x00, 0x01, 0xA6, 0x74, 0x6F, 0x70, 0x69, 0x63, 0x73, 0x91, 0xA0, 0xA7, 0x6F, 0x70, 0x74, 0x69, 0x6F, 0x6E, 0x73, 0x82, 0xAA, 0x74, 0x6F, 0x70, 0x69, 0x63, 0x73, 0x6F, 0x6E, 0x6C, 0x79, 0xC3, 0xA6, 0x70, 0x72, 0x65, 0x66, 0x69, 0x78, 0xC3};

void NetworkTableInstance::startServer()
{
    stop();
    serverTimeOffset = 0;
    server = new WsServer(NT4_SERVER_PORT);
    server->callbackArgs = this;

    topics["$clients"s] = new Topic("$clients"s, {_clients});
    topics["$clientsub$glass"s] = new Topic("$clientsub$glass"s, {glass});
    topics["$serversub"s] = new Topic("$serversub"s, {serverSub});

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
                                data->name = entry->requestedPath;
                                data->publishers = {};
                                data->subscriptions = {};
                                data->topicIds = {
                                    {"$clients"s, 0},
                                    {"$clientsub$glass"s, 1},
                                    {"$serversub"s, 2}
                                };
                                inst->clients[entry->guid] = data; });

    server->clientDisconnected.Add([](WsServer *server, const Guid &guid, WebSocketStatusCode statusCode, const std::string_view &reason, void *args)
                                   { NetworkTableInstance *inst = (NetworkTableInstance *)args;
        ClientData *data = inst->clients[guid];
        inst->clients.erase(guid);
        for (auto o : data->subscriptions)
            delete o.second;
        for (auto o : data->publishers)
            delete o.second;
        delete data; });

    server->messageReceived.Add([](WsServer *server, const Guid &guid, const WebSocketFrame &frame, void *args)
                                {
                                    if (!frame.isFragment)
                                    {
                                        NetworkTableInstance *inst = (NetworkTableInstance *)args;
                                        inst->_handleFrame(guid, frame);
                                    } });

    server->start();
    networkMode = NetworkMode::Server;
}

void NetworkTableInstance::_handleFrame(const Guid &guid, const WebSocketFrame &frame)
{
    switch (frame.opcode)
    {
    case WebSocketOpCode::TextFrame:
    {
        printf("%.*s\n", frame.payloadLength, frame.payload);

        auto unpacker = json::Unpacker();
        unpacker.set_data(frame.payload, frame.payloadLength);
        unpacker.unpack_array();

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

                std::vector<std::string> topics{};
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
                    clients[guid]->subscriptions[subuid] = new Subscription();
                }

                clients[guid]->subscriptions[subuid]->uid = subuid;
                clients[guid]->subscriptions[subuid]->topics = topics;
                clients[guid]->subscriptions[subuid]->options = options;

                announceTopics(guid); // TODO: replace, don't announce all, only announce changes to clients/sub meta topics
                break;
            }
            default:
                break;
            }
            unpacker.unpack_object_end(); // message params
            unpacker.unpack_object_end(); // the message
        } while (unpacker.ec != json::UnpackerError::OutOfRange);
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

    int64_t id = clients[guid]->topicIds[topic->name];
    return server->send(guid,
                        "[{\"method\":\"announce\",\"params\":{\"name\":\""s +
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
                            "}}}]"s);
}

bool NetworkTableInstance::isSubscribed(const std::unordered_map<int32_t, Subscription *> &subscriptions, std::string name)
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
                        return true;
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

bool NetworkTableInstance::announceTopics(const Guid &guid)
{
    assert(networkMode == NetworkMode::Server);
    bool ok = true;
    auto subs = clients[guid]->subscriptions;
    for (auto top : topics)
    {
        if (isSubscribed(subs, top.second->name))
        {
            if (!announceTopic(guid, top.second))
                ok = false;
        }
    }
    return ok;
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
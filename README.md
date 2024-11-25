# pico-radio
A network radio library for the Raspberry PI Pico W

### Features:

- Provides a Radio class for simple cyw43 driver initialization
- Built in DHCP server for Access Point mode
- Async TCP client/listener classes
- TextStream class for text based interaction with TCP clients
- UDP socket implementation with both connect and bind modes and support for broadcasting on the local network interface
- Event based WebSocket client/server implementation with nearly complete RFC 6455 specification
- Full NetworkTables v4.1 (NT4) client/server implementation

### Config Options (CMake)

- `PICO_RADIO_IP_MASKED` (default `10,67,31`). Specifies the first 3 parts of the IPv4 address to use for the radio. By default, the radio's IP address is `10.67.31.2`
- `PICO_RADIO_AP` (default `false or 0`). Should the radio run in Access Point mode?
- `PICO_RADIO_HOSTNAME` (default `"Pico-Radio"`). The radio hostname to use
- `PICO_RADIO_SSID` (default `"PicoWifi"`). The SSID of the Access Point or network to connect the radio to.
- `PICO_RADIO_PASSWORD` (default `none`). The password of the Access Point or network to connect the radio to. Don't define to use an open wifi.
- `WEBSOCKET_THREAD_STACK_SIZE` (default `4096`). The stack size of new WebSocket client threads.
- `WEBSOCKET_TIMEOUT` (default `5000`). The timeout in milliseconds of WebSocket connections. **Note:** this is not a heartbeat, only used for blocking operations or initial handshake.

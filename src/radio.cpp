#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <FreeRTOS.h>
#include <task.h>
#include <string>
#include <lwip/netif.h>
#include <lwip/ip4_addr.h>
#include <lwip/sockets.h>
#include "radio.h"
#include "config.h"

using namespace std::literals;

/// @brief Converts a lwip wifi status to a human-readable string
static const char *status_name(int status)
{
    switch (status)
    {
    case CYW43_LINK_DOWN:
        return "link down";
    case CYW43_LINK_JOIN:
        return "joining";
    case CYW43_LINK_NOIP:
        return "no ip";
    case CYW43_LINK_UP:
        return "link up";
    case CYW43_LINK_FAIL:
        return "link fail";
    case CYW43_LINK_NONET:
        return "network fail";
    case CYW43_LINK_BADAUTH:
        return "bad auth";
    }
    return "unknown";
}

/// @brief Attempts to connect a wifi network with a timeout
/// @param ssid The SSID of the network
/// @param password The password of the network (empty for an open network)
/// @param auth The auth level of the network
/// @param until absolute timestamp for the timeout
/// @return 0 on success, otherwise returns the status
int wifi_connect_until(std::string_view ssid, std::string_view password, uint32_t auth, absolute_time_t until)
{
    // initiate the connection
    int err = cyw43_wifi_join(&cyw43_state, ssid.length(), (const uint8_t *)ssid.data(), password.length(), (const uint8_t *)password.data(), auth, NULL, CYW43_CHANNEL_NONE);
    if (err)
        return err;

    int status = CYW43_LINK_UP + 1;
    while (status >= 0 && status != CYW43_LINK_UP) // continuously poll the wifi status until the timeout is hit or a connection is established
    {
        int new_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        if (new_status != status)
        {
            status = new_status;
            printf("[RADIO] Connect status: %s\n", status_name(status));
            switch (status)
            {
            case CYW43_LINK_JOIN:
            case CYW43_LINK_NOIP:
                // pulse
                break;
            case CYW43_LINK_UP:
                // on
                break;
            }
        }

        if (time_reached(until))
        {
            return PICO_ERROR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // prevent busy waiting and allow for dhcp retries
    }
    return status == CYW43_LINK_UP ? 0 : status;
}

Radio::Radio()
{
    printf("[RADIO] Initializing cyw43_arch (US)\n");
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA))
    {
        printf("[RADIO] Failed to initialize cyw43_arch\n");
        initialized = false;
        return;
    }

    netif_set_hostname(netif_default, NET_HOSTNAME);
    printf("[RADIO] Set netif hostname '%s'\n", NET_HOSTNAME);

#if WIFI_ACCESS_POINT

    assert(cyw43_is_initialized(&cyw43_state));
    cyw43_wifi_ap_set_ssid(&cyw43_state, WIFI_SSID.length(), (const uint8_t *)WIFI_SSID.data());

#if !PICO_RADIO_OPEN

    printf("[RADIO] Created encrypted access point '%.*s' / '%.*s'\n", WIFI_SSID.length(), WIFI_SSID.data(), WIFI_PASSWORD.length(), WIFI_PASSWORD.data());

    cyw43_wifi_ap_set_password(&cyw43_state, WIFI_PASSWORD.length(), (const uint8_t *)WIFI_PASSWORD.data());
    cyw43_wifi_ap_set_auth(&cyw43_state, CYW43_AUTH_WPA2_AES_PSK);

#else

    printf("[RADIO] Created open access point '%.*s'\n", WIFI_SSID.length(), WIFI_SSID.data());

    cyw43_wifi_ap_set_auth(&cyw43_state, CYW43_AUTH_OPEN);

#endif

    cyw43_wifi_set_up(&cyw43_state, CYW43_ITF_AP, true, cyw43_arch_get_country_code());

    ip4_addr_t local;
    ip4_addr_t netmask;
    ip4_addr_t gateway;

    INET_IP_MASKED(&local, 2);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    INET_IP_MASKED(&gateway, 1);

    netif_set_addr(netif_default, &local, &netmask, &gateway);

    dhcp_server_init(&dhcp_server, &gateway, &netmask);

#else

    cyw43_arch_enable_sta_mode();

#if !PICO_RADIO_OPEN

    char censoredPassword[WIFI_PASSWORD.length()];
    memset(censoredPassword, '*', WIFI_PASSWORD.length());
    printf("[RADIO] Connecting to encrypted wifi '%.*s' / '%.*s'... (30 sec)\n", WIFI_SSID.length(), WIFI_SSID.data(), WIFI_PASSWORD.length(), censoredPassword);
    int connectResult = wifi_connect_until(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, make_timeout_time_ms(300000));

#else

    printf("[RADIO] Connecting to open wifi '%.*s'... (30 sec)\n", WIFI_SSID.length(), WIFI_SSID.data());
    int connectResult = wifi_connect_until(WIFI_SSID, ""sv, CYW43_AUTH_OPEN, make_timeout_time_ms(300000));

#endif

    if (connectResult != 0)
    {
        printf("[RADIO] Failed to connect: %i\n", connectResult);
        cyw43_arch_deinit();
        initialized = false;
        return;
    }

#endif

    cyw43_wifi_pm(&cyw43_state, CYW43_DEFAULT_PM & ~0xf);
    printf("[RADIO] Wifi initialized!\n");

    initialized = true;
}

Radio::~Radio()
{
    if (initialized)
    {
        cyw43_arch_deinit();
        initialized = false;
    }
}

void Radio::deinit()
{
    if (initialized)
    {
        cyw43_arch_deinit();
        initialized = false;
    }
}

bool Radio::isInitialized()
{
    return initialized;
}
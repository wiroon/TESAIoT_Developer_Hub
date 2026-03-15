/*******************************************************************************
 * wifi_config.h — WiFi Configuration for PSoC Edge MicroPython AI
 *
 * Default: STA mode (connect to existing AP, configurable from MicroPython)
 * Supports both STA and SoftAP modes
 *******************************************************************************/

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

/* cy_wcm.h not available on CM55 (WiFi runs on CM33_NS).
 * Define security constants locally for config compatibility. */
#ifndef CY_WCM_SECURITY_WPA2_AES_PSK
#define CY_WCM_SECURITY_WPA2_AES_PSK  (6)
#endif

/* WiFi Mode: SOFTAP or STA */
#define WIFI_MODE_SOFTAP    (0)
#define WIFI_MODE_STA       (1)
#define WIFI_DEFAULT_MODE   WIFI_MODE_STA

/* SoftAP Configuration — change these to your preferred AP name/password */
#define SOFTAP_SSID             "PSoC-Edge-AP"
#define SOFTAP_PASSWORD         "changeme123"
#define SOFTAP_SECURITY         CY_WCM_SECURITY_WPA2_AES_PSK
#define SOFTAP_CHANNEL          (1)
#define SOFTAP_MAX_CLIENTS      (4)

/* SoftAP IP Configuration */
#define SOFTAP_IP_ADDRESS       "192.168.4.1"
#define SOFTAP_NETMASK          "255.255.255.0"
#define SOFTAP_GATEWAY          "192.168.4.1"

/* STA Configuration — set your WiFi SSID and password here */
#define STA_SSID                "YOUR_WIFI_SSID"
#define STA_PASSWORD            "YOUR_WIFI_PASSWORD"
#define STA_SECURITY            CY_WCM_SECURITY_WPA2_AES_PSK
#define STA_MAX_RETRIES         (10)
#define STA_RETRY_INTERVAL_MS   (2000)

#endif /* WIFI_CONFIG_H */

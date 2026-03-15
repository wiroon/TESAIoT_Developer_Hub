/**
 * @file tesaiot_license_config.h
 * @brief TESAIoT License Configuration
 *
 * This file contains your device's license credentials.
 */

#ifndef TESAIOT_LICENSE_CONFIG_H
#define TESAIOT_LICENSE_CONFIG_H

/*============================================================================
 * LICENSE CONFIGURATION
 *============================================================================*/

/**
 * Your OPTIGA Trust M UID (27 bytes as 54 hex characters)
 * NOTE: License validation is bypassed in template mode.
 *       Replace with your device UID if you enable OPTIGA (ENABLE_OPTIGA=1).
 */
#define TESAIOT_DEVICE_UID      "0000000000000000000000000000000000000000000000000000000"

/**
 * Your License Key (ECDSA signature as Base64 string)
 * NOTE: License validation is bypassed in template mode.
 *       Contact TESAIoT to obtain a license key for your device.
 */
#define TESAIOT_LICENSE_KEY     ""

/*============================================================================
 * END OF CONFIGURATION
 *============================================================================*/

#endif /* TESAIOT_LICENSE_CONFIG_H */

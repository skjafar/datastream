/**
 * @file dsUDP.h
 * @brief UDP Datastream Server
 * @author Sofian Jafar
 * @date 05/04/2021
 *
 * @defgroup udp_server UDP Server
 * @ingroup datastream
 * @brief UDP-based datastream communication server with auto-detection
 *
 * Provides a UDP server that:
 * - Listens for incoming datagrams on DS_UDP_PORT
 * - Processes datastream protocol packets
 * - Handles auto-detection/discovery protocol (if enabled)
 * - Supports multiple concurrent clients (connectionless)
 * - Automatically registers with control interface system
 *
 * The UDP server provides fast, lightweight communication ideal for:
 * - Low-latency applications
 * - Broadcast/multicast communication
 * - Network discovery
 * - Fire-and-forget commands
 *
 * @{
 */

#ifndef DSUDP_H_
#define DSUDP_H_

#include "datastream.h"

/**
 * @brief Handle to the UDP datastream task
 *
 * Can be used to:
 * - Monitor task state
 * - Delete task if needed
 * - Query task information
 */
extern TaskHandle_t dsUDPTaskHandle;

/**
 * @brief Create and start the UDP datastream server task
 *
 * Initializes the datastream subsystem and creates a FreeRTOS task
 * that runs the UDP server. The task will:
 * - Initialize datastream (calls dsInitialize())
 * - Initialize auto-detection (if DS_AUTO_DETECTION_ENABLE is set)
 * - Create a UDP socket on DS_UDP_PORT
 * - Listen for incoming datagrams
 * - Process datastream packets from any client
 * - Handle discovery requests (if auto-detection enabled)
 * - Automatically register with control interface
 *
 * Configuration (defined in ds_app_config.h or ds_default_config.h):
 * - DS_UDP_PORT: Port number to listen on
 * - DS_UDP_TASK_NAME: Task name for debugging
 * - DS_UDP_TASK_STACK_SIZE: Stack size in words
 * - DS_UDP_TASK_PRIORITY: FreeRTOS task priority
 * - DS_AUTO_DETECTION_ENABLE: Enable discovery protocol (0/1)
 *
 * @note If task creation fails, enters infinite loop
 * @note UDP server can handle multiple clients simultaneously (connectionless)
 * @note Task is automatically registered as ds_control_UDP_DATASTREAM
 * @note Auto-detection allows network scanning to find boards
 *
 * Example usage:
 * @code
 * void app_main(void) {
 *     // Initialize network stack first
 *     ESP_ERROR_CHECK(esp_netif_init());
 *     ESP_ERROR_CHECK(example_connect());
 *
 *     // Start UDP datastream server with auto-detection
 *     dsUDPTaskCreate();
 *
 *     // Clients can now:
 *     // 1. Send discovery broadcasts to find the board
 *     // 2. Send datastream packets to read/write registers/parameters
 * }
 * @endcode
 */
void dsUDPTaskCreate(void);

/** @} */ // end of udp_server group

#endif /* DSUDP_H_ */
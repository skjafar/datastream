/**
 * @file dsTCP.h
 * @brief TCP Datastream Server
 * @author Sofian Jafar
 * @date 3/22/2021
 *
 * @defgroup tcp_server TCP Server
 * @ingroup datastream
 * @brief TCP-based datastream communication server
 *
 * Provides a TCP server that:
 * - Listens for incoming client connections on DS_TCP_PORT
 * - Processes datastream protocol packets
 * - Handles one client at a time (FREERTOS_SO_REUSE_LISTEN_SOCKET)
 * - Automatically registers with control interface system
 *
 * The TCP server provides reliable, connection-oriented communication
 * ideal for applications requiring guaranteed packet delivery.
 *
 * @{
 */

#ifndef DSTCP_H_
#define DSTCP_H_

#include "datastream.h"

/**
 * @brief Handle to the TCP datastream task
 *
 * Can be used to:
 * - Monitor task state
 * - Delete task if needed
 * - Query task information
 */
extern TaskHandle_t dsTCPTaskHandle;

/**
 * @brief Create and start the TCP datastream server task
 *
 * Initializes the datastream subsystem and creates a FreeRTOS task
 * that runs the TCP server. The task will:
 * - Initialize datastream (calls dsInitialize())
 * - Create a TCP socket on DS_TCP_PORT
 * - Listen for incoming connections
 * - Process datastream packets from connected clients
 * - Automatically register with control interface
 *
 * Configuration (defined in ds_app_config.h or ds_default_config.h):
 * - DS_TCP_PORT: Port number to listen on
 * - DS_TCP_TASK_NAME: Task name for debugging
 * - DS_TCP_TASK_STACK_SIZE: Stack size in words
 * - DS_TCP_TASK_PRIORITY: FreeRTOS task priority
 *
 * @note If task creation fails, enters infinite loop
 * @note TCP server handles one client at a time
 * @note Task is automatically registered as ds_control_TCP_DATASTREAM
 *
 * Example usage:
 * @code
 * void app_main(void) {
 *     // Initialize network stack first
 *     ESP_ERROR_CHECK(esp_netif_init());
 *     ESP_ERROR_CHECK(example_connect());
 *
 *     // Start TCP datastream server
 *     dsTCPTaskCreate();
 * }
 * @endcode
 */
void dsTCPTaskCreate(void);

/** @} */ // end of tcp_server group

#endif /* DSTCP_H_ */
/**
 * @file dsCLI.h
 * @brief TCP CLI Server for Datastream
 * @author Sofian Jafar
 * @date 2/7/2026
 *
 * @defgroup cli_server CLI Server
 * @ingroup datastream
 * @brief Text-based CLI interface over TCP
 *
 * Provides a human-readable CLI over TCP for interacting with the
 * datastream register and parameter system. Useful for debugging,
 * configuration, and monitoring via telnet or netcat.
 *
 * Features:
 * - Read/write individual registers and parameters
 * - Dump all registers (with RO/RW markers) or parameters
 * - System commands
 * - Extensible via weak function for user-defined commands
 *
 * Enable by setting DS_CLI_ENABLE to 1 in ds_app_config.h.
 * When disabled, the entire module compiles to nothing.
 *
 * Configuration (defined in ds_app_config.h or ds_default_config.h):
 * - DS_CLI_PORT: Port number to listen on (default 2013)
 * - DS_CLI_TASK_NAME: Task name for debugging
 * - DS_CLI_TASK_STACK_SIZE: Stack size in words
 * - DS_CLI_TASK_PRIORITY: FreeRTOS task priority
 * - DS_CLI_MAX_INPUT_LENGTH: Maximum input line length (default 128)
 * - DS_CLI_MAX_OUTPUT_LENGTH: Maximum output line length (default 256)
 *
 * @{
 */

#ifndef DSCLI_H_
#define DSCLI_H_

#include "datastream.h"

#if (DS_CLI_ENABLE == 1)

/**
 * @brief Handle to the CLI task
 *
 * Can be used to:
 * - Monitor task state
 * - Delete task if needed
 * - Query task information
 */
extern TaskHandle_t dsCLITaskHandle;

/**
 * @brief Create and start the CLI TCP server task
 *
 * Initializes the datastream subsystem and creates a FreeRTOS task
 * that runs the CLI TCP server. The task will:
 * - Initialize datastream (calls dsInitialize())
 * - Create a TCP socket on DS_CLI_PORT
 * - Listen for incoming connections
 * - Process text commands from connected clients
 * - Automatically register with control interface as ds_control_TCP_CLI
 *
 * @note If task creation fails, enters infinite loop
 * @note CLI server handles one client at a time
 *
 * Example usage:
 * @code
 * // After network stack is initialized
 * dsCLITaskCreate();
 *
 * // Connect with: telnet <board-ip> 2013
 * // Or: nc <board-ip> 2013
 * @endcode
 */
void dsCLITaskCreate(void);

/**
 * @brief User-defined command handler (weak function)
 *
 * Override this function in your application to handle custom CLI commands.
 * Called when the input does not match any built-in command.
 *
 * @param[in]  input  The full input line (null-terminated)
 * @param[out] output Buffer to write the response into
 * @param[in]  maxLen Maximum length of the output buffer
 * @return true if the command was handled, false if not recognized
 *
 * @note This is a weak function - override in your application
 *
 * Example:
 * @code
 * bool dsCLIProcessUserCommand(const char* input, char* output, size_t maxLen)
 * {
 *     if (strncmp(input, "status", 6) == 0) {
 *         snprintf(output, maxLen, "System OK\r\n");
 *         return true;
 *     }
 *     return false;
 * }
 * @endcode
 */
bool dsCLIProcessUserCommand(const char* input, char* output, size_t maxLen);

#endif /* DS_CLI_ENABLE == 1 */

/** @} */ // end of cli_server group

#endif /* DSCLI_H_ */

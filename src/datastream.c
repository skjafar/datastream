/*
 * datastream.c
 *
 * Created: 16/06/2025 00:01:32
 *  Author: Sofian
 */

#include "datastream.h"
/* Standard includes. */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#if (DS_PLATFORM == ESP32)
#include "freertos/semphr.h"
#else
#include "semphr.h"
#endif

#include "dsRegisters.h"
#include "dsParameters.h"
#include "dsTCP.h"
#include "dsUDP.h"

#if (DS_PLATFORM == ESP32)
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_mac.h"
#endif

/*-----------------------------------------------------------*/
// Handle of the task that is currently controlling the interface (has write permission).
// This is set when the CONTROL_INTERFACE command is received.
volatile TaskHandle_t controllingTask = NULL;

// Task registration system for runtime control interface management
static ds_registered_task_t registeredTasks[DS_MAX_REGISTERED_TASKS];
static uint8_t registeredTaskCount = 0;

// Mutex for protecting control interface state (controllingTask, registeredTasks)
static SemaphoreHandle_t dsControlMutex = NULL;

// Runtime board name (persisted to flash alongside parameters)
static char dsBoardName[16];

const reply_t SYS_COMMAND_OK             = { .val = SYS_COMMAND_OK_REPLY_VAL,             .string = SYS_COMMAND_OK_REPLY_STRING };
const reply_t READ_REGISTER_OK           = { .val = READ_REGISTER_OK_REPLY_VAL,           .string = READ_REGISTER_OK_REPLY_STRING };
const reply_t WRITE_REGISTER_OK          = { .val = WRITE_REGISTER_OK_REPLY_VAL,          .string = WRITE_REGISTER_OK_REPLY_STRING };
const reply_t READ_PARAMETER_OK          = { .val = READ_PARAMETER_OK_REPLY_VAL,          .string = READ_PARAMETER_OK_REPLY_STRING };
const reply_t WRITE_PARAMETER_OK         = { .val = WRITE_PARAMETER_OK_REPLY_VAL,         .string = WRITE_PARAMETER_OK_REPLY_STRING };
const reply_t CONTROL_INTERFACE_OK       = { .val = CONTROL_INTERFACE_OK_REPLY_VAL,       .string = CONTROL_INTERFACE_OK_REPLY_STRING };
const reply_t SYS_COMMAND_ERROR          = { .val = SYS_COMMAND_ERROR_REPLY_VAL,          .string = SYS_COMMAND_ERROR_REPLY_STRING };
const reply_t INPUT_TYPE_ERROR           = { .val = INPUT_TYPE_ERROR_REPLY_VAL,           .string = INPUT_TYPE_ERROR_REPLY_STRING };
const reply_t PACKET_SIZE_ERROR          = { .val = PACKET_SIZE_ERROR_REPLY_VAL,          .string = PACKET_SIZE_ERROR_REPLY_STRING };
const reply_t ADDRESS_OUT_OF_RANGE_ERROR = { .val = ADDRESS_OUT_OF_RANGE_ERROR_REPLY_VAL, .string = ADDRESS_OUT_OF_RANGE_ERROR_REPLY_STRING };
const reply_t PERMISSION_ERROR           = { .val = PERMISSION_ERROR_REPLY_VAL,           .string = PERMISSION_ERROR_REPLY_STRING };
const reply_t CONTROL_INTERFACE_ERROR    = { .val = CONTROL_INTERFACE_ERROR_REPLY_VAL,    .string = CONTROL_INTERFACE_ERROR_REPLY_STRING };
const reply_t SYNTAX_ERROR               = { .val = SYNTAX_ERROR_REPLY_VAL,               .string = SYNTAX_ERROR_REPLY_STRING };
const reply_t PARAMETERS_ERROR           = { .val = PARAMETERS_ERROR_REPLY_VAL,           .string = PARAMETERS_ERROR_REPLY_STRING };
const reply_t CRC_ERROR                  = { .val = CRC_ERROR_REPLY_VAL,                  .string = CRC_ERROR_REPLY_STRING };

#if (CRC8_ENABLE == 1)
#include <stddef.h>

// Fast CRC-8 calculation (polynomial 0x07, initial value 0x00)
static const uint8_t crc8_table[256] = {
    0x00,0x07,0x0E,0x09,0x1C,0x1B,0x12,0x15,0x38,0x3F,0x36,0x31,0x24,0x23,0x2A,0x2D,
    0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,0x48,0x4F,0x46,0x41,0x54,0x53,0x5A,0x5D,
    0xE0,0xE7,0xEE,0xE9,0xFC,0xFB,0xF2,0xF5,0xD8,0xDF,0xD6,0xD1,0xC4,0xC3,0xCA,0xCD,
    0x90,0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,0xA8,0xAF,0xA6,0xA1,0xB4,0xB3,0xBA,0xBD,
    0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,0xFF,0xF8,0xF1,0xF6,0xE3,0xE4,0xED,0xEA,
    0xB7,0xB0,0xB9,0xBE,0xAB,0xAC,0xA5,0xA2,0x8F,0x88,0x81,0x86,0x93,0x94,0x9D,0x9A,
    0x27,0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,0x1F,0x18,0x11,0x16,0x03,0x04,0x0D,0x0A,
    0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,0x6F,0x68,0x61,0x66,0x73,0x74,0x7D,0x7A,
    0x89,0x8E,0x87,0x80,0x95,0x92,0x9B,0x9C,0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,
    0xF9,0xFE,0xF7,0xF0,0xE5,0xE2,0xEB,0xEC,0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,0xD3,0xD4,
    0x69,0x6E,0x67,0x60,0x75,0x72,0x7B,0x7C,0x51,0x56,0x5F,0x58,0x4D,0x4A,0x43,0x44,
    0x19,0x1E,0x17,0x10,0x05,0x02,0x0B,0x0C,0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,
    0x4E,0x49,0x40,0x47,0x52,0x55,0x5C,0x5B,0x76,0x71,0x78,0x7F,0x6A,0x6D,0x64,0x63,
    0x3E,0x39,0x30,0x37,0x22,0x25,0x2C,0x2B,0x06,0x01,0x08,0x0F,0x1A,0x1D,0x14,0x13,
    0xAE,0xA9,0xA0,0xA7,0xB2,0xB5,0xBC,0xBB,0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,
    0xDE,0xD9,0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,0xF4,0xF3
};

/**
 * @brief Calculate CRC-8 (polynomial 0x07, initial value 0x00) efficiently.
 * @param data Pointer to the data buffer
 * @param len  Length of the data buffer
 * @return CRC-8 value
 */
static uint8_t dsCalcCRC8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; ++i)
    {
        crc = crc8_table[crc ^ data[i]];
    }
    return crc;
}

/**
 * @brief Check the CRC-8 of the received packet.
 * @param rxPacket Pointer to the received packet structure
 * @return true if CRC check passes, false if it fails
 */
static bool dsCheckCRC8(const dsRxPacket *rxPacket)
{
    // Check if the received CRC matches the calculated CRC
    uint8_t received_crc = rxPacket->buffer[DATASTREAM_INPUT_SIZE - 1];
    uint8_t calc_crc = dsCalcCRC8(rxPacket->buffer, DATASTREAM_INPUT_SIZE - 1);
    
    if (received_crc != calc_crc)
    {
        // CRC error: handle error, drop packet, or send error response
        DS_LOGE("CRC error: received %02X, calculated %02X", received_crc, calc_crc);
        return false; // CRC check failed
    }
    
    return true; // CRC check passed
}
#endif // CRC8_ENABLE == 1

/**
 * @brief Initializes the datastream module.
 *
 * This function sets up any required resources, configurations, or state
 * necessary for the datastream component to operate. It should be called
 * before using any other datastream-related functions.
 */
void dsInitialize(void)
{
    static bool initialized = false;
    if (initialized)
    {
        return; // Already initialized
    }
    initialized = true;
    
    dsInitializeParameters(&PARS);
    dsInitializeSystemRegisters(&SYS_REGS);
    dsInitializeRegisters(&REGS);
    // Initialize the task registration system
    dsClearRegisteredTasks();

    // Create the control interface mutex
    dsControlMutex = xSemaphoreCreateMutex();

    // Initialize board name to compile-time default, then try to load from flash
    strncpy(dsBoardName, DS_BOARD_NAME, sizeof(dsBoardName) - 1);
    dsBoardName[sizeof(dsBoardName) - 1] = '\0';
    dsReadBoardNameFromFlash(dsBoardName, sizeof(dsBoardName));

    DS_LOGI("Datastream initialization complete with task registration system");
}

void dsProcessPacket(dsRxPacket *inPacket, dsTxPacket *outPacket)
{
    reply_t reply = {0};

    outPacket->contents.address = inPacket->contents.address; // Echo back the address as a sanity check

    // Check if the received packet has a valid CRC
    #if (CRC8_ENABLE == 1)
    if (!dsCheckCRC8(inPacket))
    {
        // CRC check failed
        DS_LOGE("CRC check failed");
        outPacket->contents.status = CRC_ERROR.val;
        return; // Exit the function if CRC check fails
    }
    #endif
    
    // Increment the packet count system register
    #if(DS_STATS_ENABLE == 1)
        SYS_REGS.byName.DS_PACKET_COUNT++;
    #endif

    switch (inPacket->contents.type)
    {
        case ds_type_SYS_COMMAND:
            dsProcessSysCommand(inPacket, outPacket);
            break;
        case ds_type_READ_REGISTER:
            outPacket->contents.value = dsGetRegister(inPacket->contents.address, &reply);
            outPacket->contents.status = reply.val;
            break;
        case ds_type_WRITE_REGISTER:
            dsSetRegister(inPacket->contents.address, inPacket->contents.value, &reply);
            outPacket->contents.status = reply.val;
            outPacket->contents.value = (reply.val == WRITE_REGISTER_OK_REPLY_VAL)
                ? inPacket->contents.value : 0;
            break;
        case ds_type_READ_PARAMETER:
            outPacket->contents.value = dsGetParameter(inPacket->contents.address, &reply);
            outPacket->contents.status = reply.val;
            break;
        case ds_type_WRITE_PARAMETER:
            dsSetParameter(inPacket->contents.address, inPacket->contents.value, &reply);
            outPacket->contents.status = reply.val;
            outPacket->contents.value = (reply.val == WRITE_PARAMETER_OK_REPLY_VAL)
                ? inPacket->contents.value : 0;
            break;
        // This is the control interface command, it is used to set the controlling task
        case ds_type_CONTROL_INTERFACE:
            if (dsProcessControlInterfaceCommand()) {
                outPacket->contents.status = CONTROL_INTERFACE_OK.val;
            } else {
                outPacket->contents.status = CONTROL_INTERFACE_ERROR.val;
            }
            break;
        case ds_type_READ_SYSTEM_REGISTER:
            outPacket->contents.value = dsGetSystemRegister(inPacket->contents.address, &reply);
            outPacket->contents.status = reply.val;
            break;
        case ds_type_WRITE_SYSTEM_REGISTER:
            dsSetSystemRegister(inPacket->contents.address, inPacket->contents.value, &reply);
            outPacket->contents.status = reply.val;
            outPacket->contents.value = (reply.val == WRITE_REGISTER_OK_REPLY_VAL)
                ? inPacket->contents.value : 0;
            break;
        default:
            // If the user-defined type was not processed, we assume it's an unknown type error
            // This is a fallback for unrecognized input types
            outPacket->contents.status = INPUT_TYPE_ERROR.val;
            
            // maybe user defined input type
            dsProcessUserDefinedType(inPacket, outPacket, &reply);
            break;
    }
    #if (CRC8_ENABLE == 1)
    outPacket->contents.crc = dsCalcCRC8(outPacket->buffer, DATASTREAM_OUTPUT_SIZE - 1); // Calculate CRC for the output packet
    #endif

    #if(DS_STATS_ENABLE == 1)
        if (outPacket->contents.status < 0)
        {
            dsErrorCallback(reply, inPacket, outPacket);
        }
    #endif
}

bool dsProcessSysCommand(const dsRxPacket *inputPacket, dsTxPacket *outputPacket)
{
    ds_sys_command_t command = (ds_sys_command_t)inputPacket->contents.address;
    uint32_t value = inputPacket->contents.value;
    (void)value; // Suppress unused variable warning
    
    if (dsCheckTaskWritePermission())
    {
        switch (command)
        {
            case ds_sys_command_WRITE_FLASH:
                // write parameters and board name to flash
                dsWriteParametersToFlash(&PARS);
                dsWriteBoardNameToFlash(dsBoardName);
                outputPacket->contents.status = SYS_COMMAND_OK.val;
                outputPacket->contents.value = 0;
                vTaskDelay(portTICK_PERIOD_MS * 2);
                return true;
                break;
            case ds_sys_command_READ_FLASH:
                // read parameters and board name from flash
                dsReadParametersFromFlash(&PARS);
                dsReadBoardNameFromFlash(dsBoardName, sizeof(dsBoardName));
                outputPacket->contents.status = SYS_COMMAND_OK.val;
                outputPacket->contents.value = 0;
                vTaskDelay(portTICK_PERIOD_MS * 2);
                return true;                    
                break;
            case ds_sys_command_RESET_FIRMWARE:
                // reset the firmware
                outputPacket->contents.status = SYS_COMMAND_OK.val;
                outputPacket->contents.value = 0;
                #if (DS_PLATFORM == ESP32)
                esp_restart();
                #elif (DS_PLATFORM == STM32)
                HAL_NVIC_SystemReset();
                #endif
                return true;
                break;
            default:
                // Try user-defined system command handler
                if (dsProcessUserSysCommand != NULL && dsProcessUserSysCommand(inputPacket, outputPacket))
                {
                    return true;
                }
                
                outputPacket->contents.status = SYS_COMMAND_ERROR.val;
                outputPacket->contents.value = 0;
                return false;
                break;
        }
    }
    else
    {
        outputPacket->contents.status = CONTROL_INTERFACE_ERROR.val;
        outputPacket->contents.value = 0;
        return false;
    }
}

WEAK bool dsProcessUserSysCommand(const dsRxPacket *inputPacket, dsTxPacket *outputPacket)
{
    // Default implementation - command not supported
    outputPacket->contents.status = SYS_COMMAND_ERROR.val;
    outputPacket->contents.value = 0;
    return false;
}

void dsPacketSizeError(uint16_t inPacketSize, dsTxPacket *outPacket)
{
    // This function is called when the received packet size is not equal to DATASTREAM_INPUT_SIZE
    // It will return an error message in the output packet
    outPacket->contents.status = PACKET_SIZE_ERROR.val;
    outPacket->contents.value = 0; // no value for error
    #if (CRC8_ENABLE == 1)
    outPacket->contents.crc = dsCalcCRC8(outPacket->buffer, DATASTREAM_OUTPUT_SIZE - 1); // Calculate CRC for the output packet
    #endif
    // Optionally, you can log the error or take other actions here
    DS_LOGE("Received packet size error: expected %" PRIu32 ", got %" PRIu32, (uint32_t)DATASTREAM_INPUT_SIZE, inPacketSize);
    
    #if(DS_STATS_ENABLE == 1)
        if (outPacket->contents.status < 0)
        {
            dsErrorCallback(PACKET_SIZE_ERROR, NULL, outPacket);
        }
    #endif
}

/**
 * @brief Processes the CONTROL_INTERFACE command and sets the controlling task.
 *
 * This function is called when the CONTROL_INTERFACE command is received.
 * It determines which task is currently running and sets the control interface
 * register accordingly. If the current task is recognized as a valid controller,
 * the CONTROL_INTERFACE register is updated to reflect the controlling source.
 * Otherwise, the register is set to an undecided state.
 *
 * @return true if the control interface was successfully set for a recognized task,
 *         false otherwise.
 */
bool dsProcessControlInterfaceCommand(void)
{
    bool result = false;

    if (dsControlMutex != NULL)
    {
        xSemaphoreTake(dsControlMutex, portMAX_DELAY);
    }

    controllingTask = xTaskGetCurrentTaskHandle();

    // Check dynamically registered tasks (includes built-in datastream tasks)
    ds_control_interface_t controlType = dsGetTaskControlType(controllingTask);
    if (controlType != ds_control_UNDECIDED)
    {
        SYS_REGS.byName.CONTROL_INTERFACE = controlType;
        DS_LOGI("Control interface set to %d for registered task 0x%08" PRIx32, controlType, (uint32_t)controllingTask);
        result = true;
    }
    else
    {
        // If no registered task found, set to undecided
        SYS_REGS.byName.CONTROL_INTERFACE = ds_control_UNDECIDED;
        DS_LOGE("Control interface request from unregistered task 0x%08" PRIx32, (uint32_t)controllingTask);
    }

    if (dsControlMutex != NULL)
    {
        xSemaphoreGive(dsControlMutex);
    }

    return result;
}

WEAK bool dsProcessUserDefinedType(dsRxPacket *inPacket, dsTxPacket *outPacket, reply_t *reply)
{
    // This function is weak and can be overridden by the application
    // If not overridden, it will return an error in the output packet which is the default behavior

    *reply = INPUT_TYPE_ERROR;
    outPacket->contents.status = INPUT_TYPE_ERROR.val;
    outPacket->contents.value = 0; // no value for error
    return false;
}

/**
 * @brief Checks if the current task has permission to write to registers or parameters.
 *
 * This function is declared as weak and can be overridden by the application.
 * It compares the handle of the currently running task with the handle
 * of the controlling task. If they match, the current task is granted write permission.
 * This mechanism is used to ensure that only the designated controlling task can modify
 * critical registers or parameters, preventing race conditions and unauthorized access.
 *
 * @note This function is typically called in functions such as dsSetRegister and dsSetParameter
 *       to enforce write access control.
 *
 * @return true if the current task has write permission, false otherwise.
 */
WEAK bool dsCheckTaskWritePermission(void)
{
    bool hasPermission = false;

    if (dsControlMutex != NULL)
    {
        xSemaphoreTake(dsControlMutex, portMAX_DELAY);
    }
    
    // This can be extended to check for specific tasks if needed
    // For example, if you want to allow only dsTCPTaskHandle to write:
    // return (controllingTask == dsTCPTaskHandle);
    // If you want to allow multiple tasks, you can use a list or an enum to check against
    // return (controllingTask == dsTCPTaskHandle || controllingTask == USBTask);
    // If you want to allow all tasks, just return true
    // return true;
    hasPermission = (controllingTask == xTaskGetCurrentTaskHandle());

    if (dsControlMutex != NULL)
    {
        xSemaphoreGive(dsControlMutex);
    }

    return hasPermission;
}

WEAK void dsErrorCallback(reply_t reply, dsRxPacket *inPacket, dsTxPacket *outPacket)
{
    // This function is weak and can be overridden by the application
    
    SYS_REGS.byName.DS_ERROR_COUNT++; // Increment the error count system register
}

/* ======================== Board Name API ======================== */

const char* dsGetBoardName(void)
{
    return dsBoardName;
}

void dsSetBoardName(const char* name)
{
    if (name != NULL)
    {
        strncpy(dsBoardName, name, sizeof(dsBoardName) - 1);
        dsBoardName[sizeof(dsBoardName) - 1] = '\0';
    }
}

WEAK void dsWriteBoardNameToFlash(const char* name)
{
    (void)name;
    // Default implementation does nothing - override in your application
}

WEAK void dsReadBoardNameFromFlash(char* name, size_t maxLen)
{
    (void)name;
    (void)maxLen;
    // Default implementation does nothing - override in your application
}

#if (DS_AUTO_DETECTION_ENABLE == 1)

/**
 * @brief Initialize auto-detection subsystem
 *
 * This function should be called during system initialization
 * to set up the auto-detection functionality.
 */
void dsAutoDetectionInit(void)
{
    DS_LOGI("Board auto-detection initialized");
    DS_LOGI("Board: %s, Type: %d, Serial: 0x%08" PRIx32, DS_BOARD_NAME, DS_BOARD_TYPE, (uint32_t)DS_BOARD_ID);
}

bool dsProcessDiscoveryPacket(const uint8_t *rxBuffer, size_t rxSize, uint8_t *txBuffer, size_t *txSize, uint32_t clientIP)
{
    // Validate input parameters
    if (!rxBuffer || !txBuffer || !txSize || rxSize < sizeof(ds_discovery_request_t))
    {
        return false;
    }
    
    // Cast received data to discovery request structure
    const ds_discovery_request_t *request = (const ds_discovery_request_t *)rxBuffer;
    
    // Validate magic number and command
    if (request->magic != DS_DISCOVERY_MAGIC || request->command != DS_DISCOVERY_REQUEST)
    {
        return false;
    }
    
    uint8_t *ip = (uint8_t *)&clientIP;
    DS_LOGI("Discovery request received from client IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    
    // Prepare discovery response
    ds_discovery_response_t *response = (ds_discovery_response_t *)txBuffer;
    memset(response, 0, sizeof(ds_discovery_response_t));
    
    // Fill response data
    response->magic = DS_DISCOVERY_MAGIC;
    response->command = DS_DISCOVERY_RESPONSE;
    response->board_type = DS_BOARD_TYPE;
    response->firmware_version = DS_FIRMWARE_VERSION;
    response->board_id = DS_BOARD_ID;
    
    // Get current board IP address
    #if (DS_PLATFORM == ESP32)
        // For ESP32, get IP from default network interface (WiFi/Ethernet)
        esp_netif_t *netif = esp_netif_get_default_netif();
        if (netif != NULL)
        {
            esp_netif_ip_info_t ip_info;
            if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
            {
                response->ip_address = ip_info.ip.addr;
            }
            else
            {
                response->ip_address = 0;
            }
        }
        else
        {
            response->ip_address = 0;
        }
    #else
        // For FreeRTOS+TCP, get IP from network interface
        response->ip_address = FreeRTOS_GetIPAddress();
    #endif
    
    response->tcp_port = DS_TCP_PORT;
    response->udp_port = DS_UDP_PORT;
    
    // Get MAC address
    if (!dsGetMACAddress(response->mac_address)) {
        // If MAC retrieval fails, set to all zeros
        memset(response->mac_address, 0, 6);
    }
    
    // Clear reserved bytes
    memset(response->reserved, 0, sizeof(response->reserved));
    
    // Copy board name (ensure null termination)
    strncpy(response->board_name, dsGetBoardName(), sizeof(response->board_name) - 1);
    response->board_name[sizeof(response->board_name) - 1] = '\0';
    
    *txSize = sizeof(ds_discovery_response_t);
    
    DS_LOGI("Discovery response sent: Board=%s, Type=%d, ID=0x%08" PRIx32, 
            response->board_name, response->board_type, response->board_id);
    
    return true;
}

WEAK bool dsGetMACAddress(uint8_t mac_address[6])
{
    // This is a weak function - can be overridden by the application
    // Default implementation based on platform

    #if (DS_PLATFORM == ESP32)
        // For ESP32, get MAC from default network interface
        esp_netif_t *netif = esp_netif_get_default_netif();
        if (netif != NULL)
        {
            esp_err_t ret = esp_netif_get_mac(netif, mac_address);
            if (ret == ESP_OK)
            {
                return true;
            }
        }
        // Fallback: Try to get base MAC address
        esp_err_t ret = esp_read_mac(mac_address, ESP_MAC_WIFI_STA);
        return (ret == ESP_OK);

    #elif (DS_PLATFORM == STM32) || (DS_PLATFORM == RP2040) || (DS_PLATFORM == RP2350)
        // For FreeRTOS+TCP platforms
        #include "FreeRTOS_IP.h"

        NetworkEndPoint_t * pxEndPoint = FreeRTOS_FirstEndPoint( NULL );

        if( pxEndPoint != NULL )
        {
            /* Copy the MAC address at the start of the default packet header fragment. */
            memcpy(mac_address, pxEndPoint->xMACAddress.ucBytes, 6);
            return true;
        }

        
    #endif

    // if arrived here MAC address reading failed
    return false;
}

#endif // DS_AUTO_DETECTION_ENABLE

bool dsRegisterControlTask(TaskHandle_t taskHandle, ds_control_interface_t controlType, const char* taskName)
{
    if (taskHandle == NULL)
    {
        DS_LOGE("Cannot register task: invalid task handle");
        return false;
    }

    if (dsControlMutex != NULL)
    {
        xSemaphoreTake(dsControlMutex, portMAX_DELAY);
    }

    if (registeredTaskCount >= DS_MAX_REGISTERED_TASKS)
    {
        DS_LOGE("Cannot register task: maximum number of registered tasks (%d) reached", DS_MAX_REGISTERED_TASKS);
        if (dsControlMutex != NULL) xSemaphoreGive(dsControlMutex);
        return false;
    }

    // Check if task is already registered
    for (uint8_t i = 0; i < registeredTaskCount; i++)
    {
        if (registeredTasks[i].taskHandle == taskHandle)
        {
            DS_LOGI("Task already registered, updating control type");
            registeredTasks[i].controlType = controlType;
            if (taskName != NULL)
            {
                strncpy(registeredTasks[i].taskName, taskName, sizeof(registeredTasks[i].taskName) - 1);
                registeredTasks[i].taskName[sizeof(registeredTasks[i].taskName) - 1] = '\0'; // Ensure null termination
            }
            else
            {
                registeredTasks[i].taskName[0] = '\0'; // Empty string if taskName is NULL
            }
            if (dsControlMutex != NULL) xSemaphoreGive(dsControlMutex);
            return true;
        }
    }

    // Add new registration
    registeredTasks[registeredTaskCount].taskHandle = taskHandle;
    registeredTasks[registeredTaskCount].controlType = controlType;
    if (taskName != NULL)
    {
        strncpy(registeredTasks[registeredTaskCount].taskName, taskName, sizeof(registeredTasks[registeredTaskCount].taskName) - 1);
        registeredTasks[registeredTaskCount].taskName[sizeof(registeredTasks[registeredTaskCount].taskName) - 1] = '\0'; // Ensure null termination
    }
    else
    {
        registeredTasks[registeredTaskCount].taskName[0] = '\0'; // Empty string if taskName is NULL
    }
    registeredTaskCount++;

    if (dsControlMutex != NULL)
    {
        xSemaphoreGive(dsControlMutex);
    }

    DS_LOGI("Registered task handle 0x%08" PRIx32 " for control type %d", (uint32_t)taskHandle, controlType);
    return true;
}

bool dsUnregisterControlTask(TaskHandle_t taskHandle)
{
    bool found = false;

    if (dsControlMutex != NULL)
    {
        xSemaphoreTake(dsControlMutex, portMAX_DELAY);
    }

    for (uint8_t i = 0; i < registeredTaskCount; i++)
    {
        if (registeredTasks[i].taskHandle == taskHandle)
        {
            // Shift remaining entries down
            for (uint8_t j = i; j < registeredTaskCount - 1; j++)
            {
                registeredTasks[j] = registeredTasks[j + 1];
            }
            registeredTaskCount--;
            found = true;
            break;
        }
    }

    if (dsControlMutex != NULL)
    {
        xSemaphoreGive(dsControlMutex);
    }

    if (found)
    {
        DS_LOGI("Unregistered task handle 0x%08" PRIx32, (uint32_t)taskHandle);
    }
    else
    {
        DS_LOGE("Cannot unregister task: task handle 0x%08" PRIx32 " not found", (uint32_t)taskHandle);
    }

    return found;
}

/**
 * @brief Clear all registered tasks.
 * 
 * This function removes all registered tasks from the control interface system.
 * Useful for reinitialization or cleanup.
 */
void dsClearRegisteredTasks(void)
{
    if (dsControlMutex != NULL)
    {
        xSemaphoreTake(dsControlMutex, portMAX_DELAY);
    }

    registeredTaskCount = 0;

    if (dsControlMutex != NULL)
    {
        xSemaphoreGive(dsControlMutex);
    }

    DS_LOGI("Cleared all registered tasks");
}

ds_control_interface_t dsGetTaskControlType(TaskHandle_t taskHandle)
{
    ds_control_interface_t controlType = ds_control_UNDECIDED;

    // Note: When called from dsProcessControlInterfaceCommand(), the mutex
    // is already held. FreeRTOS mutexes are recursive-safe if created with
    // xSemaphoreCreateRecursiveMutex. We use a regular mutex here so callers
    // from within a locked context should call this before taking the mutex,
    // which dsProcessControlInterfaceCommand() already does correctly.
    for (uint8_t i = 0; i < registeredTaskCount; i++)
    {
        if (registeredTasks[i].taskHandle == taskHandle)
        {
            controlType = registeredTasks[i].controlType;
            break;
        }
    }

    return controlType;
}

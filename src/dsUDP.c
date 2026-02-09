/*
 * dsUDP.c
 *
 * Created: 05/04/2025
 *  Author: Sofian
 */ 

#include "dsUDP.h"

/* Standard includes. */
#include <stdint.h>
#include <string.h>

#if (DS_PLATFORM == ESP32) // ESP32 network specific includes
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <errno.h>
#else // Native FreeRTOS+TCP includes */
    /* FreeRTOS+TCP includes. */
    #include "FreeRTOS_IP.h"
    #include "FreeRTOS_Sockets.h"
#endif

#include "dsRegisters.h"
#include "dsParameters.h"

TaskHandle_t dsUDPTaskHandle = NULL;

/*
 * Static function prototypes
 */
static void prvdsUDPTask(void *pvParameters);
static int32_t dsUDPRecvFrom( dsSocket_t xSocket, void *pvBuffer, size_t uxBufferLength, BaseType_t xFlags, dsSockaddr_t *pxSourceAddress, socklen_t *puxSourceAddressLength);
static int32_t dsUDPSendTo( dsSocket_t xSocket, const void * pvBuffer, size_t uxDataLength, BaseType_t xFlags, const dsSockaddr_t *pxDestinationAddress, socklen_t uxDestinationAddressLength );

/**
 * @brief Creates and starts the UDP datastream FreeRTOS task.
 *
 * This function initializes the datastream subsystem and then creates
 * a FreeRTOS task responsible for handling UDP datastream operations.
 * If the task creation fails, the function enters an infinite loop.
 *
 * The task is created with the following parameters:
 * - Task function: prvdsUDPTask
 * - Task name: DS_UDP_TASK_NAME
 * - Stack size: DS_UDP_TASK_STACK_SIZE
 * - Task parameter: DS_UDP_PORT (cast to void *)
 * - Priority: DS_UDP_TASK_PRIORITY
 * - Task handle: dsUDPTaskHandle
 */
void dsUDPTaskCreate(void)
{
    // initialize the datastream
    dsInitialize();

    if (xTaskCreate(
            prvdsUDPTask, DS_UDP_TASK_NAME, DS_UDP_TASK_STACK_SIZE, (void *) DS_UDP_PORT, DS_UDP_TASK_PRIORITY, &dsUDPTaskHandle)
        != pdPASS) {
        while (1) {
            ;
        }
    }
    
    /* Register the UDP datastream task with control interface system */
    dsRegisterControlTask(dsUDPTaskHandle, ds_control_UDP_DATASTREAM, DS_UDP_TASK_NAME);
}

/**
 * @brief Opens a UDP server socket on the specified port.
 * This function creates a UDP socket, sets the necessary options, and binds it to the specified port.
 *
 * @param port The port number to bind the UDP socket to.
 * @return The socket file descriptor on success, or -1 on failure.
 */
#if (DS_PLATFORM == ESP32)
static dsSocket_t dsOpenUDPServerSocket(uint16_t port)
{
    dsSocket_t sock;
    dsSockaddr_t server_addr;
    int opt = 1;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0)
    {
        DS_LOGE("Unable to create UDP socket: errno %d", errno);
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        DS_LOGE("UDP socket unable to bind: errno %d", errno);
        close(sock);
        return -1;
    }

    return sock;
}
#else
static dsSocket_t dsOpenUDPServerSocket(uint16_t port)
{
    dsSockaddr_t xServer;
    dsSocket_t xSocket = FREERTOS_INVALID_SOCKET;

    xSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
    if( xSocket != FREERTOS_INVALID_SOCKET)
    {
        /* Zero out the server structure. */
        memset( ( void * ) &xServer, 0x00, sizeof( xServer ) );

        /* Set family and port. */
        xServer.sin_port = FreeRTOS_htons( port );

        /* Bind the address to the socket. */
        if( FreeRTOS_bind( xSocket, &xServer, sizeof( xServer ) ) < 0 )
        {
            FreeRTOS_closesocket( xSocket );
            xSocket = FREERTOS_INVALID_SOCKET;
        }
    }

    return xSocket;
}
#endif

/**
 * @brief UDP server task handler for datastream communication.
 *
 * This function implements the main loop for a UDP server that handles datastream packets.
 * It performs the following steps:
 *   - Initializes datastream structures.
 *   - Opens a UDP server socket on the port specified by pvParameters.
 *   - Waits for incoming UDP datagrams.
 *   - For each received datagram:
 *       - Receives packets of size DATASTREAM_INPUT_SIZE.
 *       - If the received packet size is correct, processes the packet and sends a response.
 *       - If the packet size is incorrect, sends an error response.
 *       - Handles socket errors gracefully.
 *   - Cleans up sockets and resources on error.
 *
 * @param pvParameters Pointer to parameters passed to the task (expected to contain the port number).
 *
 * @note This function is intended to run as a FreeRTOS task and will delete itself on fatal errors.
 * @note Uses global logging macros (DS_LOGI, DS_LOGE) and relies on external datastream processing functions.
 */
void prvdsUDPTask(void *pvParameters)
{
    int32_t lBytes, lSent;
    
    // Create a socket for listening to incoming connections according to platform
    dsSockaddr_t client;
    dsSocket_t listeningSocket;

    socklen_t xSize = sizeof(client);

    dsTxPacket txPacket;
    dsRxPacket rxPacket;

    dsInitialize();
    
#if (DS_AUTO_DETECTION_ENABLE == 1)    
    uint32_t clientIP = 0;
    uint8_t discoveryBuffer[48]; // Buffer for discovery packets
    size_t discoveryTxSize;
    dsAutoDetectionInit();
#endif
    
    memset(rxPacket.buffer, 0x00, DATASTREAM_INPUT_SIZE);
    memset(txPacket.buffer, 0x00, DATASTREAM_OUTPUT_SIZE);
    lSent = 0;

    
    /* Attempt to open the UDP socket. The port number is passed in the task
    parameter. The strange casting is to remove compiler warnings on 32-bit
    machines. */
    listeningSocket = dsOpenUDPServerSocket((uint16_t)((uint32_t)pvParameters) & 0xffffUL);
    #if (DS_PLATFORM == ESP32)
    if (listeningSocket < 0)
    #else
    if (listeningSocket == FREERTOS_INVALID_SOCKET)
    #endif
    {   
        DS_LOGE("Unable to create a UDP socket: errno %d", errno);
        // Delete the task if socket creation fails
        vTaskDelete(NULL);
    }

#if (DS_AUTO_DETECTION_ENABLE == 1)
    DS_LOGI("Auto-detection enabled on port %d", (int)((uint16_t)((uint32_t)pvParameters) & 0xffff));
#endif

    for (;;)
    {
        DS_LOGI("Waiting for incoming UDP datagrams...");

        while (1)
        {
            lBytes = dsUDPRecvFrom(listeningSocket, rxPacket.buffer, DATASTREAM_INPUT_SIZE, 0, &client, &xSize);

            if (lBytes == DATASTREAM_INPUT_SIZE)
            {
                // This is a regular datastream packet
                dsProcessPacket(&rxPacket, &txPacket);
                
                lSent = dsUDPSendTo(listeningSocket, txPacket.buffer, DATASTREAM_OUTPUT_SIZE, 0, &client, xSize);

            }
#if (DS_AUTO_DETECTION_ENABLE == 1)
            else if (lBytes == sizeof(ds_discovery_request_t))
            {
                // First check if this is a discovery packet
                #if (DS_PLATFORM == ESP32)
                clientIP = client.sin_addr.s_addr;
                #else
                clientIP = client.sin_address.ulIP_IPv4;
                #endif
                
                if (dsProcessDiscoveryPacket(rxPacket.buffer, lBytes, discoveryBuffer, &discoveryTxSize, clientIP))
                {
                    // This was a discovery packet, send discovery response
                    #if (DS_PLATFORM == ESP32)
                    uint8_t *ip = (uint8_t *)&client.sin_addr.s_addr;
                    uint16_t port = ntohs(client.sin_port);
                    DS_LOGI("Sending discovery response: %d bytes to %d.%d.%d.%d:%d",
                            (int)discoveryTxSize, ip[0], ip[1], ip[2], ip[3], port);

                    // Hex dump first 44 bytes of response for debugging
                    DS_LOGI("Response hex: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                            discoveryBuffer[0], discoveryBuffer[1], discoveryBuffer[2], discoveryBuffer[3],
                            discoveryBuffer[4], discoveryBuffer[5], discoveryBuffer[6], discoveryBuffer[7],
                            discoveryBuffer[8], discoveryBuffer[9], discoveryBuffer[10], discoveryBuffer[11]);
                    #else
                    DS_LOGI("Sending discovery response: %d bytes to client", (int)discoveryTxSize);
                    #endif

                    int32_t sent = dsUDPSendTo(listeningSocket, discoveryBuffer, discoveryTxSize, 0, &client, xSize);
                    if (sent > 0) {
                        DS_LOGI("Discovery response sent successfully: %d bytes", (int)sent);
                    } else {
                        DS_LOGE("Failed to send discovery response: errno %d", errno);
                    }
                }
            }
#endif
            else if (lBytes > 0)
            {
                // Invalid packet size for datastream, send error response
                dsPacketSizeError(lBytes, &txPacket);
                lSent = dsUDPSendTo(listeningSocket, txPacket.buffer, DATASTREAM_OUTPUT_SIZE, 0, &client, xSize);
            }
            else if (lBytes < 0) 
            {
                // Socket closed or error
                DS_LOGE("UDP socket error: errno %d", errno);
                break;
            }
            
            // Check if the send was successful
            if (lSent < DATASTREAM_OUTPUT_SIZE)
            {
                // Error occurred during sending, exit the loop to close the connection
                break;
            }
        }
    }
}

/**
 * @brief Receives data from a UDP socket.
 *
 * This function receives data from a UDP socket using the platform-appropriate
 * recvfrom function. It handles both ESP32 standard sockets and FreeRTOS sockets.
 *
 * @param xSocket The socket from which data will be received.
 * @param pvBuffer Pointer to the buffer where received data will be stored.
 * @param uxBufferLength The length of the buffer in bytes.
 * @param xFlags Flags for the receive operation (currently unused).
 * @param pxSourceAddress Pointer to store the source address of the received packet.
 * @param puxSourceAddressLength Pointer to the size of the source address structure.
 * @return The number of bytes received, or -1 on error.
 */
static int32_t dsUDPRecvFrom( dsSocket_t xSocket, void *pvBuffer, size_t uxBufferLength, BaseType_t xFlags, dsSockaddr_t *pxSourceAddress, socklen_t *puxSourceAddressLength)
{
    #if (DS_PLATFORM == ESP32)
    return (int32_t)recvfrom(xSocket, pvBuffer, uxBufferLength, 0, (struct sockaddr *)pxSourceAddress, puxSourceAddressLength);
    #else
    return (int32_t)FreeRTOS_recvfrom(xSocket, pvBuffer, uxBufferLength, 0, pxSourceAddress, puxSourceAddressLength);
    #endif
}

/**
 * @brief Sends data over a UDP socket.
 *
 * This function sends data over a UDP socket using the platform-appropriate
 * sendto function. It handles both ESP32 standard sockets and FreeRTOS sockets.
 *
 * @param xSocket The socket to which data will be sent.
 * @param pvBuffer Pointer to the data buffer to be sent.
 * @param uxDataLength The length of the data buffer in bytes.
 * @param xFlags Flags for the send operation (currently unused).
 * @param pxDestinationAddress Pointer to the destination address.
 * @param uxDestinationAddressLength The size of the destination address structure.
 * @return The number of bytes sent, or -1 on error.
 */
static int32_t dsUDPSendTo( dsSocket_t xSocket, const void * pvBuffer, size_t uxDataLength, BaseType_t xFlags, const dsSockaddr_t *pxDestinationAddress, socklen_t uxDestinationAddressLength )
{
    #if (DS_PLATFORM == ESP32)
    return (int32_t)sendto(xSocket, pvBuffer, uxDataLength, 0, (const struct sockaddr *)pxDestinationAddress, uxDestinationAddressLength);
    #else
    return (int32_t)FreeRTOS_sendto(xSocket, pvBuffer, uxDataLength, 0, pxDestinationAddress, uxDestinationAddressLength);
    #endif
}

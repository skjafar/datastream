/*
 * dsTCP.c
 *
 * Created: 05/04/2025 
 *  Author: Sofian
 */ 

#include "dsTCP.h"
#include "datastream.h"

/* Standard includes. */
#include <stdint.h>
#include <string.h>

#include "dsRegisters.h"
#include "dsParameters.h"

TaskHandle_t dsTCPTaskHandle = NULL;

/*
 * Static function prototypes
 */
static void prvdsTCPTask(void *pvParameters);
static int32_t dsTCPRecv( dsSocket_t xSocket, void *pvBuffer, size_t uxBufferLength, BaseType_t xFlags);
static int32_t dsTCPSend( dsSocket_t xSocket, const void * pvBuffer, size_t uxDataLength, BaseType_t xFlags );

/**
 * @brief Creates and starts the TCP datastream FreeRTOS task.
 *
 * This function initializes the datastream subsystem and then creates
 * a FreeRTOS task responsible for handling TCP datastream operations.
 * If the task creation fails, the function enters an infinite loop.
 *
 * The task is created with the following parameters:
 * - Task function: prvdsTCPTask
 * - Task name: DS_TCP_TASK_NAME
 * - Stack size: DS_TCP_TASK_STACK_SIZE
 * - Task parameter: DS_TCP_PORT (cast to void *)
 * - Priority: DS_TCP_TASK_PRIORITY
 * - Task handle: dsTCPTaskHandle
 */
void dsTCPTaskCreate(void)
{
    // initialize the datastream
    dsInitialize();

    if (xTaskCreate(
            prvdsTCPTask, DS_TCP_TASK_NAME, DS_TCP_TASK_STACK_SIZE, (void *)DS_TCP_PORT, DS_TCP_TASK_PRIORITY, &dsTCPTaskHandle)
        != pdPASS) {
        while (1) {;}
    }
    
    /* Register the TCP datastream task with control interface system */
    dsRegisterControlTask(dsTCPTaskHandle, ds_control_TCP_DATASTREAM, DS_TCP_TASK_NAME);
}

/**
 * @brief Gracefully shuts down and closes a TCP socket.
 *
 * This function performs a graceful shutdown of the specified socket by disabling
 * both send and receive operations using shutdown(), and then closes the socket
 * descriptor to release associated resources.
 *
 * @param sock The file descriptor of the socket to be shut down and closed.
 */
#if (DS_PLATFORM == ESP32)
static void dsGracefulShutdown(int sock)
{
    shutdown(sock, SHUT_RDWR);
    close(sock);
}
#else
static void dsGracefulShutdown( dsSocket_t xSocket )
{
    TickType_t xTimeOnShutdown;
    
    /* Reduced timeout to prevent long hangs */
    #define cmdSHUTDOWN_DELAY	( pdMS_TO_TICKS( 1000 ) )

    /* Check if socket is valid before attempting shutdown */
    if( xSocket == FREERTOS_INVALID_SOCKET )
    {
        return;
    }

    /* Initiate a shutdown in case it has not already been initiated. */
    FreeRTOS_shutdown( xSocket, FREERTOS_SHUT_RDWR );

    /* Wait for the shutdown to take effect, indicated by FreeRTOS_recv()
    returning an error. */
    xTimeOnShutdown = xTaskGetTickCount();
    do
    {
        if( FreeRTOS_recv( xSocket, NULL, ipconfigTCP_MSS, FREERTOS_MSG_DONTWAIT) < 0 )
        {
            /* Socket properly closed or error occurred */
            break;
        }
        
        /* Yield to other tasks while waiting */
        vTaskDelay( pdMS_TO_TICKS( 10 ) );
        
    } while( ( xTaskGetTickCount() - xTimeOnShutdown ) < cmdSHUTDOWN_DELAY );

    /* Finished with the socket and the task. */
    FreeRTOS_closesocket( xSocket );
}
#endif

/**
 * @brief Opens a TCP server socket on the specified port.
 *
 * This function creates a TCP socket, sets socket options, binds it to the given port,
 * and starts listening for incoming connections.
 *
 * @param port The port number on which the server will listen for incoming connections.
 * @return On success, returns the socket file descriptor (a non-negative integer).
 *         On failure, returns -1 and logs the error.
 *
 * The function performs the following steps:
 *   1. Creates a socket using AF_INET, SOCK_STREAM, and IPPROTO_IP.
 *   2. Sets the SO_REUSEADDR option to allow reuse of local addresses.
 *   3. Binds the socket to INADDR_ANY and the specified port.
 *   4. Starts listening on the socket with a backlog of 5.
 *
 * Errors are logged using DS_LOGE, and resources are cleaned up on failure.
 */
#if (DS_PLATFORM == ESP32)
static int dsOpenTCPServerSocket(uint16_t port)
{
    int sock;
    struct sockaddr_in server_addr;
    int opt = 1;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0)
    {
        DS_LOGE("Unable to create a TCP socket: errno %d", errno);
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        DS_LOGE("Socket unable to bind: errno %d", errno);
        close(sock);
        return -1;
    }

    if (listen(sock, 5) < 0)
    {
        DS_LOGE("Error occurred during listen: errno %d", errno);
        close(sock);
        return -1;
    }

    return sock;
}
#else
static dsSocket_t dsOpenTCPServerSocket(uint16_t port)
{
    struct freertos_sockaddr xBindAddress;
    Socket_t xSocket = FREERTOS_INVALID_SOCKET;
    static const TickType_t xReceiveTimeOut = portMAX_DELAY;
    const BaseType_t xBacklog = 20;
    BaseType_t xReuseSocket = pdTRUE;

    /* Attempt to open the socket. */
    xSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (xSocket != FREERTOS_INVALID_SOCKET)
    {
        /* Set a time out so accept() will just wait for a connection. */
        FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );

        /* Only one connection will be used at a time, so re-use the listening
        socket as the connected socket.  See SimpleTCPEchoServer.c for an example
        that accepts multiple connections. */
        FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_REUSE_LISTEN_SOCKET, &xReuseSocket, sizeof( xReuseSocket ) );

        /* Declare an xWinProperties structure. */
        WinProperties_t  xWinProps;

        /* Fill in the required buffer and window sizes.
         * Datastream packets are only 6-7 bytes, so small buffers suffice. */
        xWinProps.lTxBufSize = 128;  /* bytes */
        xWinProps.lTxWinSize = 1;    /* MSS */
        xWinProps.lRxBufSize = 128;  /* bytes */
        xWinProps.lRxWinSize = 1;    /* MSS */

        /* Use the structure with the
        FREERTOS_SO_WIN_PROPERTIES parameter in a call to
        FreeRTOS_setsockopt(). */
        FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProps, sizeof( xWinProps ) );
        
        /* Zero out the bind address structure. */
        memset((void *)&xBindAddress, 0x00, sizeof(xBindAddress));

        /* Set family and port. */
        xBindAddress.sin_port = FreeRTOS_htons(port);

        /* Bind the address to the socket. */
        if (FreeRTOS_bind(xSocket, &xBindAddress, sizeof(xBindAddress)) == -1)
        {
            FreeRTOS_closesocket(xSocket);
            xSocket = FREERTOS_INVALID_SOCKET;
        }
        else
        {
            /* Start listening for incoming connections. */
            FreeRTOS_listen(xSocket, xBacklog);
        }
    }

    return xSocket;
}
#endif

/**
 * @brief TCP server task handler for datastream communication.
 *
 * This function implements the main loop for a TCP server that handles datastream packets.
 * It performs the following steps:
 *   - Initializes datastream structures.
 *   - Opens a TCP server socket on the port specified by pvParameters.
 *   - Waits for incoming client connections.
 *   - For each client connection:
 *       - Receives packets of size DATASTREAM_INPUT_SIZE.
 *       - If the received packet size is correct, processes the packet and sends a response.
 *       - If the packet size is incorrect, sends an error response.
 *       - Handles socket errors and closes connections gracefully.
 *   - Cleans up sockets and resources on error or client disconnect.
 *
 * @param pvParameters Pointer to parameters passed to the task (expected to contain the port number).
 *
 * @note This function is intended to run as a FreeRTOS task and will delete itself on fatal errors.
 * @note Uses global logging macros (DS_LOGI, DS_LOGE) and relies on external datastream processing functions.
 */
void prvdsTCPTask(void *pvParameters)
{
    int32_t lBytes, lSent;

    // Create a socket for listening to incoming connections according to platform
    dsSockaddr_t client;
    dsSocket_t listeningSocket, connectedSocket;

    socklen_t xSize = sizeof(client);

    dsTxPacket txPacket;
    dsRxPacket rxPacket;

    dsInitialize();
    
    memset(rxPacket.buffer, 0x00, DATASTREAM_INPUT_SIZE);
    memset(txPacket.buffer, 0x00, DATASTREAM_OUTPUT_SIZE);
    lSent = 0;

    for (;;)
    {
        /* Attempt to open the socket.  The port number is passed in the task
        parameter.  The strange casting is to remove compiler warnings on 32-bit
        machines.  NOTE:  The FREERTOS_SO_REUSE_LISTEN_SOCKET option is used,
        so the listening and connecting socket are the same - meaning only one
        connection will be accepted at a time, and that listeningSocket must
        be created on each iteration. */
        listeningSocket = dsOpenTCPServerSocket((uint16_t)((uint32_t)pvParameters) & 0xffffUL);

        // Check if the socket was created successfully, if not, delete the task
        #if (DS_PLATFORM == ESP32)
        if (listeningSocket < 0)
        #else
        if (listeningSocket == FREERTOS_INVALID_SOCKET)
        #endif
        {
            DS_LOGE("Unable to create a TCP socket: errno %d", errno);
            // Delete the task if socket creation fails
            vTaskDelete(NULL);
        }

        DS_LOGI("Waiting for incoming connection...");
        #if (DS_PLATFORM == ESP32)
        connectedSocket = accept(listeningSocket, (struct sockaddr *)&client, &xSize);
        #else
        connectedSocket = FreeRTOS_accept(listeningSocket, &client, &xSize);
        #endif

        // Check if the accept was successful, if not, close the listening socket and continue
        #if (DS_PLATFORM == ESP32)
        if (connectedSocket < 0)
        #else
        if (connectedSocket != listeningSocket)
        #endif
        {
            DS_LOGE("Unable to accept connection: errno %d", errno);
            dsGracefulShutdown(listeningSocket);
            dsGracefulShutdown(connectedSocket);
            // Continue to the next iteration to wait for a new connection
            continue;
        }

        DS_LOGI("TCPdatastream Client connected");

        lSent = 0;
        while ( lSent >= 0 )
        {
            // Receive data from the connected socket
            lBytes = dsTCPRecv(connectedSocket, rxPacket.buffer, DATASTREAM_INPUT_SIZE, 0);

            if (lBytes == DATASTREAM_INPUT_SIZE)
            {
                dsProcessPacket(&rxPacket, &txPacket);

                lSent = dsTCPSend(connectedSocket, txPacket.buffer, DATASTREAM_OUTPUT_SIZE, 0);

            }
            else if (lBytes >= 0)
            {
                // Packet size error, get proper response from datastream
                dsPacketSizeError((uint16_t)lBytes, &txPacket);

                lSent = dsTCPSend(connectedSocket, txPacket.buffer, DATASTREAM_OUTPUT_SIZE, 0);

            }
            else
            {
                // Socket closed or error
                DS_LOGE("TCP socket error: errno %d", errno);
                break;
            }
        }

        /* When FREERTOS_SO_REUSE_LISTEN_SOCKET is used, connectedSocket and 
           listeningSocket are the same, so only close once to avoid double-free */
        dsGracefulShutdown(listeningSocket);
        if (connectedSocket != listeningSocket) {
            dsGracefulShutdown(connectedSocket);
        }
    }
}
/**
 * @brief Receives data from a TCP socket.
 *
 * This function receives data from a TCP socket and stores it in the provided buffer.
 * It uses the FreeRTOS_recv function for receiving data, which is platform-specific.
 *
 * @param xSocket The socket from which to receive data.
 * @param pvBuffer Pointer to the buffer where received data will be stored.
 * @param uxBufferLength The length of the buffer in bytes.
 * @param xFlags Flags for the receive operation (currently unused).
 * @return The number of bytes received, or -1 on error.
 */
static int32_t dsTCPRecv( dsSocket_t xSocket, void * pvBuffer, size_t uxBufferLength, BaseType_t xFlags )
{
    #if (DS_PLATFORM == ESP32)
    return (int32_t)recv(xSocket, pvBuffer, uxBufferLength, 0);
    #else
    return (int32_t)FreeRTOS_recv(xSocket, pvBuffer, uxBufferLength, 0);
    #endif
}

/**
 * @brief Sends data over a TCP socket.
 *
 * This function sends data over a TCP socket using the FreeRTOS_send function.
 * It is designed to be used with FreeRTOS sockets and handles the sending of
 * data buffers.
 *
 * @param xSocket The socket to which data will be sent.
 * @param pvBuffer Pointer to the data buffer to be sent.
 * @param uxDataLength The length of the data buffer in bytes.
 * @param xFlags Flags for the send operation (currently unused).
 * @return The number of bytes sent, or -1 on error.
 */
static int32_t dsTCPSend( dsSocket_t xSocket, const void * pvBuffer, size_t uxDataLength, BaseType_t xFlags )
{
    #if (DS_PLATFORM == ESP32)
    return (int32_t)send(xSocket, pvBuffer, uxDataLength, 0);
    #else
    return (int32_t)FreeRTOS_send(xSocket, pvBuffer, uxDataLength, 0);
    #endif
}

/*
 * dsCLI.c
 *
 * Text-based CLI interface over TCP for the datastream protocol.
 * Provides human-readable access to registers, parameters, and system commands.
 *
 * Created: 02/07/2026
 *  Author: Sofian
 */

#include "dsCLI.h"

#if (DS_CLI_ENABLE == 1)

#include "datastream.h"
#include "dsRegisters.h"
#include "dsParameters.h"

/* Standard includes. */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

TaskHandle_t dsCLITaskHandle = NULL;

/*
 * Static function prototypes
 */
static void prvdsCLITask(void *pvParameters);
static int32_t dsCLISend(dsSocket_t xSocket, const void *pvBuffer, size_t uxDataLength, BaseType_t xFlags);
static int32_t dsCLIRecv(dsSocket_t xSocket, void *pvBuffer, size_t uxBufferLength, BaseType_t xFlags);
static void dsCLISendString(dsSocket_t xSocket, const char *str);
static void dsCLIProcessLine(dsSocket_t xSocket, char *line, char *outBuffer, size_t outLen);

/* Built-in command handlers */
static void cmdReadReg(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen);
static void cmdWriteReg(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen);
static void cmdReadPar(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen);
static void cmdWritePar(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen);
static void cmdSys(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen);
static void cmdRegs(dsSocket_t xSocket, char *outBuffer, size_t outLen);
static void cmdPars(dsSocket_t xSocket, char *outBuffer, size_t outLen);
static void cmdName(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen);
static void cmdHelp(dsSocket_t xSocket);

/**
 * @brief Default weak implementation of user command handler.
 *
 * Override in your application to handle custom CLI commands.
 */
WEAK bool dsCLIProcessUserCommand(const char *input, char *output, size_t maxLen)
{
    (void)input;
    (void)output;
    (void)maxLen;
    return false;
}

/**
 * @brief Creates and starts the CLI TCP server FreeRTOS task.
 */
void dsCLITaskCreate(void)
{
    dsInitialize();

    if (xTaskCreate(
            prvdsCLITask, DS_CLI_TASK_NAME, DS_CLI_TASK_STACK_SIZE,
            (void *)DS_CLI_PORT, DS_CLI_TASK_PRIORITY, &dsCLITaskHandle)
        != pdPASS) {
        while (1) {;}
    }

    dsRegisterControlTask(dsCLITaskHandle, ds_control_TCP_CLI, DS_CLI_TASK_NAME);
}

/* ======================== Socket Helpers ======================== */

#if (DS_PLATFORM == ESP32)
static void dsCLIGracefulShutdown(int sock)
{
    shutdown(sock, SHUT_RDWR);
    close(sock);
}
#else
static void dsCLIGracefulShutdown(dsSocket_t xSocket)
{
    TickType_t xTimeOnShutdown;

    #define cliSHUTDOWN_DELAY ( pdMS_TO_TICKS( 1000 ) )

    if (xSocket == FREERTOS_INVALID_SOCKET)
    {
        return;
    }

    FreeRTOS_shutdown(xSocket, FREERTOS_SHUT_RDWR);

    xTimeOnShutdown = xTaskGetTickCount();
    do
    {
        if (FreeRTOS_recv(xSocket, NULL, ipconfigTCP_MSS, FREERTOS_MSG_DONTWAIT) < 0)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    } while ((xTaskGetTickCount() - xTimeOnShutdown) < cliSHUTDOWN_DELAY);

    FreeRTOS_closesocket(xSocket);
}
#endif

#if (DS_PLATFORM == ESP32)
static int dsOpenCLIServerSocket(uint16_t port)
{
    int sock;
    struct sockaddr_in server_addr;
    int opt = 1;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0)
    {
        DS_LOGE("CLI: Unable to create TCP socket: errno %d", errno);
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        DS_LOGE("CLI: Socket unable to bind: errno %d", errno);
        close(sock);
        return -1;
    }

    if (listen(sock, 5) < 0)
    {
        DS_LOGE("CLI: Error occurred during listen: errno %d", errno);
        close(sock);
        return -1;
    }

    return sock;
}
#else
static dsSocket_t dsOpenCLIServerSocket(uint16_t port)
{
    struct freertos_sockaddr xBindAddress;
    Socket_t xSocket = FREERTOS_INVALID_SOCKET;
    static const TickType_t xReceiveTimeOut = portMAX_DELAY;
    const BaseType_t xBacklog = 20;
    BaseType_t xReuseSocket = pdTRUE;

    xSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (xSocket != FREERTOS_INVALID_SOCKET)
    {
        FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof(xReceiveTimeOut));
        FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_REUSE_LISTEN_SOCKET, &xReuseSocket, sizeof(xReuseSocket));

        WinProperties_t xWinProps;
        /* CLI responses are longer than datastream packets, use larger buffers */
        xWinProps.lTxBufSize = 512;
        xWinProps.lTxWinSize = 1;
        xWinProps.lRxBufSize = 256;
        xWinProps.lRxWinSize = 1;

        FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_WIN_PROPERTIES, (void *)&xWinProps, sizeof(xWinProps));

        memset((void *)&xBindAddress, 0x00, sizeof(xBindAddress));
        xBindAddress.sin_port = FreeRTOS_htons(port);

        if (FreeRTOS_bind(xSocket, &xBindAddress, sizeof(xBindAddress)) == -1)
        {
            FreeRTOS_closesocket(xSocket);
            xSocket = FREERTOS_INVALID_SOCKET;
        }
        else
        {
            FreeRTOS_listen(xSocket, xBacklog);
        }
    }

    return xSocket;
}
#endif

static int32_t dsCLIRecv(dsSocket_t xSocket, void *pvBuffer, size_t uxBufferLength, BaseType_t xFlags)
{
    #if (DS_PLATFORM == ESP32)
    return (int32_t)recv(xSocket, pvBuffer, uxBufferLength, 0);
    #else
    return (int32_t)FreeRTOS_recv(xSocket, pvBuffer, uxBufferLength, 0);
    #endif
}

static int32_t dsCLISend(dsSocket_t xSocket, const void *pvBuffer, size_t uxDataLength, BaseType_t xFlags)
{
    #if (DS_PLATFORM == ESP32)
    return (int32_t)send(xSocket, pvBuffer, uxDataLength, 0);
    #else
    return (int32_t)FreeRTOS_send(xSocket, pvBuffer, uxDataLength, 0);
    #endif
}

static void dsCLISendString(dsSocket_t xSocket, const char *str)
{
    size_t len = strlen(str);
    if (len > 0)
    {
        dsCLISend(xSocket, str, len, 0);
    }
}

/* ======================== Main Task ======================== */

static void prvdsCLITask(void *pvParameters)
{
    int32_t lBytes;
    dsSockaddr_t client;
    dsSocket_t listeningSocket, connectedSocket;
    socklen_t xSize = sizeof(client);

    char lineBuffer[DS_CLI_MAX_INPUT_LENGTH];
    char outBuffer[DS_CLI_MAX_OUTPUT_LENGTH];
    char recvBuffer[32];
    size_t lineIndex = 0;

    dsInitialize();

    for (;;)
    {
        listeningSocket = dsOpenCLIServerSocket((uint16_t)((uint32_t)pvParameters) & 0xffffUL);

        #if (DS_PLATFORM == ESP32)
        if (listeningSocket < 0)
        #else
        if (listeningSocket == FREERTOS_INVALID_SOCKET)
        #endif
        {
            DS_LOGE("CLI: Unable to create TCP socket");
            vTaskDelete(NULL);
        }

        DS_LOGI("CLI: Waiting for incoming connection on port %u...", (unsigned)DS_CLI_PORT);

        #if (DS_PLATFORM == ESP32)
        connectedSocket = accept(listeningSocket, (struct sockaddr *)&client, &xSize);
        #else
        connectedSocket = FreeRTOS_accept(listeningSocket, &client, &xSize);
        #endif

        #if (DS_PLATFORM == ESP32)
        if (connectedSocket < 0)
        #else
        if (connectedSocket != listeningSocket)
        #endif
        {
            DS_LOGE("CLI: Unable to accept connection");
            dsCLIGracefulShutdown(listeningSocket);
            dsCLIGracefulShutdown(connectedSocket);
            continue;
        }

        DS_LOGI("CLI: Client connected");

        /* Send welcome banner and prompt */
        dsCLISendString(connectedSocket, "Datastream CLI\r\n> ");

        lineIndex = 0;
        memset(lineBuffer, 0, sizeof(lineBuffer));

        for (;;)
        {
            lBytes = dsCLIRecv(connectedSocket, recvBuffer, sizeof(recvBuffer), 0);

            if (lBytes <= 0)
            {
                /* Socket closed or error */
                break;
            }

            /* Process received bytes */
            for (int32_t i = 0; i < lBytes; i++)
            {
                char c = recvBuffer[i];

                if (c == '\r' || c == '\n')
                {
                    if (lineIndex > 0)
                    {
                        lineBuffer[lineIndex] = '\0';
                        dsCLIProcessLine(connectedSocket, lineBuffer, outBuffer, sizeof(outBuffer));
                        lineIndex = 0;
                    }
                    /* Send prompt after processing */
                    dsCLISendString(connectedSocket, "> ");
                }
                else if (c == '\b' || c == 0x7F)
                {
                    /* Handle backspace */
                    if (lineIndex > 0)
                    {
                        lineIndex--;
                    }
                }
                else if (lineIndex < (DS_CLI_MAX_INPUT_LENGTH - 1))
                {
                    lineBuffer[lineIndex++] = c;
                }
                else
                {
                    /* Line too long - reset */
                    dsCLISendString(connectedSocket, "\r\nError: Input too long\r\n");
                    lineIndex = 0;
                }
            }
        }

        DS_LOGI("CLI: Client disconnected");

        dsCLIGracefulShutdown(listeningSocket);
        if (connectedSocket != listeningSocket)
        {
            dsCLIGracefulShutdown(connectedSocket);
        }
    }
}

/* ======================== Command Dispatch ======================== */

/**
 * @brief Skip leading whitespace in a string.
 * @return Pointer to first non-whitespace character
 */
static const char* skipWhitespace(const char *str)
{
    while (*str == ' ' || *str == '\t')
    {
        str++;
    }
    return str;
}

static void dsCLIProcessLine(dsSocket_t xSocket, char *line, char *outBuffer, size_t outLen)
{
    const char *input = skipWhitespace(line);

    if (*input == '\0')
    {
        return;
    }

    /* Match built-in commands */
    if (strncmp(input, "read reg ", 9) == 0)
    {
        cmdReadReg(xSocket, input + 9, outBuffer, outLen);
    }
    else if (strncmp(input, "write reg ", 10) == 0)
    {
        cmdWriteReg(xSocket, input + 10, outBuffer, outLen);
    }
    else if (strncmp(input, "read par ", 9) == 0)
    {
        cmdReadPar(xSocket, input + 9, outBuffer, outLen);
    }
    else if (strncmp(input, "write par ", 10) == 0)
    {
        cmdWritePar(xSocket, input + 10, outBuffer, outLen);
    }
    else if (strncmp(input, "sys ", 4) == 0)
    {
        cmdSys(xSocket, input + 4, outBuffer, outLen);
    }
    else if (strcmp(input, "sys") == 0)
    {
        dsCLISendString(xSocket, "Usage: sys <cmd> [val]\r\n");
    }
    else if (strcmp(input, "regs") == 0)
    {
        cmdRegs(xSocket, outBuffer, outLen);
    }
    else if (strcmp(input, "pars") == 0)
    {
        cmdPars(xSocket, outBuffer, outLen);
    }
    else if (strncmp(input, "name ", 5) == 0)
    {
        cmdName(xSocket, input + 5, outBuffer, outLen);
    }
    else if (strcmp(input, "name") == 0)
    {
        cmdName(xSocket, NULL, outBuffer, outLen);
    }
    else if (strcmp(input, "help") == 0)
    {
        cmdHelp(xSocket);
    }
    else
    {
        /* Try user-defined command handler */
        if (!dsCLIProcessUserCommand(input, outBuffer, outLen))
        {
            dsCLISendString(xSocket, "Unknown command. Type 'help' for available commands.\r\n");
        }
        else
        {
            dsCLISendString(xSocket, outBuffer);
        }
    }
}

/* ======================== Built-in Command Handlers ======================== */

static void cmdReadReg(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen)
{
    char *endptr;
    uint16_t address = (uint16_t)strtoul(args, &endptr, 10);

    if (endptr == args)
    {
        dsCLISendString(xSocket, "Usage: read reg <addr>\r\n");
        return;
    }

    reply_t reply = {0};
    uint32_t value = dsGetRegister(address, &reply);

    if (reply.val >= 0)
    {
        snprintf(outBuffer, outLen, "[%lu] = %lu\r\n",
                 (unsigned long)address, (unsigned long)value);
    }
    else
    {
        snprintf(outBuffer, outLen, "Error: %s", reply.string);
    }
    dsCLISendString(xSocket, outBuffer);
}

static void cmdWriteReg(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen)
{
    char *endptr;
    uint16_t address = (uint16_t)strtoul(args, &endptr, 10);

    if (endptr == args)
    {
        dsCLISendString(xSocket, "Usage: write reg <addr> <val>\r\n");
        return;
    }

    const char *valStr = skipWhitespace(endptr);
    uint32_t value = (uint32_t)strtoul(valStr, &endptr, 10);

    if (endptr == valStr)
    {
        dsCLISendString(xSocket, "Usage: write reg <addr> <val>\r\n");
        return;
    }

    reply_t reply = {0};
    dsSetRegister(address, value, &reply);

    if (reply.val >= 0)
    {
        snprintf(outBuffer, outLen, "OK [%lu] = %lu\r\n",
                 (unsigned long)address, (unsigned long)value);
    }
    else
    {
        snprintf(outBuffer, outLen, "Error: %s", reply.string);
    }
    dsCLISendString(xSocket, outBuffer);
}

static void cmdReadPar(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen)
{
    char *endptr;
    uint16_t address = (uint16_t)strtoul(args, &endptr, 10);

    if (endptr == args)
    {
        dsCLISendString(xSocket, "Usage: read par <addr>\r\n");
        return;
    }

    reply_t reply = {0};
    uint32_t value = dsGetParameter(address, &reply);

    if (reply.val >= 0)
    {
        snprintf(outBuffer, outLen, "[%lu] = %lu\r\n",
                 (unsigned long)address, (unsigned long)value);
    }
    else
    {
        snprintf(outBuffer, outLen, "Error: %s", reply.string);
    }
    dsCLISendString(xSocket, outBuffer);
}

static void cmdWritePar(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen)
{
    char *endptr;
    uint16_t address = (uint16_t)strtoul(args, &endptr, 10);

    if (endptr == args)
    {
        dsCLISendString(xSocket, "Usage: write par <addr> <val>\r\n");
        return;
    }

    const char *valStr = skipWhitespace(endptr);
    uint32_t value = (uint32_t)strtoul(valStr, &endptr, 10);

    if (endptr == valStr)
    {
        dsCLISendString(xSocket, "Usage: write par <addr> <val>\r\n");
        return;
    }

    reply_t reply = {0};
    dsSetParameter(address, value, &reply);

    if (reply.val >= 0)
    {
        snprintf(outBuffer, outLen, "OK [%lu] = %lu\r\n",
                 (unsigned long)address, (unsigned long)value);
    }
    else
    {
        snprintf(outBuffer, outLen, "Error: %s", reply.string);
    }
    dsCLISendString(xSocket, outBuffer);
}

static void cmdSys(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen)
{
    char *endptr;
    uint32_t command = (uint32_t)strtoul(args, &endptr, 10);

    if (endptr == args)
    {
        dsCLISendString(xSocket, "Usage: sys <cmd> [val]\r\n");
        return;
    }

    /* Parse optional value */
    const char *valStr = skipWhitespace(endptr);
    uint32_t value = 0;
    if (*valStr != '\0')
    {
        value = (uint32_t)strtoul(valStr, NULL, 10);
    }

    /* Build a datastream packet and process it */
    dsRxPacket inputPacket;
    dsTxPacket outputPacket;
    memset(&inputPacket, 0, sizeof(inputPacket));
    memset(&outputPacket, 0, sizeof(outputPacket));

    inputPacket.contents.type = ds_type_SYS_COMMAND;
    inputPacket.contents.address = (uint16_t)command;
    inputPacket.contents.value = value;

    dsProcessSysCommand(&inputPacket, &outputPacket);

    if (outputPacket.contents.status >= 0)
    {
        snprintf(outBuffer, outLen, "OK (status=%d, value=%lu)\r\n",
                 (int)outputPacket.contents.status, (unsigned long)outputPacket.contents.value);
    }
    else
    {
        snprintf(outBuffer, outLen, "Error (status=%d)\r\n",
                 (int)outputPacket.contents.status);
    }
    dsCLISendString(xSocket, outBuffer);
}

static void cmdRegs(dsSocket_t xSocket, char *outBuffer, size_t outLen)
{
    for (uint16_t i = 0; i < DS_REGISTER_COUNT; i++)
    {
        const char *access = (i < DS_REGISTERS_READ_ONLY_COUNT) ? "RO" : "RW";
        snprintf(outBuffer, outLen, "[%lu] = %lu (%s)\r\n",
                 (unsigned long)i, (unsigned long)REGS.byAddress[i], access);
        dsCLISendString(xSocket, outBuffer);
    }
}

static void cmdPars(dsSocket_t xSocket, char *outBuffer, size_t outLen)
{
    for (uint16_t i = 0; i < DS_PARAMETER_COUNT; i++)
    {
        snprintf(outBuffer, outLen, "[%lu] = %lu\r\n",
                 (unsigned long)i, (unsigned long)PARS.byAddress[i]);
        dsCLISendString(xSocket, outBuffer);
    }
}

static void cmdName(dsSocket_t xSocket, const char *args, char *outBuffer, size_t outLen)
{
    if (args == NULL || *args == '\0')
    {
        /* Display current board name */
        snprintf(outBuffer, outLen, "Board name: %s\r\n", dsGetBoardName());
        dsCLISendString(xSocket, outBuffer);
    }
    else
    {
        /* Set new board name */
        dsSetBoardName(args);
        snprintf(outBuffer, outLen, "OK Board name set to: %s\r\n", dsGetBoardName());
        dsCLISendString(xSocket, outBuffer);
    }
}

static void cmdHelp(dsSocket_t xSocket)
{
    dsCLISendString(xSocket, "Available commands:\r\n");
    dsCLISendString(xSocket, "  read reg <addr>         - Read a register\r\n");
    dsCLISendString(xSocket, "  write reg <addr> <val>  - Write a register\r\n");
    dsCLISendString(xSocket, "  read par <addr>         - Read a parameter\r\n");
    dsCLISendString(xSocket, "  write par <addr> <val>  - Write a parameter\r\n");
    dsCLISendString(xSocket, "  sys <cmd> [val]         - System command\r\n");
    dsCLISendString(xSocket, "  regs                    - Print all registers\r\n");
    dsCLISendString(xSocket, "  pars                    - Print all parameters\r\n");
    dsCLISendString(xSocket, "  name [new_name]         - Get/set board name\r\n");
    dsCLISendString(xSocket, "  help                    - Show this help\r\n");
}

#endif /* DS_CLI_ENABLE == 1 */

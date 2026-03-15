/*
 * ds_system_register_names.h
 *
 * Library-owned system register definitions for the datastream library.
 * These registers are accessed via ds_type_READ_SYSTEM_REGISTER and
 * ds_type_WRITE_SYSTEM_REGISTER packet types — NOT via the user register commands.
 *
 * This file is included internally by dsRegisters.h and must NOT be included
 * or modified by the user.
 *
 * Created: 14/03/2026
 *  Author: Sofian.jafar
 */

#ifndef DS_SYSTEM_REGISTER_NAMES_H_
#define DS_SYSTEM_REGISTER_NAMES_H_

#include <ds_default_config.h>

typedef struct PACKED
{
    #if (DS_STATS_ENABLE == 1)
    uint32_t  DS_PACKET_COUNT;    /**< Number of packets received */
    uint32_t  DS_ERROR_COUNT;     /**< Number of errors encountered */
    #endif

    uint32_t  CONTROL_INTERFACE;  /**< Current control interface type (ds_control_interface_t) */
    uint32_t  COUNTER_1HZ;        /**< 1 Hz counter — updated by application code via SYS_REGS */

} ds_system_register_names_t;

#endif /* DS_SYSTEM_REGISTER_NAMES_H_ */

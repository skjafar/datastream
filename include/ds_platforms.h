/*
 * ds_platforms.h
 *
 * Platform definitions for datastream library
 * This file defines numeric constants for all supported platforms
 * 
 * Created: 29/08/2025
 *  Author: Sofian.jafar
 */

#ifndef DS_PLATFORMS_H_
#define DS_PLATFORMS_H_

/****** Datastream Platform Definitions ******/
// These constants are used in preprocessor #if statements
// Users should include this file and use these constants

#define ESP32           1
#define RP2040          2  
#define RP2350          3
#define STM32           4
#define GENERIC_PLATFORM 5

// Add new platforms here with incrementing numbers
// #define NEW_PLATFORM    6

#endif /* DS_PLATFORMS_H_ */
#pragma once

#ifndef _RWREG_H_
#define _RWREG_H_

#include <string>

// ============================================================================
// CannonCruise - RenderWare Registry Interface (rwreg.h)
// ============================================================================

/**
 * @file rwreg.h
 * @brief Wrapper functions for Windows Registry access.
 *
 * Used to read/write the RenderWare engine's and the application's settings
 * via the Windows Registry. (E.g. resolution settings, volume levels, etc.)
 */

/**
 * @brief Writes a string registry value at the given path.
 * @param path   Main path (e.g.: "Software\\RenderWare")
 * @param subKey Sub key name
 * @param entry  Value name
 * @param value  String value to write
 */
void RwRegSetString(const std::string& path, const std::string& subKey, const std::string& entry, const std::string& value);

/**
 * @brief Reads a string registry value from the given path.
 * @param path         Main path
 * @param subKey       Sub key name
 * @param entry        Value name
 * @param defaultValue Default string returned if the value is not found
 * @return std::string The read value or the default value
 */
std::string RwRegGetString(const std::string& path, const std::string& subKey, const std::string& entry, const std::string& defaultValue);

/**
 * @brief Writes an integer (int) registry value at the given path.
 * @param path   Main path
 * @param subKey Sub key name
 * @param entry  Value name
 * @param value  Integer value to write
 */
void RwRegSetInt(const std::string& path, const std::string& subKey, const std::string& entry, int value);

/**
 * @brief Reads an integer (int) registry value from the given path.
 * @param path         Main path
 * @param subKey       Sub key name
 * @param entry        Value name
 * @param defaultValue Default integer returned if the value is not found
 * @return int         The read value or the default value
 */
int RwRegGetInt(const std::string& path, const std::string& subKey, const std::string& entry, int defaultValue);

#endif // _RWREG_H_


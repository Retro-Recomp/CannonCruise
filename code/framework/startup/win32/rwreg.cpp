#include "rwreg.h"

#define WIN32_LEAN_AND_MEAN
#include <cassert>
#include <windows.h>

// ============================================================================
// CannonCruise - RenderWare Registry Interface (rwreg.cpp)
// Original file:
// D:\Projects\CannonCruisePC\code\framework\startup\win32\rwreg.cpp — reverse-
// engineered and rewritten by analyzing with Ghidra and CC.asm.
//
// All RenderWare settings are kept in the Windows Registry under this root:
//   HKEY_CURRENT_USER\Software\Criterion\RenderWare\<path>
// ============================================================================

// RenderWare's registry root path (0x00612e88)
static const char *const RW_REG_ROOT = "Software\\Criterion\\RenderWare\\";

// Common: build the full key path
// e.g.: path="CannonCruise/3.6", subKey="" ->
// "Software\Criterion\RenderWare\CannonCruise/3.6"
static std::string BuildKeyPath(const std::string &path,
                                const std::string &subKey) {
  std::string fullPath = RW_REG_ROOT;
  if (!path.empty()) {
    fullPath += path;
    if (!subKey.empty() && fullPath.back() != '\\') {
      fullPath += '\\';
    }
  }
  if (!subKey.empty()) {
    fullPath += subKey;
  }
  return fullPath;
}

// ============================================================================
// RwRegSetString (FUN_00416080)
// Binary Address: 0x00416080
// Task: Writes a string under the given key and value name.
//   - Adresler: RegCreateKeyExA (0x002103FEh), RegSetValueExA (0x002103ECh),
//   RegCloseKey
//   - Assert: !entry.empty() (0x00612eec)
//   - Root: HKEY_CURRENT_USER
// ============================================================================
void RwRegSetString(const std::string &path, const std::string &subKey,
                    const std::string &entry, const std::string &value) {
  assert(!entry.empty() && "!entry.empty() Failed");

  std::string fullKey = BuildKeyPath(path, subKey);

  HKEY hKey = NULL;
  DWORD dwDisposition = 0;
  LONG result = RegCreateKeyExA(
      HKEY_CURRENT_USER, // hKey for RegCreateKeyExA (DAT_80000001)
      fullKey.c_str(),   // lpSubKey
      0,                 // Reserved
      NULL,              // lpClass (lpClass_00635580 -> empty string)
      0,                 // dwOptions
      KEY_ALL_ACCESS,    // samDesired (0x1F = KEY_ALL_ACCESS — 0x0000001F)
      NULL,              // lpSecurityAttributes
      &hKey,             // phkResult
      &dwDisposition     // lpdwDisposition
  );

  if (result == ERROR_SUCCESS) {
    // +1 for null terminator — matches INC EDX at 0x0041632c
    DWORD cbData = static_cast<DWORD>(value.size()) + 1;
    RegSetValueExA(hKey,          // HKEY
                   entry.c_str(), // lpValueName
                   0,             // Reserved
                   REG_SZ,        // dwType = 1 (0x00416333)
                   reinterpret_cast<const BYTE *>(value.c_str()), // lpData
                   cbData                                         // cbData
    );
    RegCloseKey(hKey);
  }
}

// ============================================================================
// RwRegGetString (FUN_00416690)
// Binary Address: 0x00416690
// Task: Reads the string value under the given key.
//   - Tries HKCU first (read mode: 0x20019), if that fails tries HKLM,
//     if that fails too, creates a new key under HKCU and writes the default
//     value.
//   - Assert: !entry.empty()
// ============================================================================
std::string RwRegGetString(const std::string &path, const std::string &subKey,
                           const std::string &entry,
                           const std::string &defaultValue) {
  assert(!entry.empty() && "!entry.empty() Failed");

  std::string fullKey = BuildKeyPath(path, subKey);

  // Read HKCU first (READ_ONLY)
  HKEY hKey = NULL;
  LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, // DAT_80000001
                              fullKey.c_str(), 0,
                              KEY_READ | KEY_WRITE, // samDesired = 0x20019
                              &hKey);

  if (result != ERROR_SUCCESS) {
    // If HKCU fails, try HKLM (READ_ONLY)
    result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, // DAT_80000002
                           fullKey.c_str(), 0,
                           KEY_READ | KEY_WRITE, // samDesired = 0x20019
                           &hKey);

    if (result != ERROR_SUCCESS) {
      // Both fail: create a new key under HKCU,
      // write the default value (FUN_00416080 call at 0x004169a7)
      RwRegSetString(path, subKey, entry, defaultValue);
      return defaultValue;
    }
  }

  // Key opened, query the value
  char buffer[MAX_PATH] = {};
  DWORD dwType = REG_SZ;
  DWORD dwSize = sizeof(buffer);
  result = RegQueryValueExA(hKey, entry.c_str(), NULL, &dwType,
                            reinterpret_cast<LPBYTE>(buffer), &dwSize);
  RegCloseKey(hKey);

  if (result == ERROR_SUCCESS && dwType == REG_SZ) {
    return std::string(buffer);
  }

  return defaultValue;
}

// ============================================================================
// RwRegSetInt (same mechanism as FUN_00416080, REG_DWORD)
// Integer helpers — there is no separate function for this in the binary;
// integers are stored as REG_SZ (string) too, and are read with sscanf.
// ============================================================================
void RwRegSetInt(const std::string &path, const std::string &subKey,
                 const std::string &entry, int value) {
  assert(!entry.empty() && "!entry.empty() Failed");
  RwRegSetString(path, subKey, entry, std::to_string(value));
}

int RwRegGetInt(const std::string &path, const std::string &subKey,
                const std::string &entry, int defaultValue) {
  assert(!entry.empty() && "!entry.empty() Failed");
  std::string s =
      RwRegGetString(path, subKey, entry, std::to_string(defaultValue));
  try {
    return std::stoi(s);
  } catch (...) {
    return defaultValue;
  }
}

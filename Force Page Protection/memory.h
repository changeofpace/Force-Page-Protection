#pragma once

#include <Windows.h>

namespace memory {

bool IsValidPageProtection(DWORD PageProtection);
bool RegionIsMappedView(HANDLE ProcessHandle, PVOID BaseAddress, SIZE_T RegionSize);
bool ViewHasProtectedProtection(HANDLE ProcessHandle,
                                PVOID BaseAddress,
                                SIZE_T RegionSize,
                                DWORD NewProtection);
bool RemapViewOfSection(HANDLE ProcessHandle,
                        PVOID BaseAddress,
                        SIZE_T RegionSize,
                        DWORD NewProtection);

}; // namespace memory
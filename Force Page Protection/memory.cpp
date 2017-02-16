#include "memory.h"
#include "ntapi.h"

bool memory::IsValidPageProtection(DWORD PageProtection)
{
    switch (PageProtection)
    {
    case PAGE_NOACCESS:
    case PAGE_READONLY:
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
    case PAGE_GUARD:
    case PAGE_NOCACHE:
    case PAGE_WRITECOMBINE:
    //case PAGE_REVERT_TO_FILE_MAP:
        return true;
    default:
        break;
    }
    return false;
}

bool memory::RegionIsMappedView(HANDLE ProcessHandle, PVOID BaseAddress, SIZE_T RegionSize)
{
    MEMORY_BASIC_INFORMATION mbi = {};
    SIZE_T returnLength = 0;
    NTSTATUS status = ntapi::NtQueryVirtualMemory(ProcessHandle,
                                                  BaseAddress,
                                                  ntapi::MemoryBasicInformation,
                                                  PVOID(&mbi),
                                                  sizeof(mbi),
                                                  &returnLength);
    return status == ntapi::STATUS_SUCCESS &&
           mbi.State == MEM_COMMIT &&
           mbi.Type == MEM_MAPPED;
}

bool memory::ViewHasProtectedProtection(HANDLE ProcessHandle,
                                        PVOID BaseAddress,
                                        SIZE_T RegionSize,
                                        DWORD NewProtection)
{
    PVOID regionBase = PVOID(BaseAddress);
    SIZE_T regionSize = RegionSize;
    DWORD oldProtection = 0;
    ntapi::NTSTATUS status = ntapi::NtProtectVirtualMemory(ProcessHandle,
                                                           &regionBase,
                                                           &regionSize,
                                                           NewProtection,
                                                           &oldProtection);
    return status == ntapi::STATUS_INVALID_PAGE_PROTECTION ||
           status == ntapi::STATUS_SECTION_PROTECTION;
}

static bool _RemapViewOfSection(HANDLE ProcessHandle,
                                PVOID BaseAddress,
                                SIZE_T RegionSize,
                                DWORD NewProtection,
                                PVOID CopyBuffer)
{
    // Backup the view's content.
    SIZE_T numberOfBytesRead = 0;
    if (!ReadProcessMemory(ProcessHandle, BaseAddress, CopyBuffer, RegionSize, &numberOfBytesRead))
        return false;

    // Create a section to store the remapped view.
    HANDLE hSection = NULL;
    LARGE_INTEGER sectionMaxSize = {};
    sectionMaxSize.QuadPart = RegionSize;
    ntapi::NTSTATUS status = ntapi::NtCreateSection(&hSection,
                                                    SECTION_ALL_ACCESS,
                                                    NULL,
                                                    &sectionMaxSize,
                                                    PAGE_EXECUTE_READWRITE,
                                                    SEC_COMMIT,
                                                    NULL);
    if (status != ntapi::STATUS_SUCCESS)
        return false;

    // Unmap the current view.
    status = ntapi::NtUnmapViewOfSection(ProcessHandle, BaseAddress);
    if (status != ntapi::STATUS_SUCCESS)
        return false;

    // Map the replacement view with the requested protection.
    PVOID viewBase = BaseAddress;
    LARGE_INTEGER sectionOffset = {};
    SIZE_T viewSize = 0;
    status = ntapi::NtMapViewOfSection(hSection,
                                       ProcessHandle,
                                       &viewBase,
                                       0,
                                       RegionSize,
                                       &sectionOffset,
                                       &viewSize,
                                       ntapi::ViewUnmap,
                                       0,
                                       NewProtection);
    if (status != ntapi::STATUS_SUCCESS)
        return false;

    // Restore the view's content.
    SIZE_T numberOfBytesWritten = 0;
    if (!WriteProcessMemory(ProcessHandle, viewBase, CopyBuffer, viewSize, &numberOfBytesWritten))
        return false;

    return true;
}

bool memory::RemapViewOfSection(HANDLE ProcessHandle,
                                PVOID BaseAddress,
                                SIZE_T RegionSize,
                                DWORD NewProtection)
{
    PVOID copybuf = VirtualAlloc(NULL, RegionSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!copybuf)
        return false;
    bool result = _RemapViewOfSection(ProcessHandle, BaseAddress, RegionSize, NewProtection, copybuf);
    VirtualFree(copybuf, 0, MEM_RELEASE);
    return result;
}

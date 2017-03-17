# Force Page Protection

## Summary

This x64dbg plugin sets the page protection for memory mapped views in scenarios which cause NtProtectVirtualMemory to fail.

## Motivation

NtProtectVirtualMemory will fail for memory mapped views with valid arguments in these scenarios:

1. The view is mapped with the undocumented allocation type: **SEC\_NO\_CHANGE** (0x00400000). 

2. The desired protection is incompatible with the view's initial protection. Example: trying to set the protection for a view with an initial protection of **PAGE\_READONLY** to **PAGE\_EXECUTE\_READWRITE**.

3. The view and/or backing section are created using large pages (unconfirmed / not currently supported).

A process can utilize these cases as an anti-patching mechanism. A demo of this can be found **[here](https://github.com/changeofpace/Self-Remapping-Code)**.

This plugin defeats this technique by remapping the view with the desired protection.

## Usage

### Added Commands

- **ForcePageProtection**
- **fpp**

### Syntax

    ForcePageProtection [address, protection]

This command attempts to set the page protection for the memory region that contains **address**. The **protection** argument is interpretted as a hex value representing a PAGE\_* constant.<sup>[1](#reference1)</sup> If no arguments are specified, the address is set to the active address in the disassembly view, and the protection is set to PAGE\_EXECUTE\_READWRITE (0x40).

### Example

Given the following virtual address space for a process:

    Address             Size                 Type   Protection
    00000000`00010000   00000000`00010000    MAP    -RW--
    00000000`00020000   00000000`0000C000    PRV    ERW--
    00000000`00030000   00000000`00004000    MAP    -R---
    00000000`00040000   00000000`00001000    MAP    -R---
    ...

<pre>ForcePageProtection 32000, 40</pre>

Will attempt to set all pages in the range ```30000 - 33FFF``` to **PAGE\_EXECUTE\_READWRITE**.

## Building

A post-build event require the **"X96DBG\_PATH"** environment variable to be defined to the x64dbg installation directory.

## Notes

The following protection values are currently not supported:

- PAGE\_TARGETS\_INVALID
- PAGE\_TARGETS\_NO\_UPDATE
- PAGE\_ENCLAVE\_THREAD\_CONTROL
- PAGE\_ENCLAVE\_UNVALIDATED
- PAGE\_REVERT\_TO\_FILE\_MAP

## References

demo: [Self-Remapping-Code](https://github.com/changeofpace/Self-Remapping-Code)

<a name="reference1">1</a>: [MSDN: Memory Protection Constants](https://msdn.microsoft.com/en-us/library/windows/desktop/aa366786)

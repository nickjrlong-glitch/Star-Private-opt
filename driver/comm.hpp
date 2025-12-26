#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <vector>

uintptr_t virtualaddy;
uintptr_t cr3;

#define code_rw CTL_CODE(FILE_DEVICE_VIDEO, 0x1545, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_ba CTL_CODE(FILE_DEVICE_VIDEO, 0x1546, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_get_guarded_region CTL_CODE(FILE_DEVICE_VIDEO, 0x1547, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_GetDirBase CTL_CODE(FILE_DEVICE_VIDEO, 0x1548, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_kernel_aimbot CTL_CODE(FILE_DEVICE_VIDEO, 0x1549, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_mouse_aimbot CTL_CODE(FILE_DEVICE_VIDEO, 0x154A, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_security 0x83b5b69

typedef struct _rw {
	INT32 security;
	INT32 process_id;
	ULONGLONG address;
	ULONGLONG buffer;
	ULONGLONG size;
	BOOLEAN write;
} rw, * prw;

typedef struct _ba {
	INT32 security;
	INT32 process_id;
	ULONGLONG* address;
} ba, * pba;

typedef struct _ga {
	INT32 security;
	ULONGLONG* address;
} ga, * pga;

typedef struct _MEMORY_OPERATION_DATA {
	uint32_t        pid;
	ULONGLONG*		cr3;
} MEMORY_OPERATION_DATA, * PMEMORY_OPERATION_DATA;

typedef struct _KERNEL_AIMBOT_DATA {
	INT32 security;
	INT32 process_id;
	INT32 target_x;
	INT32 target_y;
	INT32 current_x;
	INT32 current_y;
	INT32 smoothing;
	BOOLEAN enabled;
} KERNEL_AIMBOT_DATA, * PKERNEL_AIMBOT_DATA;

typedef struct _MOUSE_AIMBOT_DATA {
	INT32 security;
	INT32 process_id;
	INT32 target_x;
	INT32 target_y;
	INT32 smoothing;
	BOOLEAN enabled;
	BOOLEAN use_humanization;
} MOUSE_AIMBOT_DATA, * PMOUSE_AIMBOT_DATA;

namespace intel_driver {
	HANDLE driver_handle;
	INT32 process_id;

	bool init_driver() {
		driver_handle = CreateFileW(L"\\\\.\\IntelGraphicsDriver", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		if (!driver_handle || (driver_handle == INVALID_HANDLE_VALUE))
			return false;

		return true;
	}

	void read_physical(PVOID address, PVOID buffer, DWORD size) {
		_rw arguments = { 0 };
		arguments.security = code_security;
		arguments.address = (ULONGLONG)address;
		arguments.buffer = (ULONGLONG)buffer;
		arguments.size = size;
		arguments.process_id = process_id;
		arguments.write = FALSE;

		DeviceIoControl(driver_handle, code_rw, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);
	}

	void write_physical(PVOID address, PVOID buffer, DWORD size) {
		_rw arguments = { 0 };
		arguments.security = code_security;
		arguments.address = (ULONGLONG)address;
		arguments.buffer = (ULONGLONG)buffer;
		arguments.size = size;
		arguments.process_id = process_id;
		arguments.write = TRUE;

		DeviceIoControl(driver_handle, code_rw, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);
	}

	uintptr_t fetch_cr3() {
		uintptr_t cr3 = NULL;
		_MEMORY_OPERATION_DATA arguments = { 0 };
		arguments.pid = process_id;
		arguments.cr3 = (ULONGLONG*)&cr3;
		DeviceIoControl(driver_handle, code_GetDirBase, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);
		return cr3;
	}

	uintptr_t find_image() {
		uintptr_t image_address = { NULL };
		_ba arguments = { NULL };
		arguments.security = code_security;
		arguments.process_id = process_id;
		arguments.address = (ULONGLONG*)&image_address;
		DeviceIoControl(driver_handle, code_ba, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);
		return image_address;
	}

	bool kernel_aimbot(INT32 target_x, INT32 target_y, INT32 smoothing, BOOLEAN enabled) {
		KERNEL_AIMBOT_DATA data = { 0 };
		data.security = code_security;
		data.process_id = process_id;
		data.target_x = target_x;
		data.target_y = target_y;
		data.smoothing = smoothing;
		data.enabled = enabled;

		DWORD bytes_returned = 0;
		return DeviceIoControl(driver_handle, code_kernel_aimbot, &data, sizeof(data), nullptr, 0, &bytes_returned, NULL);
	}

	bool mouse_aimbot(INT32 target_x, INT32 target_y, INT32 smoothing, BOOLEAN enabled, BOOLEAN use_humanization) {
		MOUSE_AIMBOT_DATA data = { 0 };
		data.security = code_security;
		data.process_id = process_id;
		data.target_x = target_x;
		data.target_y = target_y;
		data.smoothing = smoothing;
		data.enabled = enabled;
		data.use_humanization = use_humanization;

		DWORD bytes_returned = 0;
		return DeviceIoControl(driver_handle, code_mouse_aimbot, &data, sizeof(data), nullptr, 0, &bytes_returned, NULL);
	}

	INT32 find_process(LPCTSTR process_name) {
		PROCESSENTRY32 pt;
		HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		pt.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hsnap, &pt)) {
			do {
				if (!lstrcmpi(pt.szExeFile, process_name)) {
					CloseHandle(hsnap);
					process_id = pt.th32ProcessID;
					return pt.th32ProcessID;
				}
			} while (Process32Next(hsnap, &pt));
		}
		CloseHandle(hsnap);
		return { NULL };
	}
}

template <typename T>
T read(uint64_t address) {
	T buffer{ };
	intel_driver::read_physical((PVOID)address, &buffer, sizeof(T));
	return buffer;
}

template <typename T>
T write(uint64_t address, T buffer) {
	intel_driver::write_physical((PVOID)address, &buffer, sizeof(T));
	return buffer;
}

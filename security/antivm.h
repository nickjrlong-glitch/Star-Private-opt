#pragma once
#include <Windows.h>
#include <intrin.h>
#include <winternl.h>
#include <Psapi.h>
#include <vector>
#include <string>

namespace vmDetection {
	static bool CheckVMDrivers() {
		const wchar_t* vmDrivers[] = {
			L"VBoxGuest.sys",
			L"VBoxMouse.sys",
			L"VBoxSF.sys",
			L"VBoxVideo.sys",
			L"vmhgfs.sys",
			L"vmGuestLib.sys",
			L"VMToolsHook.sys",
			L"vmmouse.sys",
			L"vmrawdsk.sys",
			L"vmusbmouse.sys",
			L"vmxnet.sys",
			L"vmx_svga.sys",
			L"vboxsf.sys"
		};

		for (const auto& driver : vmDrivers) {
			DWORD drivers[1024];
			DWORD cbNeeded;
			if (K32EnumDeviceDrivers((LPVOID*)drivers, sizeof(drivers), &cbNeeded)) {
				int driverCount = cbNeeded / sizeof(drivers[0]);
				for (int i = 0; i < driverCount; i++) {
					wchar_t driverPath[MAX_PATH];
					if (K32GetDeviceDriverBaseNameW((LPVOID)drivers[i], driverPath, MAX_PATH)) {
						if (_wcsicmp(driverPath, driver) == 0) {
							return true;
						}
					}
				}
			}
		}
		return false;
	}

	static bool CheckVMServices() {
		const wchar_t* vmServices[] = {
			L"VBoxService",
			L"VBoxTray",
			L"VMwareService",
			L"VMwareTray",
			L"VMwareTools",
			L"Parallels Tools",
			L"prl_cc",
			L"prl_tools"
		};

		SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
		if (scm) {
			DWORD bytesNeeded = 0;
			DWORD servicesReturned = 0;
			DWORD resumeHandle = 0;

			EnumServicesStatusExW(scm,
				SC_ENUM_PROCESS_INFO,
				SERVICE_WIN32,
				SERVICE_STATE_ALL,
				NULL,
				0,
				&bytesNeeded,
				&servicesReturned,
				&resumeHandle,
				NULL);

			if (GetLastError() == ERROR_MORE_DATA) {
				BYTE* buffer = new BYTE[bytesNeeded];
				if (EnumServicesStatusExW(scm,
					SC_ENUM_PROCESS_INFO,
					SERVICE_WIN32,
					SERVICE_STATE_ALL,
					buffer,
					bytesNeeded,
					&bytesNeeded,
					&servicesReturned,
					&resumeHandle,
					NULL)) {
					ENUM_SERVICE_STATUS_PROCESSW* services = (ENUM_SERVICE_STATUS_PROCESSW*)buffer;
					for (DWORD i = 0; i < servicesReturned; i++) {
						for (const auto& vmService : vmServices) {
							if (_wcsicmp(services[i].lpServiceName, vmService) == 0) {
								delete[] buffer;
								CloseServiceHandle(scm);
								return true;
							}
						}
					}
				}
				delete[] buffer;
			}
			CloseServiceHandle(scm);
		}
		return false;
	}

	static bool CheckVMRegistry() {
		const wchar_t* vmRegistryKeys[] = {
			L"SOFTWARE\\Oracle\\VirtualBox Guest Additions",
			L"SOFTWARE\\VMware, Inc.\\VMware Tools",
			L"SOFTWARE\\Parallels\\Parallels Tools",
			L"SYSTEM\\ControlSet001\\Services\\VBoxGuest",
			L"SYSTEM\\ControlSet001\\Services\\VBoxMouse",
			L"SYSTEM\\ControlSet001\\Services\\VBoxService",
			L"SYSTEM\\ControlSet001\\Services\\vmci",
			L"SYSTEM\\ControlSet001\\Services\\vmhgfs",
			L"SYSTEM\\ControlSet001\\Services\\vmmouse",
			L"SYSTEM\\ControlSet001\\Services\\vmrawdsk",
			L"SYSTEM\\ControlSet001\\Services\\VMTools",
			L"SYSTEM\\ControlSet001\\Services\\VMMEMCTL",
			L"SYSTEM\\ControlSet001\\Services\\vmware"
		};

		for (const auto& key : vmRegistryKeys) {
			HKEY hKey;
			if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
				RegCloseKey(hKey);
				return true;
			}
		}
		return false;
	}

	bool invalidTSC()
	{
		UINT64 timestampOne{ __rdtsc() };
		UINT64 timestampTwo{ __rdtsc() };
		return ((timestampTwo - timestampOne) > 500);
	}

	bool cpu_known_vm_vendors()
	{
		int cpuInfo[4];
		__cpuid(cpuInfo, 1);

		if (!(cpuInfo[2] & (1 << 31)))
			return false;

		const auto queryVendorIdMagic{ 0x40000000 };
		constexpr int vendorIdLength{ 13 };
		char hyperVendorId[vendorIdLength];

		__cpuid(cpuInfo, queryVendorIdMagic);
		memcpy(hyperVendorId + 0, &cpuInfo[1], 4);
		memcpy(hyperVendorId + 4, &cpuInfo[2], 4);
		memcpy(hyperVendorId + 8, &cpuInfo[3], 4);
		hyperVendorId[12] = '\0';

		static const char* vendors[]{
			"KVMKVMKVM\0\0\0",
			"Microsoft Hv",
			"VMwareVMware",
			"XenVMMXenVMM",
			"prl hyperv  ",
			"VBoxVBoxVBox",
			"AMDisbetter!",
			"AuthenticAMD",
			"GenuineIntel",
			"VIA VIA VIA",
			"bhyve bhyve",
			"KVMKVMKVM  ",
			"TCGTCGTCGTCG",
			" lrpepyh  vr",
			"ACRNACRNACRN",
			" QNXQVMBSQG ",
		};

		for (const auto& vendor : vendors)
		{
			if (!std::memcmp(vendor, hyperVendorId, vendorIdLength))
				return true;
		}

		return false;
	}

	static bool CheckVMMemory() {
		MEMORYSTATUSEX memInfo;
		memInfo.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&memInfo);

		// Most VMs have suspiciously round numbers for total physical memory
		DWORD64 totalPhysMB = memInfo.ullTotalPhys / (1024 * 1024);
		if (totalPhysMB == 512 || totalPhysMB == 1024 || totalPhysMB == 2048 || 
			totalPhysMB == 4096 || totalPhysMB == 8192) {
			return true;
		}

		return false;
	}

	static bool CheckVMDevices() {
		const wchar_t* vmDevices[] = {
			L"\\Device\\VBoxGuest",
			L"\\Device\\VBoxMouse",
			L"\\Device\\VBoxVideo",
			L"\\Device\\Vmware",
			L"\\Device\\vmci"
		};

		for (const auto& device : vmDevices) {
			UNICODE_STRING deviceName;
			RtlInitUnicodeString(&deviceName, device);
			
			OBJECT_ATTRIBUTES objAttr;
			InitializeObjectAttributes(&objAttr, &deviceName, OBJ_CASE_INSENSITIVE, NULL, NULL);
			
			HANDLE deviceHandle;
			IO_STATUS_BLOCK ioStatusBlock;
			
			if (NT_SUCCESS(NtCreateFile(&deviceHandle,
				GENERIC_READ,
				&objAttr,
				&ioStatusBlock,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				FILE_OPEN,
				FILE_NON_DIRECTORY_FILE,
				NULL,
				0))) {
				CloseHandle(deviceHandle);
				return true;
			}
		}
		return false;
	}

	bool isVM() {
		int detectionCount = 0;

		if (invalidTSC()) detectionCount++;
		if (cpu_known_vm_vendors()) detectionCount++;
		if (CheckVMDrivers()) detectionCount++;
		if (CheckVMServices()) detectionCount++;
		if (CheckVMRegistry()) detectionCount++;
		if (CheckVMMemory()) detectionCount++;
		if (CheckVMDevices()) detectionCount++;

		// If 3 or more detection methods indicate a VM
		if (detectionCount >= 3) {
			MessageBoxA(0, "Virtual Machines Are Not Allowed!", "Fatal Error", MB_ICONERROR);
			std::exit(0);
		}

		return false;
	}
}

inline bool IsInsideVM() {
	int cpuInfo[4] = {};
	__cpuid(cpuInfo, 1);
	
	// Check if running in a Virtual Machine
	if (cpuInfo[2] & (1 << 31)) {
		return true;  // Hypervisor bit is set
	}

	// Additional VM detection methods
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	
	// Check for common VM memory sizes
	MEMORYSTATUSEX memoryStatus;
	memoryStatus.dwLength = sizeof(memoryStatus);
	GlobalMemoryStatusEx(&memoryStatus);
	
	// VM typically have round number memory sizes
	if (memoryStatus.ullTotalPhys % (1024 * 1024 * 1024) == 0) {
		return true;
	}

	// Check for common VM process names
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32W processEntry;
		processEntry.dwSize = sizeof(processEntry);
		
		if (Process32FirstW(snapshot, &processEntry)) {
			do {
				const wchar_t* processName = processEntry.szExeFile;
				if (wcsstr(processName, L"vmware") ||
					wcsstr(processName, L"vbox") ||
					wcsstr(processName, L"qemu") ||
					wcsstr(processName, L"virtual")) {
					CloseHandle(snapshot);
					return true;
				}
			} while (Process32NextW(snapshot, &processEntry));
		}
		CloseHandle(snapshot);
	}

	return false;
}

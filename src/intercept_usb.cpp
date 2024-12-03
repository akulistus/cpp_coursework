#include "intercept_usp.hpp"
#include <iostream>
#include <setupapi.h>
#include <tchar.h>
#include <thread>
#include <atomic>

#pragma comment(lib, "setupapi.lib")

// GUID for COM ports
const GUID GUID_DEVINTERFACE_COMPORT = { 0x86E0D1E0L, 0x8089, 0x11D0, { 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73 } };

// Global variables
HANDLE g_hCOM = INVALID_HANDLE_VALUE;
std::vector<std::string> g_messages;
std::atomic<bool> g_reading(false);
std::thread g_readingThread;

// Detect and connect to the Arduino device
void DetectAndConnectToArduino() {
    if (g_hCOM != INVALID_HANDLE_VALUE) {
        return; // Already connected
    }

    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get device information set.\n";
        return;
    }

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = {};
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    DWORD deviceIndex = 0;
    while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_COMPORT, deviceIndex, &deviceInterfaceData)) {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);
        if (requiredSize == 0) break;

        auto detailDataBuffer = std::make_unique<BYTE[]>(requiredSize);
        auto deviceInterfaceDetailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(detailDataBuffer.get());
        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        SP_DEVINFO_DATA devInfoData = {};
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData, requiredSize, NULL, &devInfoData)) {
            std::wcout << L"Detected device path: " << deviceInterfaceDetailData->DevicePath << std::endl;

            HANDLE hCOM = CreateFile(deviceInterfaceDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hCOM != INVALID_HANDLE_VALUE) {
                DCB dcb = {};
                dcb.DCBlength = sizeof(DCB);
                if (GetCommState(hCOM, &dcb)) {
                    dcb.BaudRate = CBR_9600;
                    dcb.ByteSize = 8;
                    dcb.Parity = NOPARITY;
                    dcb.StopBits = ONESTOPBIT;
                    if (SetCommState(hCOM, &dcb)) {
                        g_hCOM = hCOM;
                        std::wcout << L"Connected to Arduino on " << deviceInterfaceDetailData->DevicePath << std::endl;
                        StartReadingArduino();  // Start reading in a background thread
                        break;
                    }
                }
                CloseHandle(hCOM);
            } else {
                std::cerr << "Failed to open COM port. Error: " << GetLastError() << std::endl;
            }
        }
        deviceIndex++;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
}

// Non-blocking serial read (running in a separate thread)
void ReadArduinoMessages() {
    if (g_hCOM == INVALID_HANDLE_VALUE) {
        return;
    }

    char buffer[256];
    DWORD bytesRead = 0;
    while (g_reading.load()) {
        if (ReadFile(g_hCOM, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                g_messages.push_back(buffer);
            }
        }
    }
}

// Start the reading process in a new thread
void StartReadingArduino() {
    g_reading.store(true);
    g_readingThread = std::thread(ReadArduinoMessages);
}

// Stop reading from Arduino
void StopReadingArduino() {
    g_reading.store(false);
    if (g_readingThread.joinable()) {
        g_readingThread.join();
    }
}

// Accessor for Arduino messages
const std::vector<std::string>& GetArduinoMessages() {
    return g_messages;
}

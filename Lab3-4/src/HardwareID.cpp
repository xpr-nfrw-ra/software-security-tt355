#include "HardwareID.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

// Windows-ի header-ների ճիշտ հերթականություն
#ifdef _WIN32
// WIN32_LEAN_AND_MEAN-ը բացառում է Windows.h-ի ավելորդ մասերը
#define WIN32_LEAN_AND_MEAN
// winsock2.h պետք է ներառել windows.h-ից առաջ
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <intrin.h>
#include <wbemidl.h>
// Պետք չէ ներառել ws2def.h-ն առանձին, այն արդեն ներառված է
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

std::string HardwareID::combineAndHash(const std::string& data) {
    unsigned long hash = 5381;
    for (char c : data) {
        hash = ((hash << 5) + hash) + c;
    }

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

#ifdef _WIN32
std::string HardwareID::getCpuIdWindows() {
    int cpuInfo[4] = { -1 };
    char cpuIdStr[17] = { 0 };

    __cpuid(cpuInfo, 0);
    snprintf(cpuIdStr, sizeof(cpuIdStr), "%08X%08X", cpuInfo[3], cpuInfo[0]);

    __cpuid(cpuInfo, 1);
    char cpuIdStr2[17] = { 0 };
    snprintf(cpuIdStr2, sizeof(cpuIdStr2), "%08X%08X", cpuInfo[3], cpuInfo[0]);

    return std::string(cpuIdStr) + std::string(cpuIdStr2);
}

std::string HardwareID::getMacAddressWindows() {
    // Սկզբում փորձել GetAdaptersAddresses (Windows Vista+)
    ULONG bufferSize = 0;
    ULONG result = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &bufferSize);

    if (result == ERROR_BUFFER_OVERFLOW && bufferSize > 0) {
        std::vector<BYTE> buffer(bufferSize);
        PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

        result = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &bufferSize);
        if (result == NO_ERROR) {
            while (pAddresses) {
                if (pAddresses->PhysicalAddressLength >= 6 &&
                    pAddresses->OperStatus == IfOperStatusUp) {

                    // Ստուգել, որ հասցեն վավեր է (ոչ բոլորը 0)
                    bool valid = false;
                    for (ULONG i = 0; i < pAddresses->PhysicalAddressLength; i++) {
                        if (pAddresses->PhysicalAddress[i] != 0) {
                            valid = true;
                            break;
                        }
                    }

                    if (valid) {
                        std::stringstream ss;
                        for (ULONG i = 0; i < pAddresses->PhysicalAddressLength; i++) {
                            ss << std::hex << std::setfill('0') << std::setw(2)
                                << (int)pAddresses->PhysicalAddress[i];
                            if (i < pAddresses->PhysicalAddressLength - 1) ss << ":";
                        }
                        return ss.str();
                    }
                }
                pAddresses = pAddresses->Next;
            }
        }
    }

    // Եթե չհաջողվեց, փորձել GetAdaptersInfo (հին Windows-ի համար)
    DWORD adapterInfoSize = 0;
    GetAdaptersInfo(NULL, &adapterInfoSize);

    if (adapterInfoSize > 0) {
        std::vector<BYTE> buffer(adapterInfoSize);
        PIP_ADAPTER_INFO pAdapter = reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());

        if (GetAdaptersInfo(pAdapter, &adapterInfoSize) == ERROR_SUCCESS) {
            while (pAdapter) {
                if (pAdapter->AddressLength >= 6) {
                    // Ստուգել, որ հասցեն վավեր է
                    bool valid = false;
                    for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                        if (pAdapter->Address[i] != 0) {
                            valid = true;
                            break;
                        }
                    }

                    if (valid) {
                        std::stringstream ss;
                        for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                            ss << std::hex << std::setfill('0') << std::setw(2)
                                << (int)pAdapter->Address[i];
                            if (i < pAdapter->AddressLength - 1) ss << ":";
                        }
                        return ss.str();
                    }
                }
                pAdapter = pAdapter->Next;
            }
        }
    }

    return "00-00-00-00-00-00";
}

std::string HardwareID::getMotherboardIdWindows() {
    HRESULT hres;
    std::string result = "UNKNOWN";

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return "UNKNOWN";

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);

    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        return "UNKNOWN";
    }

    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(L"ROOT\\CIMV2", NULL, NULL, 0,
        NULL, 0, 0, &pSvc);

    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return "UNKNOWN";
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(L"WQL",
        L"SELECT * FROM Win32_BaseBoard",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator);

    if (SUCCEEDED(hres)) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;

        if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
            VARIANT vtProp;
            if (SUCCEEDED(pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0))) {
                if (vtProp.vt == VT_BSTR && vtProp.bstrVal) {
                    char buffer[256] = { 0 };
                    size_t converted;
                    wcstombs_s(&converted, buffer, sizeof(buffer) - 1, vtProp.bstrVal, _TRUNCATE);
                    result = buffer;
                }
                VariantClear(&vtProp);
            }
            pclsObj->Release();
        }
        pEnumerator->Release();
    }

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return result;
}
#endif

HardwareID::HardwareID() {
#ifdef _WIN32
    cpuId = getCpuIdWindows();
    motherboardId = getMotherboardIdWindows();
    macAddress = getMacAddressWindows();
#else
    cpuId = "LINUX-CPU-ID";
    motherboardId = "LINUX-MB-ID";
    macAddress = "LINUX-MAC";
#endif
}

std::string HardwareID::getMachineID() {
    std::string combined = cpuId + "|" + motherboardId + "|" + macAddress;
    return combineAndHash(combined);
}

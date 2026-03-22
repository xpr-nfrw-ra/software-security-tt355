#pragma once
#ifndef HARDWAREID_H
#define HARDWAREID_H

#include <string>

/**
 * HardwareID դաս - ապարատային եզակի նույնականացուցիչի ստացման համար
 * Հիմնված է CPU ID-ի, մայրական տախտակի սերիայի և MAC հասցեի վրա
 */
class HardwareID {
private:
    std::string cpuId;
    std::string motherboardId;
    std::string macAddress;

    // Տվյալների միավորում և հեշավորում
    std::string combineAndHash(const std::string& data);

#ifdef _WIN32
    // CPU ID-ի ստացում Windows-ում
    std::string getCpuIdWindows();

    // MAC հասցեի ստացում Windows-ում
    std::string getMacAddressWindows();

    // Մայրական տախտակի ID-ի ստացում WMI-ի միջոցով
    std::string getMotherboardIdWindows();
#endif

public:
    // Կոնստրուկտոր - հավաքում է ապարատային տվյալները
    HardwareID();

    // Սարքավորման եզակի ID-ի ստացում
    std::string getMachineID();

    // Առանձին բաղադրիչների ստացում (դեբագի համար)
    std::string getCpuId() { return cpuId; }
    std::string getMotherboardId() { return motherboardId; }
    std::string getMacAddress() { return macAddress; }
};

#endif

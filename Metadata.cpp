//
// Created by Stephen F on 13/07/23.
//

#include "Metadata.h"

#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

Metadata::Metadata(int duration) : mDuration(duration)
{

}

std::string Metadata::Platform() const
{
    std::ifstream deviceProperties("/etc/device.properties");
    if (!deviceProperties) {
        return "Unknown";
    }

    std::string line;
    char platform[512];

    while (std::getline(deviceProperties, line)) {
        if (sscanf(line.c_str(), "FRIENDLY_ID=\"%s\"", platform) != 0) {
            return std::string(platform);
        }
    }

    return "Unknown";
}

std::string Metadata::Image() const
{
    std::ifstream versionFile("/version.txt");
    if (!versionFile) {
        return "Unknown";
    }

    std::string line;
    char image[512];

    while (std::getline(versionFile, line)) {
        if (sscanf(line.c_str(), "imagename:%256s", image) != 0) {
            return std::string(image);
        }
    }

    return "Unknown";
}

std::string Metadata::Mac() const
{
    std::ifstream macFile("/sys/class/net/eth0/address");
    if (!macFile) {
        return "Unknown";
    }

    std::stringstream buffer;
    buffer << macFile.rdbuf();

    return buffer.str();
}

std::string Metadata::ReportTimestamp() const
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::stringstream buffer;
    buffer << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    return buffer.str();
}

int Metadata::Duration() const
{
    return mDuration;
}

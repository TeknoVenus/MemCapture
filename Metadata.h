//
// Created by Stephen F on 13/07/23.
//

#pragma once

#include <string>

class Metadata
{
public:
    Metadata(int duration);

    std::string Platform() const;
    std::string Image() const;
    std::string Mac() const;
    std::string ReportTimestamp() const;
    int Duration() const;

private:
    const int mDuration;
};

//
// Created by Stephen F on 24/11/22.
//

#ifndef MEMCAPTURE_UTILS_H
#define MEMCAPTURE_UTILS_H

#include <tabulate/table.hpp>
#include <string>
#include "Measurement.h"

class Utils
{
public:
    inline static void PrintTable(tabulate::Table &table)
    {
        table.format()
                .padding(0)
                .border_top(" ")
                .border_bottom(" ")
                .border_left(" ")
                .border_right(" ")
                .hide_border_bottom()
                .hide_border_top()
                .corner(" ");

        table.print(std::cout);
    }
};

#endif //MEMCAPTURE_UTILS_H

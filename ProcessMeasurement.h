//
// Created by Stephen F on 13/07/23.
//

#pragma once

#include "Process.h"
#include "Measurement.h"

struct processMeasurement
{
    processMeasurement(Process _process, Measurement _pss, Measurement _rss, Measurement _uss)
            : ProcessInfo(std::move(_process)),
              Pss(std::move(_pss)),
              Rss(std::move(_rss)),
              Uss(std::move(_uss))
    {
    }

    Process ProcessInfo;
    Measurement Pss;
    Measurement Rss;
    Measurement Uss;
};
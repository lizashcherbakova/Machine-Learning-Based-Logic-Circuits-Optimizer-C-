//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/model/gate.h"
#include "util.h"

#include <unordered_map>
#include <iostream>
#include <algorithm>

namespace eda::gate::optimizer {

    // Структура для хранения параметров схемы
    struct PlainParameters {
        int numInputs;
        int numOutputs;
        int numGates;
        int numAnds;
        int numInvertedEdges;
        int longestPath;
    };

    // Класс для сбора параметров схемы
    class PlainParametersCollector {
    private:
        GNet *net;
        PlainParameters parameters;

        void collectInputs();

        void collectOutputs();

        void collectGates();

        void collectAndGates();

        void collectInvertedEdges();

        void findLongestPath();

    public:
        explicit PlainParametersCollector(GNet *_net);

        void collect();

        void printParameters(std::ostream &stream) const;

        PlainParameters getParameters() const;
    };

} // namespace eda::gate::optimizer
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#ifndef SYNTHESIS_SCENARIO_OPTIMIZER_H
#define SYNTHESIS_SCENARIO_OPTIMIZER_H

#include "gate/parser/gate_verilog.h"
#include "gate/parser/graphml.h"
#include "gate/optimizer/plain_parameters_collector.h"

#include <lorina/common.hpp>
#include <lorina/diagnostics.hpp>
#include <lorina/verilog.hpp>

#include <Python.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace eda::gate::optimizer {

    class SynthesisScenarioOptimizer {
    public:
        SynthesisScenarioOptimizer();
        ~SynthesisScenarioOptimizer();

        void readGraphML(const std::string &filename);
        void readVerilog(const std::string &filename);
        void collectParameters();
        void readSynthesisScenarios(const std::string &filename);
        void evaluateAndSelectBestScenarios(int k, const std::string &outputFile);

    private:
        eda::gate::model::GNet *net;
        PlainParameters parameters;
        std::unordered_map<std::string, std::string> scenarios;
        std::vector<std::pair<std::string, double>> evaluateScenarios();

        void initializePython();
        void finalizePython();
        double predictScenarioQuality(const std::string &scenario);
    };

} // namespace eda::gate::optimizer

#endif // SYNTHESIS_SCENARIO_OPTIMIZER_H

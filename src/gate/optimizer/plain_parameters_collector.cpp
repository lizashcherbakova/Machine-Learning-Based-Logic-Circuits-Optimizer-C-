//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "plain_parameters_collector.h"

namespace eda::gate::optimizer {

    PlainParametersCollector::PlainParametersCollector(GNet *_net) : net(_net) {}

    void PlainParametersCollector::collectInputs() {
        parameters.numInputs = net->nSourceLinks();
    }

    void PlainParametersCollector::collectOutputs() {
        parameters.numOutputs = net->nTargetLinks();
    }

    void PlainParametersCollector::collectGates() {
        parameters.numGates = net->nGates();
    }

    void PlainParametersCollector::collectAndGates() {
        parameters.numAnds = 0;
        for (const auto &link : net->gates()) {
            if (link->isAnd()) {
                parameters.numAnds++;
            }
        }
    }

    void PlainParametersCollector::collectInvertedEdges() {
        parameters.numInvertedEdges = 0;
        for (const auto &link : net->gates()) {
            if (link->isNot()) {
                parameters.numInvertedEdges++;
            }
        }
    }

    void PlainParametersCollector::findLongestPath() {
        std::unordered_map<Gate::Id, int> distances;

        for (const auto &gate : net->gates()) {
            distances[gate->id()] = 0;
        }

        auto updateDistance = [&](const Gate::Id &from, const Gate::Id &to) {
            distances[to] = std::max(distances[to], distances[from] + 1);
        };

        for (const auto &gate : net->gates()) {
            for (const auto &input : gate->inputs()) {
                updateDistance(input.node(), gate->id());
            }
        }

        parameters.longestPath = 0;
        for (const auto &[gateId, distance] : distances) {
            parameters.longestPath = std::max(parameters.longestPath, distance);
        }
    }

    void PlainParametersCollector::collect() {
        collectInputs();
        collectOutputs();
        collectGates();
        collectAndGates();
        collectInvertedEdges();
        findLongestPath();
    }

    void PlainParametersCollector::printParameters(std::ostream &stream) const {
        stream << "Number of Inputs: " << parameters.numInputs << "\n";
        stream << "Number of Outputs: " << parameters.numOutputs << "\n";
        stream << "Number of Gates: " << parameters.numGates << "\n";
        stream << "Number of And Gates: " << parameters.numAnds << "\n";
        stream << "Number of Inverted Edges: " << parameters.numInvertedEdges << "\n";
        stream << "Longest Path: " << parameters.longestPath << "\n";
    }

    PlainParameters PlainParametersCollector::getParameters() const {
        return parameters;
    }

} // namespace eda::gate::optimizer


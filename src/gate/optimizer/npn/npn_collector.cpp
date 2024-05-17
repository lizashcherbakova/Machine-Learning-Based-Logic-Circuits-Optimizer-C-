
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/npn/npn_collector.h"

namespace eda::gate::optimizer {

    void calculateAverageAndDeviation(const std::vector<NPNStats> &stats, SumStruct &sumStruct) {
        assert(!stats.empty());
        double sumMax = 0.0;
        double sumMin = 0.0;
        int n = static_cast<int>(stats.size());

        // Calculate the sum of the elements.
        for (const auto &data: stats) {
            sumMax += data.maxHeight;
            sumMin += data.minHeight;
        }

        // Calculate the mean.
        double meanMax = sumMax / n;
        double meanMin = sumMin / n;

        sumStruct.maxHeightA = meanMax;
        sumStruct.minHeightA = meanMin;

        // Calculate the squared differences from the mean.
        double squaredDiffSumMax = 0.0;
        double squaredDiffSumMin = 0.0;
        for (const auto &data: stats) {
            squaredDiffSumMax += (data.maxHeight - meanMax) * (data.maxHeight - meanMax);
            squaredDiffSumMin += (data.minHeight - meanMin) * (data.minHeight - meanMin);
        }

        // Calculate the standard deviation.
        sumStruct.minHeightD = std::sqrt(squaredDiffSumMin / n);
        sumStruct.maxHeightD = std::sqrt(squaredDiffSumMax / n);
    }

    bool
    NPNCollector::fillNPNStats(const Cut &cut, size_t cutSize, GateId gateId, NPNStats &toFill) {
        ConeVisitor coneVisitor(cut, gateId);
        Walker walker(net, &coneVisitor);
        walker.walk(cut, gateId, false);

        // Make binding.
        BoundGNet boundGNet;

        const auto &cutConeMap = coneVisitor.getResultMatch();
        const auto &resultCut = coneVisitor.getResultCutOldGates();

        if (cut.size() != resultCut.size()) {
            delete coneVisitor.getGNet();
            return false;
        }
        if (collectHeight) {
            getHeights(gateId, toFill.maxHeight, toFill.minHeight, cut);
        }
        for (const auto &gate: resultCut) {
            boundGNet.inputBindings.push_back(cutConeMap.find(gate)->second);
        }
        boundGNet.net = std::shared_ptr<GNet>(coneVisitor.getGNet());
        TruthTable table = TruthTable::build(boundGNet);
        auto npnClass = truthTableToNPN(table);

        toFill.npnClass = npnClass._bits;
        toFill.cut = cut;
        return true;
    }

    kitty::static_truth_table<6>
    NPNCollector::truthTableToNPN(const TruthTable &table) {
        kitty::static_truth_table<6> kt;
        kt._bits = table.raw();

        const auto [tt, inputNegations, outputNegation] = kitty::exact_npn_canonization(
                kt);

        return tt;
    }

    void NPNCollector::addNPNStat(const GateId &gateId, const NPNStats &stat) {
        auto &gateStat = gateStatsMap[gateId];
        gateStat.npnClassInfo.push_back(stat);
        ++gateStat.numberOfCuts;
        SumStruct &data = npnStatistics[stat.npnClass];
        data.count.emplace_back(stat);
    }

    void NPNCollector::process(size_t cutSize, size_t maxCutsNumber) {
        CutStorage storage = findCuts(net, cutSize, maxCutsNumber, false);

        std::cout << "Cuts found" << std::endl;

        for (auto &[gateId, cs]: storage.cuts) {
            for (const auto &c: cs) {
                if (c.size() != cutSize) {
                    continue;
                }
                NPNStats npnStats;
                if (fillNPNStats(c, cutSize, gateId, npnStats)) {
                    addNPNStat(gateId, npnStats);
                }
            }
        }

        for (auto &[npn, stats]: npnStatistics) {
            calculateAverageAndDeviation(stats.count, stats);
        }
    }

    void NPNCollector::printGateStatistics(std::ostream &stream) const {
        for (const auto &[gateId, stats]: gateStatsMap) {
            // Format and print the Gate ID and its corresponding function name
            stream << "Gate : " << Gate::get(gateId)->func() << gateId << "\n";

            // Print the number of cuts for this gate
            stream << "\tNumber of Cuts: " << stats.numberOfCuts << "\n";

            // Print NPN stats for this gate
            for (const auto &npnStats: stats.npnClassInfo) {
                stream << "\tNPN Class: " << npnStats.npnClass
                       << ", Max height: " << npnStats.maxHeight
                       << ", Min height: " << npnStats.minHeight << "\n";
            }

            stream << "\n";
        }
    }

    void NPNCollector::printHistogramData(std::ostream &stream) const {
        stream << "NPN Class;Count;MaxHeightA;MaxHeightD;MinHeightA;MinHeightD\n";
        for (const auto &[npnClass, data]: npnStatistics) {
            stream << npnClass << ";" << data.count.size() << ";" << data.maxHeightA << ";" << data.maxHeightD << ";"
                   << data.minHeightA << ";" << data.minHeightD << "\n";
        }
    }

    std::unordered_map<uint64_t, std::vector<std::shared_ptr<GNet>>>
    NPNCollector::getEssentialCones(int topNumber, int conesNumber) const {
        // Selecting first topNumber NPN classes.
        topNumber = std::min(topNumber, static_cast<int>(npnStatistics.size()));
        std::vector<std::pair<uint64_t, SumStruct>> popularNPN;
        popularNPN.reserve(npnStatistics.size());
        for (const auto &pair: npnStatistics) {
            popularNPN.emplace_back(pair);
        }
        std::partial_sort(popularNPN.begin(), popularNPN.begin() + topNumber,
                          popularNPN.end(),
                          [](const std::pair<uint64_t, SumStruct> &a,
                             const std::pair<uint64_t, SumStruct> &b) {
                              return a.second.count.size() > b.second.count.size(); // Sort in descending order
                          });

        std::unordered_map<uint64_t, std::vector<std::shared_ptr<GNet>>> rez;
        for (int i = 0; i < topNumber; ++i) {
            rez.insert({popularNPN[i].first, {}});
        }
        for (const auto &[gate, gateStat]: gateStatsMap) {
            for (const auto &npnStat: gateStat.npnClassInfo) {
                auto found = rez.find(npnStat.npnClass);
                if (found != rez.end() &&
                    static_cast<int>(found->second.size()) < conesNumber) {
                    Order order;
                    order.insert(order.end(), npnStat.cut.begin(), npnStat.cut.end());
                    auto binding = extractCone(net, gate, npnStat.cut, order);
                    found->second.push_back(binding.net);
                }
            }
        }
        return rez;
    }

} // namespace eda::gate::optimizer
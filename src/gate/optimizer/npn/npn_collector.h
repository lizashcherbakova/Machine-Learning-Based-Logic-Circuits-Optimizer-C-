//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"
#include "gate/optimizer/truthtable.h"
#include "gate/optimizer/util.h"

#include <kitty/npn.hpp>

#include <cmath>

namespace eda::gate::optimizer {

  struct NPNStats {
    uint64_t npnClass;
    int minHeight, maxHeight; // TODO add max and min height.
    Cut cut;
  };

  struct GateStats {
    GateId gateId;
    int numberOfCuts;
    std::vector<NPNStats> npnClassInfo;
  };

  struct SumStruct {
    std::vector<NPNStats> count;
    double maxHeightA = -1, maxHeightD = -1, minHeightA = -1, minHeightD = -1;
  };

  // Main class to collect and manage statistics
  class NPNCollector {
  private:
    bool collectHeight = false;
    GNet *net;
    std::unordered_map<GateId, GateStats> gateStatsMap;
    std::unordered_map<uint64_t, SumStruct> npnStatistics;

    bool fillNPNStats(const Cut &cut, size_t cutSize, GateId gateId,
                      NPNStats &npnStats);

    kitty::static_truth_table<6> truthTableToNPN(const TruthTable &table);

  public:
    explicit NPNCollector(GNet *_net) : net(_net) {}

    void addNPNStat(const GateId &gateId, const NPNStats &stat);

    void process(size_t cutSize, size_t maxCutsNumber);

    void printGateStatistics(std::ostream &stream) const;

    void printHistogramData(std::ostream &stream) const;

    /*!
    * \brief Retrieves the top NPN classes and their associated cones up to a specified number.
    *
    * This function first identifies the most popular NPN classes based on their statistics.
    * It then iterates over the gate statistics map to find all cones associated with each of these popular NPN classes.
    * The number of cones collected for each NPN class is limited by the conesNumber parameter.
    *
    * \param topNumber The number of top NPN classes to consider.
    * \param conesNumber The maximum number of cones to collect for each NPN class.
    * \return A map where each key is an NPN class and each value is a vector of shared pointers to the associated GNet objects.
    */
    std::unordered_map<uint64_t, std::vector<std::shared_ptr<GNet>>>
    getEssentialCones(int topNumber, int conesNumber) const;
  };

} // namespace eda::gate::optimizer
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/optimizer/optimizer.h"
#include "gate/optimizer/util.h"
#include "gate/parser/gate_verilog.h"
#include "gate/printer/dot.h"
#include "gate/optimizer/optimizer_util.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>
#include <chrono>

using namespace eda::gate::parser::verilog;

namespace eda::gate::optimizer {

  int checkCutStorage(const CutStorage &storage) {
    int cutsCounter = 0;
    for (auto &[v, cs]: storage.cuts) {
      for (const auto &c: cs) {
        GateId failed;

        if (!isCut(v, c, failed)) {
          std::cerr << "Wrong cut for v " << v << "; failed " << failed
                    << "\n";
          assert(false);
        }
        ++cutsCounter;
      }
    }
    std::cout << "cuts counted " << cutsCounter << "\n";
    return cutsCounter;
  }

  int findCutsTest(GNet *net, int cutSize = 4) {
    CutStorage storage = findCuts(net, cutSize, 100);
    std::cout << "gates number " << net->nGates() << "\n";
    return checkCutStorage(storage);
  }

  GNet *getLorinaGnet(const std::string &name) {
    const std::filesystem::path subCatalog =
        std::filesystem::path("test") / "data" / "gate" / "parser" /
        "verilog";
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path filename = homePath / subCatalog / name;
    auto gNet = eda::gate::parser::verilog::getNet(filename, name);
    return gNet;
  }

  TEST(FindCutTest, UpgradedFindCuts_adder) {
    auto gNet = getLorinaGnet("adder.v");
    findCutsTest(gNet);
    delete gNet;
  }

  TEST(FindCutTest, UpgradedFindCuts_c17) {
    auto gNet = getLorinaGnet("c17.v");
    findCutsTest(gNet);
    delete gNet;
  }

  TEST(FindCutTest, UpgradedFindCuts_GNet1) {
    GNet gNet;
    gnet1(gNet);
    int cutsNumber = findCutsTest(&gNet);
    ASSERT_EQ(cutsNumber, 18);
  }

  TEST(FindCutTest, UpgradedFindCuts_diamondShapedGraph) {
    GNet gNet;
    diamondShapedGraph(gNet);
    findCutsTest(&gNet);
  }

  std::pair<int, double> calculateCutsMetrics(const CutStorage &storage) {
    int totalCuts = 0;
    int gateCount = 0;

    for (auto &[v, cs]: storage.cuts) {
      totalCuts += cs.size();
      gateCount++;
    }

    double averageCutsPerGate = (gateCount == 0) ? 0.0 :
                                static_cast<double>(totalCuts) / gateCount;

    return {totalCuts, averageCutsPerGate};
  }

  std::pair<Cuts, double>
  garbageCutsNumber(const CutStorage &storage, const GNet *gNet) {
    Cuts garbage;

    for (auto &[v, cs]: storage.cuts) {
      for (auto &c: cs) {
        ConeVisitor coneVisitor(c, v);
        Walker walker(gNet, &coneVisitor);
        walker.walk(c, v, false);
        if (coneVisitor.getResultCutOldGates().size() < c.size()) {
          garbage.emplace(c);
        }
      }
    }
    double averageCutsPerGate = (storage.cuts.empty()) ? 0.0 :
                                static_cast<double>(garbage.size()) /
                                static_cast<double>(storage.cuts.size());
    return {garbage, averageCutsPerGate};
  }

  void
  compareCutStorages(const CutStorage &verifiable, const CutStorage &excessive,
                     const Cuts &garbage) {
    for (const auto &[rootVertexB, cutsB]: excessive.cuts) {
      if (verifiable.cuts.find(rootVertexB) == verifiable.cuts.end()) {
        for (const auto &cut: cutsB) {
          if (garbage.find(cut) == garbage.end()) {
            std::cout << "Root Vertex: " << rootVertexB << ", Cut: { ";
            for (const auto &elem: cut) {
              std::cout << elem << " ";
            }
            std::cout << "}" << std::endl;
          }
        }
      } else {
        const auto &cutsA = verifiable.cuts.at(rootVertexB);
        for (const auto &cut: cutsB) {
          if (cutsA.find(cut) == cutsA.end() && garbage.find(cut) ==
                                                garbage.end()) {
            std::cout << "Root Vertex: " << rootVertexB << ", Cut: { ";
            for (const auto &elem: cut) {
              std::cout << elem << " ";
            }
            std::cout << "}" << std::endl;
          }
        }
      }
    }
  }

  void findCutsCompare(const GNet *gNet, int cutSize, std::string name,
                       bool print, bool garbageOutput = true) {
    if (print) {
      Dot dot(gNet);
      dot.print(name + ".dot");
    }

    std::cout << "gates number = " << gNet->nGates() << std::endl;

    // Start timing for the old method
    auto startOld = std::chrono::high_resolution_clock::now();

    CutStorage storageOld = findCuts(gNet, cutSize, CutsFindVisitor::ALL_CUTS,
                                     true);

    // End timing for the old method
    auto endOld = std::chrono::high_resolution_clock::now();
    auto durationOld = std::chrono::duration_cast<std::chrono::milliseconds>(
        endOld - startOld);

    auto [totalCutsOld, avgCutsPerGateOld] = calculateCutsMetrics(storageOld);
    std::cout << "Found old : totalCutsOld = " << totalCutsOld
              << " avgCutsPerGateOld = " << avgCutsPerGateOld << std::endl;
    std::cout << "Old method execution time: " << durationOld.count()
              << " milliseconds\n" << std::endl;

    // Start timing for the new method
    auto startNew = std::chrono::high_resolution_clock::now();

    CutStorage storageNew = findCuts(gNet, cutSize, CutsFindVisitor::ALL_CUTS,
                                     false);

    // End timing for the new method
    auto endNew = std::chrono::high_resolution_clock::now();
    auto durationNew = std::chrono::duration_cast<std::chrono::milliseconds>(
        endNew - startNew);

    auto [totalCutsNew, avgCutsPerGateNew] = calculateCutsMetrics(storageNew);
    std::cout << "Found new : totalCuts = " << totalCutsNew
              << " avgCutsPerGate = " << avgCutsPerGateNew << std::endl;
    std::cout << "New method execution time: " << durationNew.count()
              << " milliseconds\n" << std::endl;


    if (garbageOutput) {
      auto [garbageOld, avgCutsPerGOld] = garbageCutsNumber(storageOld, gNet);
      std::cout << "Garbage old : totalCutsOld = " << garbageOld.size()
                << " not garb:"
                << totalCutsOld - garbageOld.size()
                << " avgCutsPerGateOld = " << avgCutsPerGOld << std::endl;
      auto [garbageNew, avgCutsPerGNew] = garbageCutsNumber(storageNew, gNet);
      std::cout << "Garbage new : totalCutsOld = " << garbageNew.size()
                << " avgCutsPerGateOld = " << avgCutsPerGNew << std::endl;
      compareCutStorages(storageNew, storageOld, garbageOld);
    }
  }

  TEST(CutsCompareTest, adder) {
    GNet *gNet = getLorinaGnet("adder.v");
    findCutsCompare(gNet, 4, "adder", true);
    delete gNet;
  }

  TEST(CutsCompareTest, div) {
    GNet *gNet = getLorinaGnet("div.v");
    findCutsCompare(gNet, 4, "div", false, false);
    delete gNet;
  }

  TEST(CutsCompareTest, multiplier) {
    GNet *gNet = getLorinaGnet("multiplier.v");
    findCutsCompare(gNet, 4, "multiplier", false, false);
    delete gNet;
  }

} // namespace eda::gate::optimizer

//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/npn/npn_collector.h"
#include "gate/optimizer/optimizer_util.h"
#include "gate/model/examples.h"
#include "gate/parser/graphml.h"

#include "gtest/gtest.h"

#include <fstream>
#include <chrono>
#include <ctime>

namespace eda::gate::optimizer::npn {

  void printNetCones(const std::filesystem::path &path,
                     const std::unordered_map<uint64_t, std::vector<std::shared_ptr<GNet>>> &conesMap,
                     GNet *gNet = nullptr, const std::string &name = "") {

    if (!gNet) {
      auto out = createOutPath(path) / (name + ".dot");
      Dot dot(gNet);
      dot.print(out);
    }

    for (const auto &[npnClass, cones]: conesMap) {

      auto fullPath = path / std::to_string(npnClass);
      fullPath = createOutPath(fullPath);
      std::cout << fullPath << std::endl;

      for (size_t i = 0; i < cones.size(); ++i) {
        Dot dot(cones[i].get());
        dot.print(fullPath / ("cone" + std::to_string(i) + ".dot"));
      }
    }

  }

  NPNCollector
  npnStatistics(GNet *gNet, size_t cutSize, const std::string &name) {
    NPNCollector npn(gNet);
    npn.process(cutSize, CutsFindVisitor::ALL_CUTS);

    std::cout << "processed ";
    auto time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::cout << ctime(&time) << std::endl;

    auto fullPath = createOutPath("npnStatistics");
    std::string csv_filename = (fullPath / (name + ".csv"));

    // Open output CSV file stream.
    std::ofstream csv_file(fullPath / (name + ".csv"));

    if (csv_file.is_open()) {
      npn.printHistogramData(csv_file);
      csv_file.close();
    } else {
      std::cerr << "Unable to open file: " << csv_filename << "\n";
      npn.printHistogramData(std::cout);
    }
    return npn;
  }

    std::pair<NPNCollector, GNet *>
    graphMLNPNStatistics2(size_t cutSize, std::string filename, std::string scheme) {
        std::cout << filename << std::endl;
        auto time = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
        std::cout << ctime(&time) << std::endl;

        parser::graphml::GraphMLParser parser;
        auto gNet = parser.parse(filename);

        std::cout << gNet->nGates() << " parsed ";
        time = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
        std::cout << ctime(&time) << std::endl;

        NPNCollector npnCollector = npnStatistics(gNet, cutSize, scheme);
        return {npnCollector, gNet};
    }

  std::pair<NPNCollector, GNet *>
  graphMLNPNStatistics(size_t cutSize, const std::string &scheme,
                       std::string postfix = "_orig.bench.graphml") {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path subCatalog =
        std::filesystem::path("test") / "data" / "gate" / "parser" /
        "graphml";
    const std::filesystem::path prefixPathIn = homePath / subCatalog;
    std::string filename = (prefixPathIn / (scheme + postfix));

    std::cout << filename << std::endl;
    auto time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::cout << ctime(&time) << std::endl;

    parser::graphml::GraphMLParser parser;
    auto gNet = parser.parse(filename);

    std::cout << gNet->nGates() << " parsed ";
    time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::cout << ctime(&time) << std::endl;

    NPNCollector npnCollector = npnStatistics(gNet, cutSize, scheme);
    return {npnCollector, gNet};
  }

    std::pair<NPNCollector, GNet *>
    graphMLNPNStatistics(std::string &filename, std::string &scheme, size_t cutSize) {
        std::cout << filename << std::endl;
        auto time = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
        std::cout << ctime(&time) << std::endl;

        // parser::graphml::GraphMLParser2 parser;
        auto gNet = parser::graphml::GraphMLParser::parse(filename);

        std::cout << gNet->nGates() << " parsed ";
        time = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
        std::cout << ctime(&time) << std::endl;

        NPNCollector npnCollector = npnStatistics(gNet, cutSize, scheme);
        return {npnCollector, gNet};
    }

  TEST(NpnTest, gnet3) {
    GNet net;
    auto g = gnet3(net);
    auto npn = npnStatistics(&net, 4, "gnet3");
    npn.printGateStatistics(std::cout);
    printNetCones("gnet3", npn.getEssentialCones(10, 10), &net, "gnet3");
  }

  TEST(NpnTest, ethernetCone) {
    auto values = graphMLNPNStatistics(4, "ethernet");
    printNetCones("ethernet", values.first.getEssentialCones(10, 10),
                  values.second, "ethernet");
    delete values.second;
  }

  TEST(NpnTest, sha256Cone) {
    auto values = graphMLNPNStatistics(4, "sha256");
    printNetCones("sha256", values.first.getEssentialCones(10, 10),
                  values.second, "sha256");
    delete values.second;
  }

  //----- Tests to run.

  void npnTestFunctionCone(const std::string &testName) {
    auto values = graphMLNPNStatistics(4, testName);
    printNetCones(testName, values.first.getEssentialCones(200, 1),
                  values.second, testName);
    delete values.second;
  }

  void npnTestFunction(const std::string &testName, int stepNumber,
                       int designNumber) {
    auto [npn, gnet] = graphMLNPNStatistics(4, testName,
                                            "_syn" +
                                            std::to_string(designNumber) +
                                            "_step" +
                                            std::to_string(stepNumber) +
                                            ".bench.graphml");
    std::string folderName = "/home/dreamer_1977/work_clion_github/utopia-eda/output/step" + std::to_string(stepNumber) + "d" + std::to_string(designNumber);
    std::fstream fs(folderName + "/" +
        testName + "_" + std::to_string(gnet->nGates()) + ".csv",
        std::fstream::out);
    npn.printHistogramData(fs);
    fs.close();
    delete gnet;
  }


    void npnTestFunction(const std::string &testName, const std::string &source,
                         const std::string &dest) {
        auto [npn, gnet] = graphMLNPNStatistics2(4, source, testName);
        std::fstream fs(dest + "/" +
                        testName + "_" + std::to_string(gnet->nGates()) + ".csv",
                        std::fstream::out);
        npn.printHistogramData(fs);
        fs.close();
        delete gnet;
    }

    TEST(NpnTest, SpecificPathTestFunction) {
        std::string configFile = "/home/dreamer/work/rewriting/output/temp_filename.txt";

        std::ifstream temp_file(configFile);
        std::string source_file, dest_dir, name;
        if (temp_file.is_open()) {
            getline(temp_file, source_file);
            getline(temp_file, dest_dir);
            getline(temp_file, name);

            if (!name.empty()) {
                auto gNet = eda::gate::parser::verilog::getNet(source_file, name);

                std::cout << "Parsed :" << gNet->nGates() << std::endl;

                NPNCollector npnCollector(gNet);
                npnCollector.process(4, CutsFindVisitor::ALL_CUTS);
                std::fstream fs(dest_dir + "_" + std::to_string(gNet->nGates()) + ".csv",std::fstream::out);
                npnCollector.printHistogramData(fs);
                std::cout << "Histogram printed to :" << dest_dir << std::endl;
                fs.close();
            } else {
                std::cerr << "File is empty";
            }

            temp_file.close();
        } else {
            std::cerr << "Unable to open file " << configFile;
        }
    }

  TEST(NpnTest, generalTestFunction) {
    std::string temp_filename = "/home/dreamer/work/utopia-eda/test/data/gate/parser/graphml/temp_filename.txt";
    std::ifstream temp_file(temp_filename);
    std::string source_file, dest_dir, name;

    if (temp_file.is_open()) {
      getline(temp_file, source_file);
      getline(temp_file, dest_dir);
      getline(temp_file, name);

      if (!name.empty()) {
        std::cout << "Running test: " << name << std::endl;
        npnTestFunction(name, source_file, dest_dir);
      } else {
        std::cerr << "File is empty";
      }

      temp_file.close();
    } else {
      std::cerr << "Unable to open file";
    }
  }

  TEST(NpnTest, handTestFunction) {
    std::string test_name = "sha256";
    std::cout << "Running test: " << test_name << std::endl;
    npnTestFunction(test_name, 1, 1);
  }

  TEST(NpnTest, ethernet) {
    npnTestFunctionCone("ethernet");
  }

  TEST(NpnTest, idft) {
    npnTestFunctionCone("idft");
  }

  TEST(NpnTest, sha256) {
    npnTestFunctionCone("sha256");
  }

  TEST(NpnTest, vga_lcd) {
    npnTestFunctionCone("vga_lcd");
  }

  TEST(NpnTest, simple_spi) {
    npnTestFunctionCone("simple_spi");
  }

  TEST(NpnTest, dynamic_node) {
    npnTestFunctionCone("dynamic_node");
  }

  TEST(NpnTest, mem_ctrl) {
    npnTestFunctionCone("mem_ctrl");
  }

  TEST(NpnTest, mem_ctrl_syn40_step1) {
    npnTestFunctionCone("mem_ctrl_syn40_step1");
  }

  TEST(NpnTest, jpeg) {
    npnTestFunctionCone("jpeg");
  }

  TEST(NpnTest, des3_area) {
    npnTestFunctionCone("des3_area");
  }

  TEST(NpnTest, ac97_ctrl) {
    npnTestFunctionCone("ac97_ctrl");
  }

  TEST(NpnTest, aes_xcrypt) {
    npnTestFunctionCone("aes_xcrypt");
  }

  TEST(NpnTest, spi) {
    npnTestFunctionCone("spi");
  }

  TEST(NpnTest, pci) {
    npnTestFunctionCone("pci");
  }

  TEST(NpnTest, tinyRocket) {
    npnTestFunctionCone("tinyRocket");
  }

  TEST(NpnTest, wb_conmax) {
    npnTestFunctionCone("wb_conmax");
  }

  TEST(NpnTest, ss_pcm) {
    npnTestFunctionCone("ss_pcm");
  }

  TEST(NpnTest, tv80) {
    npnTestFunctionCone("tv80");
  }

  TEST(NpnTest, aes) {
    npnTestFunctionCone("aes");
  }

  TEST(NpnTest, fpu) {
    npnTestFunctionCone("fpu");
  }

  TEST(NpnTest, fir) {
    npnTestFunctionCone("fir");
  }

  TEST(NpnTest, aes_secworks) {
    npnTestFunctionCone("aes_secworks");
  }

  TEST(NpnTest, i2c) {
    npnTestFunctionCone("i2c");
  }

  TEST(NpnTest, usb_phy) {
    npnTestFunctionCone("usb_phy");
  }

  TEST(NpnTest, sasc) {
    npnTestFunctionCone("sasc");
  }

  TEST(NpnTest, dft) {
    npnTestFunctionCone("dft");
  }

  TEST(NpnTest, iir) {
    npnTestFunctionCone("iir");
  }

  TEST(NpnTest, bp_be) {
    npnTestFunctionCone("bp_be");
  }

  TEST(NpnTest, picosoc) {
    npnTestFunctionCone("picosoc");
  }

  TEST(NpnTest, wb_dma) {
    npnTestFunctionCone("wb_dma");

    auto gNet = eda::gate::parser::verilog::getNet("/home/dreamer/work/rewriting/wb_dma.v", "wb_dma.v");

    std::cout << gNet->nGates() << std::endl;

    NPNCollector npnCollector(gNet);
    npnCollector.process(4, 10000);
        std::fstream fs("/home/dreamer/work/utopia-eda/output/wb_dma_verilog.csv",
                        std::fstream::out);
        npnCollector.printHistogramData(fs);
        fs.close();
  }

}  // namespace eda::gate::optimizer
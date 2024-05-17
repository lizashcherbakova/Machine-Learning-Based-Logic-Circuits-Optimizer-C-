//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache License v2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/graphml.h"
#include "gtest/gtest.h"
#include "gate/printer/dot.h"
#include "util/logging.h"
#include "gate/optimizer/plain_parameters_collector.h"

#include <filesystem>
#include <string>

namespace eda::gate::optimizer {

    using GNet = eda::gate::model::GNet;

    GNet* parseGraphMLAndCollectParameters(const std::string &infile) {
        if (!getenv("UTOPIA_HOME")) {
            LOG_ERROR << "UTOPIA_HOME is not set.";
        }

        const std::filesystem::path subCatalog =
                std::filesystem::path("test") / "data" / "gate" / "parser" /
                "graphml" / "OpenABC" / "graphml_openabcd";
        const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
        const std::filesystem::path prefixPathIn = homePath / subCatalog;
        const std::filesystem::path prefixPathOut =
                homePath / subCatalog / "output";
        std::string filename = (prefixPathIn / (infile + ".graphml")).string();

        if (!std::filesystem::exists(filename)) {
            LOG_ERROR << "File " << filename << " doesn't exist!" << std::endl;
            return nullptr;
        }

        parser::graphml::GraphMLParser::ParserData data;
        return parser::graphml::GraphMLParser::parse(filename, data);
    }

    TEST(PlainParametersCollectorTest, ac97Ctrl) {
        auto gNet = parseGraphMLAndCollectParameters("ac97_ctrl_orig.bench");

        eda::gate::optimizer::PlainParametersCollector collector(gNet);
        collector.collect();
        optimizer::PlainParameters parameters = collector.getParameters();

        collector.printParameters(std::cout);

        EXPECT_GT(parameters.numInputs, 0);
        EXPECT_GT(parameters.numOutputs, 0);
        EXPECT_GT(parameters.numGates, 0);
        EXPECT_GE(parameters.numInvertedEdges, 0);
        EXPECT_GT(parameters.longestPath, 0);

        delete gNet;
}

} // namespace eda::gate::optimizer

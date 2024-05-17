//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache License v2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "synthesis_scenario_optimizer.h"


namespace eda::gate::optimizer {

    SynthesisScenarioOptimizer::SynthesisScenarioOptimizer()
            : net(nullptr) {
        initializePython();
    }

    SynthesisScenarioOptimizer::~SynthesisScenarioOptimizer() {
        delete net;
        finalizePython();
    }

    void SynthesisScenarioOptimizer::readGraphML(const std::string &filename) {
        parser::graphml::GraphMLParser::ParserData data;
        net = parser::graphml::GraphMLParser::parse(filename, data);
    }

    void SynthesisScenarioOptimizer::readVerilog(const std::string &filename) {
        eda::gate::parser::verilog::GateVerilogParser parser(filename);
        lorina::text_diagnostics consumer;
        lorina::diagnostic_engine diag(&consumer);
        lorina::return_code result = read_verilog(filename, parser, &diag);
        assert(result == lorina::return_code::success);
        net = parser.getGnet();
    }

    void SynthesisScenarioOptimizer::collectParameters() {
        PlainParametersCollector collector(net);
        collector.collect();
        parameters = collector.getParameters();
    }

    void SynthesisScenarioOptimizer::readSynthesisScenarios(const std::string &filename) {
        std::ifstream infile(filename);
        std::string line;
        while (std::getline(infile, line)) {
            std::istringstream iss(line);
            std::string name, steps;
            if (std::getline(iss, name, ':') && std::getline(iss, steps)) {
                scenarios[name] = steps;
            }
        }
    }

    std::vector<std::pair<std::string, double>> SynthesisScenarioOptimizer::evaluateScenarios() {
        std::vector<std::pair<std::string, double>> results;
        for (const auto &[name, steps]: scenarios) {
            double quality = predictScenarioQuality(steps);
            results.emplace_back(name, quality);
        }
        return results;
    }

    void SynthesisScenarioOptimizer::evaluateAndSelectBestScenarios(int k, const std::string &outputFile) {
        auto results = evaluateScenarios();
        std::sort(results.begin(), results.end(), [](const auto &a, const auto &b) {
            return a.second > b.second;
        });

        std::ofstream outfile(outputFile);
        for (int i = 0; i < std::min(k, static_cast<int>(results.size())); ++i) {
            outfile << results[i].first << ": " << scenarios[results[i].first] << "\n";
        }
    }

    void SynthesisScenarioOptimizer::initializePython() {
        Py_Initialize();
    }

    void SynthesisScenarioOptimizer::finalizePython() {
        Py_Finalize();
    }

    double SynthesisScenarioOptimizer::predictScenarioQuality(const std::string &scenario) {
        PyObject *pName, *pModule, *pFunc;
        PyObject *pArgs, *pValue;

        pName = PyUnicode_DecodeFSDefault("synthesis_predictor");
        pModule = PyImport_Import(pName);
        Py_DECREF(pName);

        if (pModule != nullptr) {
            pFunc = PyObject_GetAttrString(pModule, "predict_quality");
            if (pFunc && PyCallable_Check(pFunc)) {
                pArgs = PyTuple_New(2);
                PyTuple_SetItem(pArgs, 0, Py_BuildValue("(iiiii)",
                                                        parameters.numInputs, parameters.numOutputs,
                                                        parameters.numAnds,
                                                        parameters.numInvertedEdges, parameters.longestPath));
                PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", scenario.c_str()));

                pValue = PyObject_CallObject(pFunc, pArgs);
                Py_DECREF(pArgs);
                if (pValue != nullptr) {
                    double result = PyFloat_AsDouble(pValue);
                    Py_DECREF(pValue);
                    Py_DECREF(pFunc);
                    Py_DECREF(pModule);
                    return result;
                } else {
                    Py_DECREF(pFunc);
                    Py_DECREF(pModule);
                    PyErr_Print();
                    fprintf(stderr, "Call failed\n");
                    return -1.0;
                }
            } else {
                if (PyErr_Occurred())
                    PyErr_Print();
                fprintf(stderr, "Cannot find function \"predict_quality\"\n");
            }
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
        } else {
            PyErr_Print();
            fprintf(stderr, "Failed to load \"synthesis_predictor\"\n");
            return -1.0;
        }
        return -1.0;
    }

} // namespace eda::gate::optimizer

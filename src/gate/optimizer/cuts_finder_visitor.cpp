//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/cuts_finder_visitor.h"

namespace eda::gate::optimizer {

  using Gate = eda::gate::model::Gate;
  using Cuts = CutStorage::Cuts;
  using Cut = CutStorage::Cut;
  using CutIt = Cuts::iterator;

  CutsFindVisitor::CutsFindVisitor(unsigned int cutSize, CutStorage *cutStorage,
                                   unsigned int maxCutsNumber, bool old) :
      cutSize(cutSize), maxCutNum(maxCutsNumber),
      cutStorage(cutStorage), old(old) {}


  VisitorFlags CutsFindVisitor::onNodeBegin(const GateId &vertex) {
    if (old) {
      return onNodeBeginOld(vertex);
    } else {
      return onNodeBeginNew(vertex);
    }
  }

  VisitorFlags CutsFindVisitor::onNodeBeginOld(const GateId &vertex) {
    Gate *gate = Gate::get(vertex);
    if (gate->func() == model::GateSymbol::NOT) {
      return CONTINUE;
    }
    auto *cuts = &cutStorage->cuts[vertex];

    // Adding trivial cut.
    Cut self;
    self.emplace(vertex);
    cuts->emplace(self);


    std::vector<CutIt> ptrs;
    std::vector<Cuts *> inputCuts;
    size_t i = 0;

    ptrs.reserve(gate->inputs().size());
    inputCuts.reserve(gate->inputs().size());

    // Initializing ptrs to cuts of input.
    for (auto input: gate->inputs()) {
      GateId gateIdInput = input.node();
      Gate *gateInput = Gate::get(input.node());
      if (gateInput->func() == model::GateSymbol::NOT) {
        gateIdInput = gateInput->inputs().begin()->node();
      }
      inputCuts.push_back(&cutStorage->cuts[gateIdInput]);
      ptrs.push_back(inputCuts.back()->begin());
    }

    while (true) {
      // Fix cut.
      Cut collected;

      for (auto &it: ptrs) {
        collected.insert((*it).begin(), (*it).end());
        if (collected.size() > cutSize) {
          collected = Cut();
          break;
        }
      }

      if (!collected.empty()) {
        cuts->emplace(collected);

        if (maxCutNum != ALL_CUTS && cuts->size() > maxCutNum) {
          return CONTINUE;
        }
      }

      // Incrementing iterators to move to the next combination of nodes for cut.
      size_t prevInd = i;
      while (i < inputCuts.size() && ++ptrs[i] == inputCuts[i]->end()) {
        ptrs[i] = inputCuts[i]->begin();
        ++i;
      }

      if (i >= inputCuts.size()) {
        break;
      }

      if (i != prevInd) {
        i = 0;
      }
    }
    return CONTINUE;
  }

  VisitorFlags CutsFindVisitor::onNodeBeginNew(const GateId &vertex) {
    // Checking if the gate is valid.
    Gate *gate = Gate::get(vertex);
    if (gate->func() == model::GateSymbol::NOT) {
      return CONTINUE;
    }
    Cuts *cuts = &cutStorage->cuts[vertex];

    // Adding trivial cut.
    Cut self;
    self.emplace(vertex);
    cuts->emplace(self);

    std::vector<CutIt> ptrs;
    std::vector<Cuts *> inputCuts;
    size_t i = 0;

    ptrs.reserve(gate->inputs().size());
    inputCuts.reserve(gate->inputs().size());

    // Initializing ptrs to cuts of input.
    for (auto input: gate->inputs()) {
      GateId gateIdInput = input.node();
      Gate *gateInput = Gate::get(input.node());
      if (gateInput->func() == model::GateSymbol::NOT) {
        gateIdInput = gateInput->inputs().begin()->node();
      }
      if (cutStorage->cuts[gateIdInput].empty()) {
          onNodeBeginNew(gateIdInput);
      }
      inputCuts.push_back(&cutStorage->cuts[gateIdInput]);
      ptrs.push_back(inputCuts.back()->begin());
    }

    while (true) {
      // Fix cut.
      Cut collected;

      for (auto &it: ptrs) {
        collected.insert((*it).begin(), (*it).end());
        if (collected.size() > cutSize) {
          collected = Cut();
          break;
        }
      }

      // Determining cut type. (good cut, bigger_cut (cut that contains dominator), smaller_cut)
      bool biggerCut = false;
      bool incrementAll = false;

      if (!collected.empty()) {
        std::vector<Cuts::iterator> toRemove;
        for (auto it = cuts->begin(); it != cuts->end(); ++it) {
          const Cut &addedCut = *it;
          if (addedCut.size() > collected.size()) {
            if (isSubsetOf(collected, addedCut)) {
              toRemove.emplace_back(it);
            }
          } else {
            if (isSubsetOf(addedCut, collected)) {
              biggerCut = true;
              break;
            }
          }
        }
        // remove all and add this
        // if it's dominator (size==0 and smallerCut = true) - everything all cuts have to be incremented (think of how to do that)
        // don't add this because it's a bigger cut

        if (!biggerCut) {
          for (const auto &it: toRemove) {
            cuts->erase(it);
          }

          // Emplacing the cut.
          cuts->emplace(collected);
          if (maxCutNum != ALL_CUTS && cuts->size() > maxCutNum) {
            return CONTINUE;
          }

          incrementAll = collected.size() == 1;
        }
      }

      // Incrementing iterators to move to the next combination of nodes for cut.
      if (incrementAll) {
        bool newCombination = false;
        for (size_t j = 0; j < ptrs.size(); ++j) {
          ++ptrs[j];
          if (ptrs[j] == inputCuts[j]->end()) {
            ptrs[j] = inputCuts[j]->begin();
          } else {
            newCombination = true;
          }
        }
        if (!newCombination) {
          break;
        }
        i = 0;
      } else {
        size_t prevInd = i;
        while (i < inputCuts.size() && ++ptrs[i] == inputCuts[i]->end()) {
          ptrs[i] = inputCuts[i]->begin();
          ++i;
        }

        if (i >= inputCuts.size()) {
          break;
        }

        if (i != prevInd) {
          i = 0;
        }
      }
    }
    return CONTINUE;
  }

  VisitorFlags CutsFindVisitor::onNodeEnd(const GateId &) {
    return CONTINUE;
  }

} // namespace eda::gate::optimizer

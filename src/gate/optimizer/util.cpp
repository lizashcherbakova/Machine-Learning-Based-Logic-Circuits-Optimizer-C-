//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/util.h"

#include <queue>

namespace eda::gate::optimizer {

  std::vector<GNet::GateId> getNext(GateId node, bool forward) {
    std::vector<GNet::GateId> next;
    if (forward) {
      const auto &outputs = Gate::get(node)->links();
      next.reserve(outputs.size());
      for (const auto &out: outputs) {
        next.emplace_back(out.target);
      }
    } else {
      const auto &inputs = Gate::get(node)->inputs();
      next.reserve(inputs.size());
      for (const auto &in: inputs) {
        next.emplace_back(in.node());
      }
    }
    return next;
  }

  bool isCut(const GateId &gate, const Cut &cut, GateId &failed) {
    std::queue<GateId> bfs;
    bfs.push(gate);
    while (!bfs.empty()) {
      Gate *cur = Gate::get(bfs.front());
      if (cut.find(cur->id()) == cut.end()) {
        if (cur->isSource()) {
          failed = cur->id();
          return false;
        }
        for (auto input: cur->inputs()) {
          bfs.push(input.node());
        }
      }
      bfs.pop();
    }
    return true;
  }

  std::unordered_set<GateId>
  intersect(std::unordered_map<GateId, std::unordered_set<GateId>> &dominators,
            const std::vector<base::model::Signal<GateId>> &inputs) {

    // Finding the least dominators set.
    auto minIt = std::min_element(
        inputs.begin(), inputs.end(), [&](const auto &a, const auto &b) {
          return dominators[a.node()].size() < dominators[b.node()].size();
        });

    std::unordered_set<GateId> last = dominators[minIt->node()];

    for (const auto &signal: inputs) {
      if (last.empty()) {
        break;
      }
      std::unordered_set<GateId> result;
      if (signal == *minIt) {
        continue;
      }
      const auto &set = dominators[signal.node()];
      for (const GateId &elem: last) {
        if (set.find(elem) != set.end()) {
          result.emplace(elem);
        }
      }
      last = std::move(result);
    }
    return last;
  }

  std::unordered_map<GateId, std::unordered_set<GateId>>
  findDominators(const std::vector<GateId> &topoOrder) {
    std::unordered_map<GateId, std::unordered_set<GateId>> dominators;

    for (unsigned int current: topoOrder) {
      const auto &inputs = Gate::get(current)->inputs();

      if (inputs.empty()) {
        dominators[current] = {current};
      } else {
        auto newDom = intersect(dominators, Gate::get(current)->inputs());
        auto &set = dominators[current] = std::move(newDom);
        set.emplace(current);
      }
    }

    return dominators;
  }

  void
  getConeSet(GateId start, std::unordered_set<GateId> &cone, bool forward) {
    std::queue<GateId> bfs;
    bfs.push(start);

    // First trace to define needed nodes.
    while (!bfs.empty()) {
      GateId cur = bfs.front();
      bfs.pop();
      cone.emplace(cur);
      auto next = getNext(cur, forward);
      for (auto node: next) {
        bfs.push(node);
      }
    }
  }

  void getConeSet(GateId start, const Cut &cut,
                  std::unordered_set<GateId> &coneNodes, bool forward) {
    std::queue<GateId> bfs;
    bfs.push(start);

    // First trace to define needed nodes.
    while (!bfs.empty()) {
      GateId cur = bfs.front();
      bfs.pop();
      coneNodes.emplace(cur);
      if (cut.find(cur) != cut.end()) {
        continue;
      }
      auto next = getNext(cur, forward);
      for (auto node: next) {
        bfs.push(node);
      }
    }
  }

  BoundGNet extractCone(const GNet *net, GateId root, const Cut &cut,
                        const Order &order) {
    ConeVisitor coneVisitor(cut, root);
    Walker walker(net, &coneVisitor);
    walker.walk(cut, root, false);

    BoundGNet boundGNet;
    boundGNet.net = std::shared_ptr<GNet>(coneVisitor.getGNet());
    const auto &cutConeMap = coneVisitor.getResultMatch();
    for (const auto &gate: order) {
      boundGNet.inputBindings.push_back(cutConeMap.find(gate)->second);
    }
    return boundGNet;
  }

  BoundGNet extractCone(const GNet *net, GateId root, const Order &order) {
    Cut cut(order.begin(), order.end());

    ConeVisitor coneVisitor(cut, root);
    Walker walker(net, &coneVisitor);
    walker.walk(cut, root, false);

    BoundGNet boundGNet;
    boundGNet.net = std::shared_ptr<GNet>(coneVisitor.getGNet());
    const auto &cutConeMap = coneVisitor.getResultMatch();
    for (const auto &gate: order) {
      boundGNet.inputBindings.push_back(cutConeMap.find(gate)->second);
    }
    return boundGNet;
  }

  void rmRecursive(GNet *net, GateId start) {

    std::vector<GateId> removed;

    auto targets = TargetsList(start);

    LinksRemoveCounter removeCounter(targets, {}, removed);
    Walker walker = Walker(net, &removeCounter);
    walker.walk(start, false);

    // Deleting startRm target nodes.
    for (auto node: targets.getTargets()) {
      const auto &outputs = Gate::get(node)->links();
      for (const auto &out: outputs) {
        auto *next = Gate::get(out.target);
        if (next->isTarget()) {
          net->eraseGate(out.target);
        } else {
          auto inputs = next->inputs();
          auto foundId =
              std::find_if(inputs.begin(), inputs.end(),
                           [node](const auto &x) { return x.node() == node; });
          inputs.erase(foundId);
          net->setGate(out.target, next->func(), inputs);
        }
      }
      net->eraseGate(node);
    }

    // Erasing gates with zero fanout.
    for (auto gate: removed) {
      net->eraseGate(gate);
    }
  }

  bool isSubsetOf(const Cut &smaller, const Cut &bigger) {
    for (const GateId &gateId: smaller) {
      if (bigger.find(gateId) == bigger.end()) {
        return false;
      }
    }
    return true;
  }

  void getHeights(GateId start, int &maxHeight, int &minHeight,
                  const Cut &cut) {
    minHeight = std::numeric_limits<int>::max();
    maxHeight = -1;
    // Pair of gate ID and current height.
    std::queue<std::pair<GateId, int>> bfs;
    bfs.emplace(start, 0);
    std::unordered_set<GateId> visited;

    while (!bfs.empty()) {
      auto [current, currentHeight] = bfs.front();
      bfs.pop();
      if (cut.count(current)) {
        minHeight = std::min(minHeight, currentHeight);
        maxHeight = std::max(maxHeight, currentHeight);
      } else {
        if (visited.count(current)) {
          continue;
        }
        visited.emplace(current);
        auto nextGates = getNext(current, false);
        for (auto &nextGate: nextGates) {
          bfs.emplace(nextGate, currentHeight + 1);
        }
      }
    }
  }
} // namespace eda::gate::optimizer

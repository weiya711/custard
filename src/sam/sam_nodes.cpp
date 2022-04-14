#include "sam_nodes.h"

using namespace std;

namespace taco {
namespace sam {

    vector<SamIR> FiberLookupNode::getInputs() const {
        vector<SamIR> result;
        result.push_back(in_ref);
        return result;
    }

    vector<SamIR> FiberLookupNode::getOutputs() const {
        vector<SamIR> result;
        result.push_back(out_crd);
        result.push_back(out_ref);
        return result;
    }

    vector<SamIR> FiberWriteNode::getInputs() const {
        vector<SamIR> result;
        result.push_back(in_crd);
        return result;
    }

    std::vector<SamIR> FiberWriteNode::getOutputs() const {
        return {};
    }
}
}

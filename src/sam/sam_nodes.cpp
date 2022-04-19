#include "sam_nodes.h"

using namespace std;

namespace taco {
namespace sam {
    // FiberLookupNode
    vector<SamIR> FiberLookupNode::getInputs() const {
        vector<SamIR> result;
        return result;
    }

    vector<SamIR> FiberLookupNode::getOutputs() const {
        vector<SamIR> result;
        result.push_back(out_crd);
        result.push_back(out_ref);
        return result;
    }

    std::string FiberLookupNode::getName() const {
        stringstream ss;
        ss << "FiberLookup " << i.getName() << ": " << tensorVar.getName() << to_string(mode) << "\\n" << modeFormat.getName();
        return ss.str();
    }

    void FiberLookupNode::setOutCrd(SamIR new_out_crd) {
        out_crd = new_out_crd;
    }

    void FiberLookupNode::setOutRef(SamIR new_out_ref) {
        out_ref = new_out_ref;
    }

    void FiberLookupNode::setOutputs(SamIR new_out_ref, SamIR new_out_crd) {
        setOutCrd(new_out_crd);
        setOutRef(new_out_ref);
    }

    void FiberLookupNode::setSource(bool new_source) {
        source = new_source;
    }

    void FiberLookupNode::setDim(bool new_dim) {
        dim = new_dim;
    }

    // FiberWriteNode
    vector<SamIR> FiberWriteNode::getInputs() const {
        vector<SamIR> result;
        return result;
    }

    std::vector<SamIR> FiberWriteNode::getOutputs() const {
        return {};
    }

    void FiberWriteNode::setSink(bool new_sink) {
        this->sink = new_sink;
    }

    std::string FiberWriteNode::getName() const {
        stringstream ss;
        ss << "FiberWrite " << i.getName() << ": " << tensorVar.getName() << to_string(mode) << "\\n" << modeFormat.getName();
        return ss.str();
    }

    std::string RepeatNode::getName() const {
        stringstream ss;
        ss << "Repeat " <<  i.getName()  << ": " << tensorVar.getName();
        return ss.str();
    }

    std::string IntersectNode::getName() const {
        stringstream ss;
        ss << "Intersect " << i.getName();
        return ss.str();
    }

    std::string UnionNode::getName() const {
        stringstream ss;
        ss << "Union " << i.getName();
        return ss.str();
    }

    std::string RepeatSigGenNode::getName() const {
        stringstream ss;
        ss << "RepeatSignalGenerator " << i.getName() << ": " << tensorVar.getName() << to_string(mode);
        return ss.str();
    }
}
}

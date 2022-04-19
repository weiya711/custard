#include "sam_nodes.h"

using namespace std;

namespace taco {
namespace sam {
    // FiberLookupNode
    std::string FiberLookupNode::getName() const {
        stringstream ss;
        ss << "FiberLookup " << i.getName() << ": " << tensorVar.getName() << to_string(mode) << "\\n" << modeFormat.getName();
        return ss.str();
    }

    std::string FiberWriteNode::getName() const {
        stringstream ss;
        if (vals) {
            ss << "FiberWrite Vals: " << tensorVar.getName();
        } else {
            ss << "FiberWrite " << i.getName() << ": " << tensorVar.getName() << to_string(mode) << "\\n"
               << modeFormat.getName();
        }
        return ss.str();
    }

    std::string RepeatNode::getName() const {
        stringstream ss;
        ss << "Repeat " <<  i.getName()  << ": " << tensorVar.getName();
        return ss.str();
    }

    std::string JoinerNode::getName() const {
        stringstream ss;
        ss << getNodeName() << " " << i.getName();
        return ss.str();
    }

    std::string RepeatSigGenNode::getName() const {
        stringstream ss;
        ss << "RepeatSignalGenerator " << i.getName();
        return ss.str();
    }

    std::string ArrayNode::getName() const {
        stringstream ss;
        ss << "Array Vals: " << tensorVar.getName();
        return ss.str();
    }

    std::string SparseAccumulatorNode::getNodeStr() const {
        stringstream ss;
        ss << "SparseAccumulator " << to_string(order);
        return ss.str();
    }
}
}

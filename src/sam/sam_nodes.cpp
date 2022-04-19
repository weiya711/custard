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
        ss << "RepeatSignalGenerator " << i.getName();
        return ss.str();
    }
}
}

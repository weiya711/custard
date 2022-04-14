#ifndef TACO_SAM_NODES_ABSTRACT_H
#define TACO_SAM_NODES_ABSTRACT_H
#include <vector>
#include <memory>

#include "taco/type.h"
#include "taco/util/uncopyable.h"
#include "taco/util/intrusive_ptr.h"

namespace taco {
namespace sam {

class SamVisitorStrict;

enum class SamNodeType {
    FiberLookup,
    FiberWrite,
    Repeat,
    RepeatSigGen,
    Intersect,
    Union,
    Array,
    Mul,
    Add,
    Reduce,
    SparseAccumulator
};

struct SAMNode : public util::Manageable<SAMNode>,
                 private util::Uncopyable {
public:
    SAMNode() = default;

    explicit SAMNode(SamNodeType _type) : _type_info(_type) {}

    virtual ~SAMNode() = default;

    virtual void accept(SamVisitorStrict *) const = 0;

    virtual std::vector<SAMNode> getInputs() = 0;

    virtual std::vector<SAMNode> getOutputs() = 0;

    virtual SamNodeType type_info() const;

protected:
    SamNodeType _type_info;
};

}
}
#endif //TACO_SAM_NODES_ABSTRACT_H

#ifndef TACO_SAM_NODES_ABSTRACT_H
#define TACO_SAM_NODES_ABSTRACT_H
#include <vector>
#include <memory>

#include "taco/type.h"
#include "taco/util/uncopyable.h"
#include "taco/util/intrusive_ptr.h"

namespace taco {
namespace sam {

class SAMVisitorStrict;
class SamIR;

enum class SamNodeType {
    Root,
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

    virtual ~SAMNode() = default;

    virtual void accept(SAMVisitorStrict *) const = 0;

    virtual std::vector<SamIR> getInputs() const = 0;

    virtual std::vector<SamIR> getOutputs() const = 0;

    virtual std::string getName() const = 0;

    SamNodeType type_info() const;

    bool printed = false;

protected:
    SamNodeType _type_info;
};

}
}
#endif //TACO_SAM_NODES_ABSTRACT_H

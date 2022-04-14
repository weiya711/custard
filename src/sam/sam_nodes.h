#ifndef TACO_SAM_NODES_H
#define TACO_SAM_NODES_H
#include <vector>
#include <memory>

#include "sam_visitor.h"
#include "taco/format.h"
#include "taco/index_notation/index_notation.h"
#include "sam_nodes_abstract.h"
#include "sam_ir.h"


namespace taco {
namespace sam {

struct FiberLookupNode : public SAMNode {
    FiberLookupNode() : SAMNode() {}

    FiberLookupNode(const SamIR& in_ref, const SamIR& out_ref, const SamIR& out_crd)
            : SAMNode(), in_ref(in_ref), out_ref(out_ref), out_crd(out_crd) {}

    void accept(SAMVisitorStrict* v) const {
        v->visit(this);
    }
    // Inputs
    SamIR in_ref;
    //Outputs
    SamIR out_ref;
    SamIR out_crd;

    // Metadata
    TensorVar tensorVar;
    /// Mode format for this fiber lookup block
    ModeFormat modeFormat;
    IndexVar i;
    int mode = 0;

    /// Dimension of this fiber lookup
    int dim = 0;
    /// If this fiber lookup is a SAM graph source (i.e. stream data from a user-provided input)
    /// This is used in the AHA backend example for glb mode
    bool source = false;

    SamNodeType _type_info = SamNodeType::FiberLookup;

};

struct FiberWrite : public SAMNode {
    // Inputs
    SamIR in_crd;
    // Outputs
    // TODO: figure out if write scanners should have outputs (for workspaces)

    // Metadata
    /// Dimension of this fiber lookup
    int dim = 0;
    /// If this fiber write is a SAM graph sink (i.e. stream data back to the end-user as final result)
    /// This is used in the AHA backend example for glb mode
    bool sink = false;
    bool vals = false;
    ModeFormat modeFormat;
    IndexVar i;
    int rank;
    TensorVar tensorVar;

    static const SamNodeType _type_info = SamNodeType::FiberWrite;
};

struct Repeat : public SAMNode {
    // Inputs
    SamIR in_crd;
    SamIR in_repsig;
    // Outputs
    SamIR out_crd;

    // Metadata
    // None

    static const SamNodeType _type_info = SamNodeType::Repeat;
};

struct RepeatSigGen : public SAMNode {
    // Inputs
    SamIR in_crd;
    // No Outputs
    SamIR out_repsig;

    // Metadata
    // None

    static const SamNodeType _type_info = SamNodeType::RepeatSigGen;
};

struct Intersect : public SAMNode {
    // Inputs
    SamIR in_a_crd;
    SamIR in_b_crd;
    SamIR in_a_ref;
    SamIR in_b_ref;

    // No Outputs
    SamIR out_a_ref;
    SamIR out_b_ref;
    SamIR out_crd;

    // Metadata
    // None

    static const SamNodeType _type_info = SamNodeType::Intersect;
};

struct Union : public SAMNode {
    // Inputs
    SamIR in_a_crd;
    SamIR in_b_crd;
    SamIR in_a_ref;
    SamIR in_b_ref;

    // No Outputs
    SamIR out_a_ref;
    SamIR out_b_ref;
    SamIR out_crd;

    // Metadata
    // None

    static const SamNodeType _type_info = SamNodeType::Union;
};

struct Array : public SAMNode {
    // Inputs
    SamIR in_ref;
    // No Outputs
    SamIR out_val;

    // Metadata
    int length = 0;
    int vals = true;


    static const SamNodeType _type_info = SamNodeType::Array;
};

struct Mul : public SAMNode {
    // Inputs
    SamIR in_a_val;
    SamIR in_b_val;
    // No Outputs
    SamIR out_val;

    // Metadata
    // None

    static const SamNodeType _type_info = SamNodeType::Mul;
};

struct Add : public SAMNode {
    // Inputs
    SamIR in_a_val;
    SamIR in_b_val;
    // No Outputs
    SamIR out_val;

    // Metadata
    // None

    static const SamNodeType _type_info = SamNodeType::Add;
};

struct Reduce : public SAMNode {
    // Inputs
    SamIR in_val;
    // No Outputs
    SamIR out_val;

    // Metadata
    // None

    static const SamNodeType _type_info = SamNodeType::Reduce;
};

struct SparseAccumulator : public SAMNode {
    // Inputs
    SamIR in_val;
    // TODO: need to parameterize coordinate streams...

    // No Outputs
    SamIR out_val;

    // Metadata
    int order = 0;

    static const SamNodeType _type_info = SamNodeType::SparseAccumulator;
};

        /// Returns true if expression e is of type E.
        template <typename E>
        inline bool isa(const SAMNode* e) {
            return e != nullptr && dynamic_cast<const E*>(e) != nullptr;
        }

/// Casts the expression e to type E.
        template <typename E>
        inline const E* to(const SAMNode* e) {
            taco_iassert(isa<E>(e)) <<
                                    "Cannot convert " << typeid(e).name() << " to " << typeid(E).name();
            return static_cast<const E*>(e);
        }

        template <typename I>
        inline const typename I::Node* getNode(const I& sam) {
            taco_iassert(isa<typename I::Node>(sam.ptr));
            return static_cast<const typename I::Node*>(sam.ptr);
        }

}
}


#endif //TACO_SAM_NODES_H

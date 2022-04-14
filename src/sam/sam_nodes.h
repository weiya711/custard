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

    FiberLookupNode(const SamIR& in_ref, const IndexVar& i,
                    const TensorVar& tensorVar, int& mode)
            : FiberLookupNode(in_ref, nullptr, nullptr, i, tensorVar, mode) {
    }

    FiberLookupNode(const SamIR& in_ref, const SamIR& out_ref, const SamIR& out_crd, const IndexVar& i,
                    const TensorVar& tensorVar, int& mode)
            : SAMNode(), in_ref(in_ref), out_ref(out_ref), out_crd(out_crd), tensorVar(tensorVar), mode(mode), i(i) {
        taco_iassert(mode < tensorVar.getOrder());
        modeFormat = tensorVar.getFormat().getModeFormats().at(mode);
    }

    void accept(SAMVisitorStrict* v) const override{
        v->visit(this);
    }

    std::vector<SamIR> getInputs() const override;

    std::vector<SamIR> getOutputs() const override;

    // Inputs
    SamIR in_ref;
    //Outputs
    SamIR out_ref;
    SamIR out_crd;

    // Metadata
    TensorVar tensorVar;
    /// Mode format for this fiber lookup block
    ModeFormat modeFormat;
    int mode = 0;
    IndexVar i;

    /// Dimension of this fiber lookup
    int dim = 0;
    /// If this fiber lookup is a SAM graph source (i.e. stream data from a user-provided input)
    /// This is used in the AHA backend example for glb mode
    bool source = false;

    SamNodeType _type_info = SamNodeType::FiberLookup;

};

struct FiberWriteNode : public SAMNode {
    FiberWriteNode() : SAMNode() {}

    FiberWriteNode(const SamIR& in_crd, IndexVar& i,
                   const TensorVar& tensorVar, int mode)
            : FiberWriteNode(in_crd, i, tensorVar, mode, true, false) {
    }

    FiberWriteNode(const SamIR& in_crd, IndexVar& i,
                   const TensorVar& tensorVar, int mode, bool sink, bool vals)
            : SAMNode(), in_crd(in_crd), tensorVar(tensorVar), mode(mode), i(i), sink(sink), vals(vals) {
        taco_iassert(mode < tensorVar.getOrder());
        modeFormat = tensorVar.getFormat().getModeFormats().at(mode);
    }


    void accept(SAMVisitorStrict* v) const override {
        v->visit(this);
    }

    std::vector<SamIR> getInputs() const override;

    std::vector<SamIR> getOutputs() const override;

    // Inputs
    SamIR in_crd;
    // Outputs
    // TODO: figure out if write scanners should have outputs (for workspaces)

    // Metadata
    TensorVar tensorVar;
    int mode = 0;
    ModeFormat modeFormat;
    IndexVar i;

    /// Dimension of this fiber lookup
    int dim = 0;
    /// If this fiber write is a SAM graph sink (i.e. stream data back to the end-user as final result)
    /// This is used in the AHA backend example for glb mode
    bool sink = true;
    bool vals = false;

    static const SamNodeType _type_info = SamNodeType::FiberWrite;
};

/*
struct RepeatNode : public SAMNode {
    RepeatNode() : SAMNode() {}

    RepeatNode(const SamIR& in_crd, const SamIR& in_repsig,
               const TensorVar& tensorVar, const IndexVar& i) :
               RepeatNode(in_crd, in_repsig, nullptr, tensorVar, i) {}

    RepeatNode(const SamIR& in_crd, const SamIR& in_repsig, const SamIR& out_crd,
               const TensorVar& tensorVar, const IndexVar& i)
            : SAMNode(), in_crd(in_crd), in_repsig(in_repsig), out_crd(out_crd),
            tensorVar(tensorVar), i(i) {
    }

    // Inputs
    SamIR in_crd;
    SamIR in_repsig;
    // Outputs
    SamIR out_crd;

    // Metadata
    TensorVar tensorVar;
    IndexVar i;

    static const SamNodeType _type_info = SamNodeType::Repeat;
};

struct RepeatSigGenNode : public SAMNode {
    RepeatSigGenNode() : SAMNode() {}

    RepeatSigGenNode(const SamIR& in_crd, const SamIR& out_repsig,
                     TensorVar& tensorVar, const IndexVar& i) : SAMNode(), in_crd(in_crd), out_repsig(out_repsig),
                     tensorVar(tensorVar), i(i) {}

    // Inputs
    SamIR in_crd;
    // No Outputs
    SamIR out_repsig;

    // Metadata
    TensorVar tensorVar;
    IndexVar i;

    static const SamNodeType _type_info = SamNodeType::RepeatSigGen;
};

struct JoinerNode : public SAMNode {
    JoinerNode() : SAMNode() {}

    JoinerNode(SamIR& in_a_crd, SamIR& in_a_ref, SamIR& in_b_crd, SamIR& in_b_ref,
            SamIR& out_crd, SamIR& out_a_ref, SamIR& out_b_ref) : SAMNode(),
    in_a_crd(in_a_crd), in_a_ref(in_a_ref),
    in_b_crd(in_b_crd), in_b_ref(in_b_ref),
    out_crd(out_crd), out_a_ref(out_a_ref), out_b_ref(out_b_ref) {}
    // Inputs
    SamIR in_a_crd;
    SamIR in_a_ref;
    SamIR in_b_crd;
    SamIR in_b_ref;

    // No Outputs
    SamIR out_crd;
    SamIR out_a_ref;
    SamIR out_b_ref;

    // Metadata: None
};

struct IntersectNode : public JoinerNode {
    IntersectNode() : JoinerNode() {}

    IntersectNode(SamIR& in_a_crd, SamIR& in_a_ref, SamIR& in_b_crd, SamIR& in_b_ref,
                  SamIR& out_crd, SamIR& out_a_ref, SamIR& out_b_ref) : JoinerNode(
                  in_a_crd, in_a_ref,
                  in_b_crd, in_b_ref,
                  out_crd, out_a_ref, out_b_ref) {}

    static const SamNodeType _type_info = SamNodeType::Intersect;
};

struct UnionNode : public JoinerNode {
    UnionNode() : JoinerNode() {}

    UnionNode(SamIR& in_a_crd, SamIR& in_a_ref, SamIR& in_b_crd, SamIR& in_b_ref,
            SamIR& out_crd, SamIR& out_a_ref, SamIR& out_b_ref) : JoinerNode(
            in_a_crd, in_a_ref,
            in_b_crd, in_b_ref,
            out_crd, out_a_ref, out_b_ref) {}

    static const SamNodeType _type_info = SamNodeType::Union;
};

struct ArrayNode : public SAMNode {
    // Inputs
    SamIR in_ref;
    // No Outputs
    SamIR out_val;

    // Metadata
    int length = 0;
    int vals = true;


    static const SamNodeType _type_info = SamNodeType::Array;
};

struct MulNode : public SAMNode {
    // Inputs
    SamIR in_a_val;
    SamIR in_b_val;
    // No Outputs
    SamIR out_val;

    // Metadata
    // None

    static const SamNodeType _type_info = SamNodeType::Mul;
};

struct AddNode : public SAMNode {
    // Inputs
    SamIR in_a_val;
    SamIR in_b_val;
    // No Outputs
    SamIR out_val;

    // Metadata
    // None

    static const SamNodeType _type_info = SamNodeType::Add;
};

struct ReduceNode : public SAMNode {
    // Inputs
    SamIR in_val;
    // No Outputs
    SamIR out_val;

    // Metadata
    // None

    static const SamNodeType _type_info = SamNodeType::Reduce;
};

struct SparseAccumulatorNode : public SAMNode {
    // Inputs
    SamIR in_val;
    // TODO: need to parameterize coordinate streams...

    // No Outputs
    SamIR out_val;

    // Metadata
    int order = 0;

    static const SamNodeType _type_info = SamNodeType::SparseAccumulator;
};

*/

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

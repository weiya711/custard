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

struct RootNode : public SAMNode {
    RootNode() : SAMNode() {}

    explicit RootNode(const std::vector<SamIR>& nodes) : SAMNode(), nodes(nodes) {
    }

    void accept(SAMVisitorStrict* v) const override {
        v->visit(this);
    }

    std::string getName() const override {
        return "RootNode";
    }

    std::vector<SamIR> nodes;

    SamNodeType _type_info = SamNodeType::Root;
};


struct BroadcastNode : public SAMNode {
    BroadcastNode() : SAMNode() {}

    explicit BroadcastNode(const std::vector<SamIR>& outputs, const SamEdgeType& type, int nodeID) :
    SAMNode(), outputs(outputs), type(type), nodeID(nodeID) {
    }

    void accept(SAMVisitorStrict* v) const override {
        v->visit(this);
    }

    std::string getName() const override {
        return "WireBroadcast";
    }

    std::vector<SamIR> outputs;
    SamEdgeType type;
    int nodeID = 0;

    SamNodeType _type_info = SamNodeType::Broadcast;
};

struct FiberLookupNode : public SAMNode {
    FiberLookupNode() : SAMNode() {}

    FiberLookupNode(const SamIR& out_ref, const SamIR& out_crd, const IndexVar& i,
                    const TensorVar& tensorVar, int& mode, bool root, bool source, int nodeID)
            : SAMNode(), out_ref(out_ref), out_crd(out_crd),
            tensorVar(tensorVar), mode(mode), i(i), root(root), source(source), nodeID(nodeID) {
        taco_iassert(mode < tensorVar.getOrder());
        modeFormat = tensorVar.getFormat().getModeFormats().at(mode);
    }

    void accept(SAMVisitorStrict* v) const override{
        v->visit(this);
    }

    std::string getName() const override;


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
    bool root = false;
    bool source = false;

    int nodeID = 0;

    SamNodeType _type_info = SamNodeType::FiberLookup;

};

struct FiberWriteNode : public SAMNode {
    FiberWriteNode() : SAMNode() {}

    FiberWriteNode(IndexVar& i,
                   const TensorVar& tensorVar, int mode, bool sink, bool vals, int nodeID)
            : SAMNode(), tensorVar(tensorVar), mode(mode), i(i), sink(sink), vals(vals), nodeID(nodeID) {
        taco_iassert(mode < tensorVar.getOrder());
        modeFormat = tensorVar.getFormat().getModeFormats().at(mode);
    }

    void accept(SAMVisitorStrict* v) const override {
        v->visit(this);
    }

    std::string getName() const override;

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
    bool root = false;
    bool vals = false;

    int nodeID = 0;

    static const SamNodeType _type_info = SamNodeType::FiberWrite;
};


struct RepeatNode : public SAMNode {
    RepeatNode() : SAMNode() {}

    RepeatNode(const SamIR& out_ref, const IndexVar& i, const TensorVar& tensorVar, bool root, int nodeID)
            : SAMNode(), out_ref(out_ref),
            i(i), tensorVar(tensorVar), root(root), nodeID(nodeID) {
    }


    void accept(SAMVisitorStrict* v) const override {
        v->visit(this);
    }

    std::string getName() const override;

    // Outputs
    SamIR out_ref;

    // Metadata
    IndexVar i;
    TensorVar tensorVar;

    bool root = false;

    int nodeID = 0;

    static const SamNodeType _type_info = SamNodeType::Repeat;
};

    struct JoinerNode : public SAMNode {
        virtual std::string getNodeName() const = 0;

        // No Outputs
        SamIR out_crd;
        std::vector<SamIR> out_refs;

        // Metadata: None
        IndexVar i;
        int numInputs = 0;
        int nodeID = 0;

    protected:
        JoinerNode() : SAMNode() {}

        JoinerNode(SamIR& out_crd, std::vector<SamIR>& out_refs, IndexVar& i, int nodeID) :
        SAMNode(), out_crd(out_crd), out_refs(out_refs), i(i), nodeID(nodeID) {
            numInputs = (int)out_refs.size();
        }
    };

    struct IntersectNode : public JoinerNode {
        IntersectNode() : JoinerNode() {}

        IntersectNode(SamIR& out_crd, std::vector<SamIR>& out_refs, IndexVar& i, int nodeID) :
        JoinerNode(out_crd, out_refs, i, nodeID) {}

        void accept(SAMVisitorStrict* v) const override {
            v->visit(this);
        }

        std::string getName() const override;
        std::string getNodeName() const override {
            return "intersect";
        }

        static const SamNodeType _type_info = SamNodeType::Intersect;
    };

    struct UnionNode : public JoinerNode {
        UnionNode() : JoinerNode() {}

        UnionNode(SamIR& out_crd, std::vector<SamIR>& out_refs, IndexVar& i, int nodeID) :
        JoinerNode(out_crd, out_refs, i, nodeID) {}

        void accept(SAMVisitorStrict* v) const override {
            v->visit(this);
        }


        std::string getName() const override;
        std::string getNodeName() const override {
            return "union";
        }

        static const SamNodeType _type_info = SamNodeType::Union;
    };

struct RepeatSigGenNode : public SAMNode {
    RepeatSigGenNode() : SAMNode() {}

    RepeatSigGenNode(const SamIR& out_repsig, const IndexVar& i, int nodeID) :
    SAMNode(), out_repsig(out_repsig), i(i), nodeID(nodeID) {}

    void accept(SAMVisitorStrict* v) const override {
        v->visit(this);
    }

    std::string getName() const override;

    // No Outputs
    SamIR out_repsig;

    // Metadata
    IndexVar i;

    int nodeID = 0;

    static const SamNodeType _type_info = SamNodeType::RepeatSigGen;
};

struct ArrayNode : public SAMNode {
    ArrayNode() : SAMNode() {}

    ArrayNode(const SamIR& out_val, const TensorVar& tensorVar, int nodeID) :
            SAMNode(), out_val(out_val), tensorVar(tensorVar), nodeID(nodeID) {}

    void accept(SAMVisitorStrict* v) const override {
        v->visit(this);
    }

    std::string getName() const override;

    // No Outputs
    SamIR out_val;

    // Metadata
    TensorVar tensorVar;
    bool vals = true;

    int nodeID = 0;

    static const SamNodeType _type_info = SamNodeType::Array;
};

/*
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

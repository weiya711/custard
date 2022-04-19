//
// Created by oliviahsu on 4/13/22.
//

#ifndef TACO_SAM_IR_H
#define TACO_SAM_IR_H

#include "sam_nodes_abstract.h"
#include "sam_nodes.h"

namespace taco {
    class TensorVar;
    class IndexVar;

namespace sam {
    struct RootNode;
    struct BroadcastNode;
    struct FiberLookupNode;
    struct FiberWriteNode;
    struct RepeatNode;
    struct RepeatSigGenNode;
    struct IntersectNode;
    struct UnionNode;
    struct ArrayNode;
    struct AddNode;
    struct MulNode;
    struct ReduceNode;
    struct SparseAccumulatorNode;

    class SamIR;
    class SAMVisitorStrict;

    /// Return true if the index statement is of the given subtype.
    template <typename SubType> bool isa(SamIR);

    template <typename SubType> SubType to(SamIR);

    class SamIR : public util::IntrusivePtr<const SAMNode> {
    public:
        SamIR() : util::IntrusivePtr<const SAMNode>(nullptr) {}
        explicit SamIR(const SAMNode* n) : util::IntrusivePtr<const SAMNode>(n) {}

        void accept(SAMVisitorStrict *v) const;

        /// Casts index expression to specified subtype.
        template<typename SubType>
        SubType as() {
            return to<SubType>(*this);
        }

        /// Print the index expression.
        friend std::ostream &operator<<(std::ostream &, const SamIR &);
    };

    class FiberLookup : public SamIR {
    public:
        FiberLookup() = default;

        explicit FiberLookup(const FiberLookupNode *);


        FiberLookup(SamIR out_ref, SamIR out_crd, IndexVar i, TensorVar tensorVar, int mode, int nodeID,
                    bool root=false, bool source=true);

        typedef FiberLookupNode Node;

        bool outputsDefined() const;

        SamIR getOutRef() const;

        SamIR getOutCrd() const;

        TensorVar getTensorVar() const;

        IndexVar getIndexVar() const;
    };

    class FiberWrite : public SamIR {
    public:
        FiberWrite() = default;

        explicit FiberWrite(const FiberWriteNode *);

        FiberWrite(IndexVar i, TensorVar tensorVar, int mode, int nodeID, bool sink=true, bool vals=false);

        typedef FiberWriteNode Node;

    };

    class Repeat : public SamIR {
    public:
        Repeat() = default;

        explicit Repeat(const RepeatNode *);

        Repeat(SamIR out_ref, IndexVar i, TensorVar tensorVar, int nodeID, bool root=false);

        typedef RepeatNode Node;

    };

    class RepeatSigGen : public SamIR {
    public:
        RepeatSigGen() = default;

        explicit RepeatSigGen(const RepeatSigGenNode *);

        RepeatSigGen(SamIR out_repsig, IndexVar i, int nodeID);

        typedef RepeatSigGenNode Node;

    };

    class Broadcast : public SamIR {
    public:
        Broadcast() = default;

        explicit Broadcast(const BroadcastNode *);

        Broadcast(std::vector<SamIR> outputs, SamEdgeType type, int nodeID);

        typedef BroadcastNode Node;
    };

    class Root : public SamIR {
    public:
        Root() = default;

        explicit Root(const RootNode *);

        Root(std::vector<SamIR> nodes);

        typedef RootNode Node;

    };

    class Intersect : public SamIR {
    public:
        Intersect() = default;

        explicit Intersect(const IntersectNode *);

        Intersect(SamIR out_crd, std::vector<SamIR> out_refs, IndexVar i, int nodeID);

        typedef IntersectNode Node;

    };

    class Union : public SamIR {
    public:
        Union() = default;

        explicit Union(const UnionNode *);

        Union(SamIR out_crd, std::vector<SamIR> out_refs, IndexVar i, int nodeID);

        typedef UnionNode Node;

    };

    class Array : public SamIR {
    public:
        Array() = default;

        explicit Array(const ArrayNode *);

        Array(SamIR out_val, TensorVar tensorVar, int nodeID);

        typedef ArrayNode Node;

    };

    class Mul : public SamIR {
    public:
        Mul() = default;

        explicit Mul(const taco::sam::MulNode *);

        Mul(SamIR out_val, int nodeID);

        typedef taco::sam::MulNode Node;

    };

    class Add : public SamIR {
    public:
        Add() = default;

        explicit Add(const taco::sam::AddNode *);

        Add(SamIR out_val, int nodeID);

        typedef taco::sam::AddNode Node;

    };

    class Reduce : public SamIR {
    public:
        Reduce() = default;

        explicit Reduce(const ReduceNode *);

        Reduce(SamIR out_val, int nodeID);

        typedef ReduceNode Node;

    };

    class SparseAccumulator : public SamIR {
    public:
        SparseAccumulator() = default;

        explicit SparseAccumulator(const SparseAccumulatorNode *);

        SparseAccumulator(SamIR out_val, int order, int nodeID);

        typedef SparseAccumulatorNode Node;

    };
}
}

#endif //TACO_SAM_IR_H

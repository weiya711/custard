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

    struct FiberLookupNode;
    struct FiberWriteNode;
    struct RepeatNode;
    struct RootNode;
    struct IntersectNode;
    struct UnionNode;

    class SamIR;
    class SAMVisitorStrict;

    /// Return true if the index statement is of the given subtype.
    template <typename SubType> bool isa(SamIR);

    template <typename SubType> SubType to(SamIR);

    class SamIR : public util::IntrusivePtr<const SAMNode> {
    public:
        SamIR() : util::IntrusivePtr<const SAMNode>(nullptr) {}
        SamIR(const SAMNode* n) : util::IntrusivePtr<const SAMNode>(n) {}

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

        FiberLookup(const FiberLookupNode *);

        FiberLookup(SamIR in_ref, IndexVar i, TensorVar tensorVar, int mode, bool root=false, bool source=true);

        FiberLookup(SamIR in_ref, SamIR out_ref, SamIR out_crd, IndexVar i, TensorVar tensorVar, int mode, bool root=false, bool source=true);

        typedef FiberLookupNode Node;

        bool outputsDefined() const;

        SamIR getInRef() const;

        SamIR getOutRef() const;
        void setOutRef();

        SamIR getOutCrd() const;
        void setOutCrd();

        TensorVar getTensorVar() const;

        IndexVar getIndexVar() const;
    };

    class FiberWrite : public SamIR {
    public:
        FiberWrite() = default;

        explicit FiberWrite(const FiberWriteNode *);

        FiberWrite(SamIR in_crd, IndexVar i, TensorVar tensorVar, int mode, bool sink=true, bool vals=false);

        typedef FiberWriteNode Node;

    };

    class Repeat : public SamIR {
    public:
        Repeat() = default;

        explicit Repeat(const RepeatNode *);

        Repeat(SamIR in_crd, SamIR in_repsig, IndexVar i, TensorVar tensorVar, bool root=false);

        Repeat(SamIR in_crd, SamIR in_repsig, SamIR out_ref, IndexVar i, TensorVar tensorVar, bool root=false);

        typedef RepeatNode Node;

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

        Intersect(SamIR out_crd, SamIR out_a_ref, SamIR out_b_ref, IndexVar i);

        typedef IntersectNode Node;

    };

    class Union : public SamIR {
    public:
        Union() = default;

        explicit Union(const UnionNode *);

        Union(SamIR out_crd, SamIR out_a_ref, SamIR out_b_ref, IndexVar i);

        typedef UnionNode Node;

    };
}
}

#endif //TACO_SAM_IR_H

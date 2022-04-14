#ifndef TACO_SAM_VISITOR_H
#define TACO_SAM_VISITOR_H

namespace taco {
namespace sam {

    struct FiberLookupNode;
    struct FiberWriteNode;
    struct RepeatNode;
    struct RepeatSigGenNode;
    struct IntersectNode;
    struct UnionNode;
    struct ArrayNode;
    struct MulNode;
    struct AddNode;
    struct ReduceNode;
    struct SparseAccumulatorNode;

    class SAMVisitorStrict {
    public:
        virtual ~SAMVisitorStrict();
//        void visit(const SamIRNode &);

        virtual void visit(const FiberLookupNode *) = 0;
        virtual void visit(const FiberWriteNode *) = 0;

        virtual void visit(const RepeatNode *) = 0;
        virtual void visit(const RepeatSigGenNode *) = 0;

        virtual void visit(const IntersectNode *) = 0;
        virtual void visit(const UnionNode *) = 0;

        virtual void visit(const ArrayNode *) = 0;

        virtual void visit(const AddNode *) = 0;
        virtual void visit(const MulNode *) = 0;

        virtual void visit(const ReduceNode *) = 0;
        virtual void visit(const SparseAccumulatorNode *) = 0;
    };

/// Visit nodes in an expression.
    class SAMVisitor : public SAMVisitorStrict {
    public:
        virtual ~SAMVisitor();

        using SAMVisitorStrict::visit;

        virtual void visit(const FiberLookupNode *);
        virtual void visit(const FiberWriteNode *);

        virtual void visit(const RepeatNode *);
        virtual void visit(const RepeatSigGenNode *);

        virtual void visit(const IntersectNode *);
        virtual void visit(const UnionNode *);

        virtual void visit(const ArrayNode *);

        virtual void visit(const AddNode *);
        virtual void visit(const MulNode *);

        virtual void visit(const ReduceNode *);

        virtual void visit(const SparseAccumulatorNode *);
    };

// TODO: Create matcher class for SAM Visitors
    }
}
#endif //TACO_SAM_VISITOR_H

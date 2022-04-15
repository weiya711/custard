//
// Created by oliviahsu on 4/14/22.
//

#include "sam_printer.h"
#include "sam_nodes.h"
using namespace std;

namespace taco {
namespace sam {
    // SAMPrinter
    void SAMPrinter::visit(const FiberLookupNode *op) {
        os << op->getName();
        os << "->";
        op->out_crd.accept(this);
    }

    void SAMPrinter::print(const SamIR &sam) {
        sam.accept(this);
    }


    void SAMDotNodePrinter::print(const SamIR &sam) {
        sam.accept(this);
    }


    // SAMDotNodePrinter
    void SAMDotNodePrinter::visit(const RootNode *op) {
        os << "digraph " << name << " {" << endl;
        for (auto node : op->nodes) {
            node.accept(this);
        }
    }

    void SAMDotNodePrinter::visit(const FiberLookupNode *op) {
        id++;

        string src = op->source ? "_src" : "";
        string root = op->root ? "_root" : "";

        std::stringstream comment;
        comment << "\"fiber_lookup-" << op->i.getName() << "_" << op->tensorVar.getName() << std::to_string(op->mode)
        << "_" << op->modeFormat.getName() << src << root << "\"";

        os << tab;
        os << to_string(id) << " [comment=" << comment.str();
        if (prettyPrint) {
            os << " label=\"" << op->getName() << "\"";
            os << " color=green4 shape=box style=filled";
        }
        os << "]" << endl;

        if (op->out_crd.defined()) {
            op->out_crd.accept(this);
        }

        if (op->out_ref.defined()) {
            op->out_ref.accept(this);
        }

    }

    void SAMDotNodePrinter::visit(const FiberWriteNode *op) {
        id++;

        string sink = op->sink ? "_sink" : "";

        std::stringstream comment;
        comment << "\"fiber_write-" << op->i.getName() << "_" << op->tensorVar.getName() << std::to_string(op->mode)
                << "_" << op->modeFormat.getName() << sink << "\"";

        os << tab;
        os << to_string(id) << " [comment=" << comment.str();
        if (prettyPrint) {
            os << " label=\"" << op->getName() << "\"";
            os << " color=green3 shape=box style=filled";
        }
        os << "]" << endl;
    }

    void SAMDotNodePrinter::visit(const RepeatNode *op) {
        id++;

        string root = op->root ? "_root" : "";

        std::stringstream comment;
        comment << "\"repeat-" << op->i.getName() << "_" << op->tensorVar.getName() << root << "\"";

        os << tab;
        os << to_string(id) << " [comment=" << comment.str();
        if (prettyPrint) {
            os << " label=\"" << op->getName() << "\"";
            os << " color=cyan2 shape=box style=filled";
        }
        os << "]" << endl;

        if (op->out_ref.defined()) {
            op->out_ref.accept(this);
        }
    }

    void SAMDotNodePrinter::visit(const JoinerNode *op) {
        id++;

        std::stringstream comment;
        comment << "\"" << op->getNodeName() << "-" << op->i.getName() << "\"";

        os << tab;
        os << to_string(id) << " [comment=" << comment.str();
        if (prettyPrint) {
            os << " label=\"" << op->getName() << "\"";
            os << " color=purple shape=box style=filled";
        }
        os << "]" << endl;

        if (op->out_crd.defined()) {
            op->out_crd.accept(this);
        }

        if (op->out_a_ref.defined()) {
            op->out_a_ref.accept(this);
        }

        if (op->out_b_ref.defined()) {
            op->out_b_ref.accept(this);
        }
    }

    void SAMDotEdgePrinter::print(const SamIR &sam) {
        sam.accept(this);
    }

    void SAMDotEdgePrinter::visit(const RootNode *op) {
        for (auto node : op->nodes) {
            node.accept(this);
        }
        cout << endl << "}" << endl;
    }

    void SAMDotEdgePrinter::visit(const FiberLookupNode *op) {
        id++;
        int nodeID = id;

        if (!op->root) {
            stringstream ss;
            if (!edgeType.empty()) {
                ss << " [label=\"" << edgeType << "\"";
                if (prettyPrint) {
                    ss << " " << edgeStyle[edgeType];
                }
                ss << "]";
            }
            os << nodeID << ss.str() << endl;
        }

        if (op->out_crd.defined()) {
            edgeType = "crd";
            os << tab << nodeID << " -> ";
            op->out_crd.accept(this);
        }

        if (op->out_ref.defined()) {
            edgeType = "ref";
            os << tab << nodeID << " -> ";
            op->out_ref.accept(this);
        }

        edgeType = "";
    }

    void SAMDotEdgePrinter::visit(const FiberWriteNode *op) {
        id++;
        int nodeID = id;

        stringstream ss;
        if (!edgeType.empty()) {
            ss << " [label=\"" << edgeType << "\"";
            if (prettyPrint) {
                ss << " " << edgeStyle[edgeType];
            }
            ss << "]";
        }
        os << nodeID << ss.str() << endl;

        edgeType = "";

    }

    void SAMDotEdgePrinter::visit(const RepeatNode *op) {
        id++;
        int nodeID = id;

        if (!op->root) {
            stringstream ss;
            if (!edgeType.empty()) {
                ss << " [label=\"" << edgeType << "\"";
                if (prettyPrint) {
                    ss << " " << edgeStyle[edgeType];
                }
                ss << "]";
            }
            os << nodeID << ss.str() << endl;
        }

        if (op->out_ref.defined()) {
            edgeType = "ref";
            os << tab << nodeID << " -> ";
            op->out_ref.accept(this);
        }
        edgeType = "";
    }

    void SAMDotEdgePrinter::visit(const JoinerNode *op) {
        id++;
        int nodeID = id;

        stringstream ss;
        if (!edgeType.empty()) {
            ss << " [label=\"" << edgeType << "\"";
            if (prettyPrint) {
                ss << " " << edgeStyle[edgeType];
            }
            ss << "]";
        }
        os << nodeID << ss.str() << endl;

        if (op->out_crd.defined()) {
            edgeType = "crd";
            os << tab << nodeID << " -> ";
            op->out_crd.accept(this);
        }

        if (op->out_a_ref.defined()) {
            edgeType = "ref";
            os << tab << nodeID << " -> ";
            op->out_a_ref.accept(this);
        }

        if (op->out_b_ref.defined()) {
            edgeType = "ref";
            os << tab << nodeID << " -> ";
            op->out_b_ref.accept(this);
        }
    }

}
}

#include "sam_graph.h"

#include <set>
#include <vector>
#include <queue>
#include <functional>

#include "taco/index_notation/index_notation.h"
#include "taco/index_notation/index_notation_nodes.h"
#include "taco/index_notation/index_notation_visitor.h"
#include "taco/index_notation/schedule.h"
#include "lower/iteration_forest.h"
#include "lower/tensor_path.h"
#include "taco/util/strings.h"
#include "taco/util/collections.h"
#include "sam_printer.h"

using namespace std;

namespace taco {
    using namespace sam;

// class SAMGraph
    struct SAMGraph::Content {
        Content(IterationForest iterationForest, const vector<IndexVar>& freeVars,
                TensorPath resultTensorPath, vector<TensorPath> tensorPaths,
                map<IndexExpr,TensorPath> mapAccessNodesToPaths, IndexExpr expr)
                : iterationForest(iterationForest),
                  freeVars(freeVars.begin(), freeVars.end()),
                  resultTensorPath(resultTensorPath),
                  tensorPaths(tensorPaths),
                  accessNodesToPaths(mapAccessNodesToPaths),
                  expr(expr) {
        }
        IterationForest           iterationForest;
        set<IndexVar>             freeVars;

        TensorPath                resultTensorPath;
        vector<TensorPath>        tensorPaths;

        vector<TensorVar>         workspaces;

        map<IndexExpr,TensorPath> accessNodesToPaths;

        // TODO: This must be replaced by a map that maps each index variable to a
        //       (potentially rewritten) index expression.
        IndexExpr                 expr;
    };

    SAMGraph::SAMGraph() {
    }

    SAMGraph SAMGraph::make(Assignment assignment) {
        TensorVar tensor = assignment.getLhs().getTensorVar();
        IndexExpr expr = assignment.getRhs();

        vector<TensorPath> tensorPaths;
        vector<TensorVar> workspaces;
        map<IndexExpr,TensorPath> accessNodesToPaths;

        map<IndexVar,Dimension> indexVarDomains = assignment.getIndexVarDomains();

        map<IndexVar,IndexVar> oldToSplitVar;  // remap split index variables
        for (auto& indexVarRange : indexVarDomains) {
            auto indexVar = indexVarRange.first;
            oldToSplitVar.insert({indexVar, indexVar});
        }

        match(expr,
              function<void(const AccessNode*)>([&](const AccessNode* op) {
                  auto type = op->tensorVar.getType();
                  taco_iassert((size_t)type.getShape().getOrder() == op->indexVars.size())
                          << "Tensor access " << IndexExpr(op) << " but tensor format only has "
                          << type.getShape().getOrder() << " modes.";
                  Format format = op->tensorVar.getFormat();

                  // copy index variables to path
                  vector<IndexVar> path(op->indexVars.size());
                  for (size_t i=0; i < op->indexVars.size(); ++i) {
                      int ordering = op->tensorVar.getFormat().getModeOrdering()[i];
                      path[i] = oldToSplitVar.at(op->indexVars[ordering]);
                  }

                  TensorPath tensorPath(path, op);
                  accessNodesToPaths.insert({op, tensorPath});
                  tensorPaths.push_back(tensorPath);
              })
        );

        auto freeVars = assignment.getFreeVars();
        vector<IndexVar> resultVars;
        for (int i = 0; i < tensor.getType().getShape().getOrder(); ++i) {
            size_t idx = tensor.getFormat().getModeOrdering()[i];
            resultVars.push_back(freeVars[idx]);
        }
        TensorPath resultPath = TensorPath(resultVars, Access(tensor, freeVars));

        // Construct a forest decomposition from the tensor path graph
        IterationForest forest =
                IterationForest(util::combine({resultPath}, tensorPaths));

        // Create the iteration graph
        SAMGraph samGraph = SAMGraph();
        samGraph.content =
                make_shared<SAMGraph::Content>(forest, freeVars,
                                                     resultPath, tensorPaths,
                                                     accessNodesToPaths, expr);
        return samGraph;
    }

    const std::vector<IndexVar>& SAMGraph::getRoots() const {
        return content->iterationForest.getRoots();
    }

    const std::vector<IndexVar>&
    SAMGraph::getChildren(const IndexVar& var) const {
        return content->iterationForest.getChildren(var);
    }

    const IndexVar& SAMGraph::getParent(const IndexVar& var) const {
        return content->iterationForest.getParent(var);
    }

    vector<IndexVar> SAMGraph::getAncestors(const IndexVar& var) const {
        std::vector<IndexVar> ancestors;
        ancestors.push_back(var);
        IndexVar parent = var;
        while (content->iterationForest.hasParent(parent)) {
            parent = content->iterationForest.getParent(parent);
            ancestors.push_back(parent);
        }
        return ancestors;
    }

    vector<IndexVar> SAMGraph::getDescendants(const IndexVar& var) const{
        vector<IndexVar> descendants;
        descendants.push_back(var);
        for (auto& child : getChildren(var)) {
            util::append(descendants, getDescendants(child));
        }
        return descendants;
    }

    const vector<TensorPath>& SAMGraph::getTensorPaths() const {
        return content->tensorPaths;
    }

    const TensorPath&
    SAMGraph::getTensorPath(const IndexExpr& operand) const {
        taco_iassert(util::contains(content->accessNodesToPaths, operand));
        return content->accessNodesToPaths.at(operand);
    }

    const TensorPath& SAMGraph::getResultTensorPath() const {
        return content->resultTensorPath;
    }

    IndexVarType SAMGraph::getIndexVarType(const IndexVar& var) const {
        return (util::contains(content->freeVars, var))
               ? IndexVarType::Free : IndexVarType::Sum;
    }

    bool SAMGraph::isFree(const IndexVar& var) const {
        return getIndexVarType(var) == IndexVarType::Free;
    }

    bool SAMGraph::isReduction(const IndexVar& var) const {
        return !isFree(var);
    }

    bool SAMGraph::isLastFreeVariable(const IndexVar& var) const {
        return isFree(var) && !hasFreeVariableDescendant(var);
    }

    bool SAMGraph::hasFreeVariableDescendant(const IndexVar& var) const {
        // Traverse the iteration forest subtree of var to determine whether it has
        // any free variable descendants
        auto children = content->iterationForest.getChildren(var);
        for (auto& child : children) {
            if (isFree(child)) {
                return true;
            }
            // Child is not free; check if it a free descendent
            if (hasFreeVariableDescendant(child)) {
                return true;
            }
        }
        return false;
    }

    bool SAMGraph::hasReductionVariableAncestor(const IndexVar& var) const {
        if (isReduction(var)) {
            return true;
        }

        IndexVar parent = var;
        while (content->iterationForest.hasParent(parent)) {
            parent = content->iterationForest.getParent(parent);
            if (isReduction(parent)) {
                return true;
            }
        }
        return false;
    }

    const IndexExpr& SAMGraph::getIndexExpr(const IndexVar&) const {
        return content->expr;
    }

    std::vector<IndexVar> SAMGraph::getIndexVars() const {
        return content->iterationForest.getNodes();
    }

    std::vector<IndexVar> SAMGraph::getOrderedIndexVars() const {
        vector<IndexVar> indexVars;
        int count = 0;

        auto roots = content->iterationForest.getRoots();

        if (roots.size() > 0) {
            indexVars.insert(indexVars.end(), roots.begin(), roots.end());

            while (count < (int) indexVars.size()) {
                auto indexvar = indexVars.at(count);

                if (content->iterationForest.hasChildren(indexvar)) {
                    auto children = content->iterationForest.getChildren(indexvar);
                    indexVars.insert(indexVars.end(), children.begin(), children.end());
                }
                count++;
            }
        }
        return indexVars;
    }

    map<IndexVar, vector<TensorVar>> SAMGraph::getContractions() const {
        std::map<IndexVar, std::vector<TensorVar>> contractMap;
        for (const auto& indexvar : getOrderedIndexVars()) {
            vector<TensorVar> tensorList;
            for (auto& tensorPath : getTensorPaths()) {
                auto vars = tensorPath.getVariables();
                auto tensor = tensorPath.getAccess().getTensorVar();

                if (std::count(vars.begin(), vars.end(), indexvar) > 0) {
                    tensorList.emplace_back(tensor);
                }
            }
            contractMap[indexvar] = tensorList;
        }
        return contractMap;
    }

    vector<SamNodeType> SAMGraph::getComputation() const {
        vector<SamNodeType> compute;
        match(content->expr,
              function<void(const taco::MulNode*)>([&](const taco::MulNode* op) {
                  compute.push_back(SamNodeType::Mul);
              }),
              function<void(const taco::AddNode*)>([&](const taco::AddNode* op) {
                  compute.push_back(SamNodeType::Add);
              })
        );

        auto indexVars = getIndexVars();
        auto isInnerReduction = true;
        for (std::vector<IndexVar>::reverse_iterator it = indexVars.rbegin(); it != indexVars.rend(); it++) {
            if (isReduction(*it) && isInnerReduction) {
                compute.push_back(SamNodeType::Reduce);
            } else if (isReduction(*it)) {
                compute.push_back(SamNodeType::SparseAccumulator);
            } else {
                isInnerReduction = false;
            }
        }

        return compute;
    }

    void SAMGraph::printComputation(std::ostream& os) {
        auto compute = getComputation();
        for (auto it : compute) {
            os << (int)it << endl;
        }
    }

    void SAMGraph::printContractions(std::ostream& os) {
        auto contractMap = getContractions();

        for (auto p : contractMap) {
            os << get<0>(p) << ": " << util::join(get<1>(p)) << endl;
        }
    }

    void SAMGraph::printOutputIteration(std::ostream& os) {
        auto& resultPath = getResultTensorPath();
        string resultName = resultPath.getAccess().getTensorVar().getName();
        auto& resultVars = resultPath.getVariables();

        os << " -> " << resultName << " ValsWrite" << endl;
        for (auto indexvar : resultVars) {
            os << " -> " << resultName << "_" << indexvar << " FiberWrite " << endl;
        }
        os << endl;
    }

    map<IndexVar, ModeFormat> SAMGraph::getFormatMapping(const TensorPath path) {
        map<IndexVar, ModeFormat> result;
        auto tensor = path.getAccess().getTensorVar();
        taco_iassert(tensor.getFormat().getModeOrdering().size() == path.getVariables().size());

        for (int i = 0; i < (int)path.getVariables().size(); i++) {
            auto indexvar = path.getVariables()[i];
            auto format = tensor.getFormat().getModeFormats()[i];
            result[indexvar] = format;
        }
        return result;
    }

    SamIR SAMGraph::makeInputIterationGraph() {
        int id = 0;

        vector<SamIR> rootNodes;

        int numIndexVars = (int)getOrderedIndexVars().size();

        vector<TensorVar> inputTensors;
        for (auto tensorPath : getTensorPaths()) {
            inputTensors.push_back(tensorPath.getAccess().getTensorVar());
        }

        // Output Assignment
        map<IndexVar, SamIR> resultWriteIRNodes;
        map<IndexVar, bool> resultHasSource;
        auto resultVars = getResultTensorPath().getVariables();
        int mode = (int)resultVars.size() - 1;
        for (int count = 0; count < (int) getOrderedIndexVars().size(); count++) {
            IndexVar indexvar = getOrderedIndexVars().at(getOrderedIndexVars().size() - 1 - count);
            if (std::count(resultVars.begin(), resultVars.end(), indexvar) > 0) {
                auto node = FiberWrite(indexvar, getResultTensorPath().getAccess().getTensorVar(), mode, id,
                                       true);
                id++;
                mode--;
                resultWriteIRNodes[indexvar] = node;
                resultHasSource[indexvar] = false;
            }

        }

        SamIR resultWriteVals = FiberWrite(nullptr, getResultTensorPath().getAccess().getTensorVar(), 0, id,
                                           true, true);
        id++;

        // TODO: FINISH THIS
        // Elementwise Compute Operations
        vector<SamNodeType> compute;
        match(content->expr,
              function<void(const taco::MulNode*,Matcher*)>([&](
                      const taco::MulNode* op, Matcher* ctx) {
                  compute.push_back(SamNodeType::Mul);
                  ctx->match(op->a);
                  ctx->match(op->b);
              }),
              function<void(const taco::AddNode*,Matcher*)>([&](
                      const taco::AddNode* op, Matcher* ctx) {
                  compute.push_back(SamNodeType::Add);
                  ctx->match(op->a);
                  ctx->match(op->b);
              }));

        // Reduction Operations
        auto isInnerReduction = true;
        vector<int> reductionOrder;
        for (int count = 0; count < numIndexVars; count++) {
            IndexVar indexvar = getOrderedIndexVars().at(numIndexVars - 1 - count);
            if (isReduction(indexvar) && isInnerReduction) {
                compute.push_back(SamNodeType::Reduce);
                reductionOrder.push_back(count);
            } else if (isReduction(indexvar) && !isInnerReduction) {
                compute.push_back(SamNodeType::SparseAccumulator);
                reductionOrder.push_back(count);
                isInnerReduction = false;
            } else {
                isInnerReduction = false;
            }
        }

        SamIR computeNode = SamIR();
        if (!compute.empty()) {
            auto prevComputeNode = resultWriteVals;
            for (auto it =  compute.rbegin(); it != compute.rend(); it++) {
                auto computation = *it;
                switch (computation) {
                    case SamNodeType::Mul:
                        computeNode = taco::sam::Mul(prevComputeNode, id);
                        break;
                    case SamNodeType::Add:
                        computeNode = taco::sam::Add(prevComputeNode, id);
                        break;
                    case SamNodeType::Reduce:
                        computeNode = taco::sam::Reduce(prevComputeNode, id);
                        taco_iassert(reductionOrder.back() == 0) << "Reduce node must have a reduction order of 0";
                        taco_iassert(!reductionOrder.empty()) << "Number of reduction (Reduction) nodes does not "
                                                                 "match the number of reduction orders.";
                        reductionOrder.pop_back();
                        break;
                    case SamNodeType::SparseAccumulator:
                        // FIXME: make sure order is correct (currently isn't)
                        taco_iassert(!reductionOrder.empty()) << "Number of reduction (Sparse Accumulation) nodes does not "
                                                                 "match the number of reduction orders.";
                        computeNode = taco::sam::SparseAccumulator(prevComputeNode, reductionOrder.back(), id);
                        reductionOrder.pop_back();
                        break;
                    default:
                        break;
                }
                prevComputeNode = computeNode;
                id++;
            }
        }

        // Values Arrays
        map<TensorVar, SamIR> inputValsArrays;

        for (auto tensor : inputTensors) {
            auto array = taco::sam::Array(compute.empty() ? resultWriteVals : computeNode, tensor, id);
            id++;
            inputValsArrays[tensor] = array;
        }


        // Tensor Contractions
        // FIXME: this code assumes 2 input operands at a time
        map<vector<TensorVar>, bool> contractionType;
        match(content->expr,
              function<void(const taco::MulNode*,Matcher*)>([&](
                      const taco::MulNode* op, Matcher* ctx) {
                  if (isa<Access>(op->a) && isa<Access>(op->b)) {
                      vector<TensorVar> tensors;
                      tensors.push_back(to<Access>(op->a).getTensorVar());
                      tensors.push_back(to<Access>(op->b).getTensorVar());
                      contractionType[tensors] = true;
                  } else {
                      ctx->match(op->a);
                      ctx->match(op->b);
                  }
              }),
              function<void(const taco::AddNode*,Matcher*)>([&](
                      const taco::AddNode* op, Matcher* ctx) {
                  if (isa<Access>(op->a) && isa<Access>(op->b)) {
                      vector<TensorVar> tensors;
                      tensors.push_back(to<Access>(op->a).getTensorVar());
                      tensors.push_back(to<Access>(op->b).getTensorVar());
                      contractionType[tensors] = false;
                  } else {
                      ctx->match(op->a);
                      ctx->match(op->b);
                  }
              })
        );

        std::map<IndexVar, std::vector<TensorVar>> contractions;
        for (const auto& indexvar : getOrderedIndexVars()) {
            vector<TensorVar> tensorList;
            for (auto& tensorPath : getTensorPaths()) {
                auto vars = tensorPath.getVariables();
                auto tensor = tensorPath.getAccess().getTensorVar();

                if (std::count(vars.begin(), vars.end(), indexvar) > 0) {
                    tensorList.emplace_back(tensor);
                }
            }
            contractions[indexvar] = tensorList;
        }


        // Input Iteration without Contractions
        map<IndexVar, vector<SamIR>> nodeMap;
        for (int count = 0; count < numIndexVars; count++) {

            IndexVar indexvar = getOrderedIndexVars().at(numIndexVars - 1 - count);
            bool isRoot = count == numIndexVars - 1;
            IndexVar prevIndexVar = count == 0 ? nullptr : getOrderedIndexVars().at(numIndexVars - count);

            auto crdDest = contains(resultHasSource,indexvar) && !resultHasSource.at(indexvar) ?
                           resultWriteIRNodes.at(indexvar) : SamIR();

            bool hasContraction = contractions.at(indexvar).size() > 1;
            // FIXME: This will eventually need to be the iteration algebra for fused kernels
            bool isIntersection = contains(contractionType, contractions.at(indexvar)) &&
                                  contractionType.at(contractions.at(indexvar));

            vector<SamIR> nodes(getTensorPaths().size());
            vector<SamIR> repeatNodes;
            for (int ntp = 0; ntp < (int) getTensorPaths().size(); ntp++) {
                SamIR node;

                TensorPath tensorPath = getTensorPaths().at(ntp);
                auto tensorVar = tensorPath.getAccess().getTensorVar();

                auto vars = tensorPath.getVariables();

                // FIXME: see if repeat nodes are ever a root
                if (std::count(vars.begin(), vars.end(), indexvar) == 0) {
                    if (count == 0) {
                        node = Repeat(inputValsArrays[tensorVar], indexvar, tensorVar, id, false);
                    } else {
                        auto prevSAMNode = nodeMap[prevIndexVar][ntp];
                        node = Repeat(prevSAMNode, indexvar, tensorVar, id, false);
                    }
                    id++;
                    nodes[ntp] = node;
                    repeatNodes.push_back(node);
                }

            }

            // Repeats exist for this indexvar
            SamIR repeatSigGenNode;
            if (repeatNodes.size() > 1) {
                auto broadcast = Broadcast(repeatNodes, sam::SamEdgeType::repsig, id);
                id++;
                repeatSigGenNode = RepeatSigGen(broadcast, indexvar, id);
                id++;
            } else if (repeatNodes.size() > 0) {
                repeatSigGenNode = RepeatSigGen(repeatNodes.at(0), indexvar, id);
                id++;
            }

            // if a RepSigGen, the destination coordinate needs to be broadcasted to the RepSigGen block
            if(repeatSigGenNode.defined() && crdDest.defined()) {
                    crdDest = Broadcast({crdDest, repeatSigGenNode}, sam::SamEdgeType::crd, id);
                    id++;
            } else if (repeatSigGenNode.defined()) {
                crdDest = repeatSigGenNode;
            }


            // FIXME: Assumes 2 input operands
            SamIR contractNode;
            if (hasContraction) {
                vector<SamIR> contractOuts;
                if (prevIndexVar.defined()) {
                    contractOuts = nodeMap[prevIndexVar];
                } else {
                    for (const auto& arrs: inputValsArrays) {
                        contractOuts.push_back(arrs.second);
                    }
                }

                if (isIntersection)
                    contractNode = taco::sam::Intersect(crdDest, contractOuts, indexvar, id);
                else
                    contractNode = taco::sam::Union(crdDest, contractOuts, indexvar, id);
                id++;
            }

            for (int ntp = 0; ntp < (int) getTensorPaths().size(); ntp++) {
                SamIR node;

                TensorPath tensorPath = getTensorPaths().at(ntp);
                auto tensorVar = tensorPath.getAccess().getTensorVar();

                auto formats = getFormatMapping(tensorPath);

                auto vars = tensorPath.getVariables();

                auto it = find(vars.begin(), vars.end(), indexvar);
                mode = it != vars.end() ? (int) distance(vars.begin(), it) : 9999;

                if (std::count(vars.begin(), vars.end(), indexvar) > 0) {

                    if (count == 0) {
                        node = FiberLookup(hasContraction ? contractNode : inputValsArrays[tensorVar], hasContraction ? contractNode : crdDest,
                                           indexvar, tensorVar, mode,  id, isRoot, true, hasContraction);
                    } else {
                        auto prevSAMNode = hasContraction ? contractNode : nodeMap[prevIndexVar][ntp];
                        node = FiberLookup(prevSAMNode, hasContraction ? contractNode : crdDest,
                                           indexvar, tensorVar, mode, id, isRoot, true, hasContraction);
                    }

                    mode--;
                    id++;
                    nodes[ntp] = node;

                    if (count == numIndexVars - 1) {
                        rootNodes.push_back(node);
                    }
                }
            }
            nodeMap[indexvar] = nodes;
        }


        taco_iassert(numIndexVars > 0);
        auto root = Root(rootNodes);
        return root;
    }

    void SAMGraph::printInputIteration(std::ostream& os) {
        for (auto& tensorPath : getTensorPaths()) {
            auto tensorName = tensorPath.getAccess().getTensorVar().getName();
            cout << tensorPath.getAccess().getTensorVar().getFormat().getModeFormats() << endl;
            cout << util::join(tensorPath.getAccess().getTensorVar().getFormat().getModeOrdering()) << endl;


            os << "  " << " start input " <<  tensorName <<  " \t-> ";
            auto formats = getFormatMapping(tensorPath);
            for (auto indexvar : getOrderedIndexVars()) {
                auto vars = tensorPath.getVariables();
                if (std::count(vars.begin(), vars.end(), indexvar) > 0) {
                    os << " " << tensorName << "_" << indexvar << " " << formats[indexvar] << ": FiberLookup" << " \t->";
                } else {
                    os << " RepeatSigGen -> " << tensorName << "_" << indexvar << " Repeat \t->";
                }
            }
            os << endl;
        }
    }

    void SAMGraph::printInputIterationAsDot(std::ostream& os) {
        auto sam = makeInputIterationGraph();
        SAMDotNodePrinter printer(os);
        printer.print(sam);

        SAMDotEdgePrinter printerEdge(os);
        printerEdge.print(sam);
    }

    void SAMGraph::printAsDot(std::ostream& os) {
        os << "digraph {";
        os << "\n root [label=\"\" shape=none]";

        for (auto& path : getTensorPaths()) {
            string name = path.getAccess().getTensorVar().getName();
            auto& vars = path.getVariables();
            if (vars.size() > 0) {
                os << "\n root -> " << vars[0]
                   << " [label=\"" << name << "\"]";
            }
        }

        auto& resultPath = getResultTensorPath();
        string resultName = resultPath.getAccess().getTensorVar().getName();
        auto& resultVars = resultPath.getVariables();
        if (resultVars.size() > 0) {
            os << "\n root -> " << resultVars[0]
               << " [style=dashed label=\"" << resultName << "\"]";
        }

        for (auto& path : getTensorPaths()) {
            string name = path.getAccess().getTensorVar().getName();
            auto& vars = path.getVariables();
            for (size_t i = 1; i < vars.size(); i++) {
                os << "\n " << vars[i-1] << " -> " << vars[i]
                   << " [label=\"" << name << "\"]";
            }
        }

        for (size_t i = 1; i < resultVars.size(); i++) {
            os << "\n " << resultVars[i-1] << " -> " << resultVars[i]
               << " [style=dashed label=\"" << resultName << "\"]";
        }
        os << "\n}\n";
        os.flush();
    }

    std::ostream& operator<<(std::ostream& os, const SAMGraph& graph) {
        os << "Index Variable Forest" << std::endl;
        os << graph.content->iterationForest << std::endl;
        os << "Result tensor path" << std::endl;
        os << "  " << graph.getResultTensorPath() << std::endl;
        os << "Tensor paths:" << std::endl;
        for (auto& tensorPath : graph.getTensorPaths()) {
            os << "  " << tensorPath << std::endl;
        }
        os << "Free Variables" << std::endl;
        for (auto n : graph.content->iterationForest.getNodes())
            os << graph.isFree(n) << ",";
        os << std::endl;
        os << "Reduction Variables" << std::endl;
        for (auto n : graph.content->iterationForest.getNodes())
            os << graph.isReduction(n) << ",";
        os << std::endl;
        return os;
    }

}

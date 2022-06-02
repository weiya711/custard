#include "sam_graph.h"

#include <set>
#include <vector>
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
        for (auto it = indexVars.rbegin(); it != indexVars.rend(); it++) {
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

    void SAMGraph::printComputation(std::ostream& os) const {
        auto compute = getComputation();
        for (auto it : compute) {
            os << (int)it << endl;
        }
    }

    void SAMGraph::printContractions(std::ostream& os) const {
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

    size_t SAMGraph::getMode(const IndexVar& indexvar, const TensorPath& path) const {
        auto vars = path.getVariables();
        auto tensor = path.getAccess().getTensorVar();
        auto it = find(vars.begin(), vars.end(), indexvar);
        taco_iassert(it != vars.end())  << "Indexvar " << indexvar << " should be in the tensor path"
                                        << path;
        auto mode = tensor.getFormat().getModeOrdering().at(distance(vars.begin(), it));
        return mode;
    }

    SamIR SAMGraph::makeGraph() {
        int id = 0;

        vector<SamIR> rootNodes;

        int numIndexVars = (int)getOrderedIndexVars().size();
        
        auto resultVars = getResultTensorPath().getVariables();
        auto resultTensor = getResultTensorPath().getAccess().getTensorVar();

        vector<TensorVar> inputTensors;
        for (const auto& tensorPath : getTensorPaths()) {
            inputTensors.push_back(tensorPath.getAccess().getTensorVar());
        }

        // Create map from index variable to input tensor dimension (size)
        // Eg. A(i,j) = B(i,k) * C(k,j) would have a dimensionMap of
        // i : B0_dim, j: C1_dim, k: B1_dim
        map<IndexVar, string> dimensionMap;
        string dimName;
        for (auto& tensorPath : getTensorPaths()) {
            auto vars = tensorPath.getVariables();
            auto tensor = tensorPath.getAccess().getTensorVar();

            for (auto & var : vars) {
                if (!contains(dimensionMap,var)) {
                    size_t mode = getMode(var, tensorPath);
                    dimensionMap[var] = tensor.getName() + to_string(mode) + "_dim";
                }
            }
        }

        // If dimension doesn't exist due to result broadcasting, use result dimension
        for (const auto& indexvar : getOrderedIndexVars()) {
            if (!contains(dimensionMap, indexvar)) {
                size_t mode = getMode(indexvar, getResultTensorPath());
                dimensionMap[indexvar] = resultTensor.getName() + to_string(mode) + "_dim";
            }
        }

        // Create map from index variable to datastructure sizes using dimensionMap above
        // A compressed level will always have sizes:
        //      seg: prevLevel's crdsize + 1
        //      crd: prevLevel's crdSize * current level's dimension
        map<IndexVar, pair<std::string, std::string>> sizeMap;
        IndexVar prevIndexVar;
        for (int count = 0; count < (int) getResultTensorPath().getSize(); count++) {
            IndexVar indexvar = resultVars.at(count);
            string segSize;
            string crdSize;
            if (count == 0) {
                segSize = "2";
                crdSize = dimensionMap.at(indexvar);
            } else {
                assert(prevIndexVar.defined());
                segSize = sizeMap.at(prevIndexVar).second + "+1";
                crdSize = sizeMap.at(prevIndexVar).second + "*" + dimensionMap.at(indexvar);
            }
            sizeMap[indexvar] = pair<string,string>(segSize, crdSize);

            prevIndexVar = indexvar;
        }

        // Output Assignment: Vals ONLY
        string valsSizeStr = "1";
        for (const auto& indexvar : resultVars) {
            if (contains(dimensionMap, indexvar)) {
                if (!valsSizeStr.empty()) {
                    valsSizeStr += "*";
                }
                valsSizeStr += dimensionMap.at(indexvar);
            }
        }
        SamIR resultWriteVals = FiberWrite(nullptr, getResultTensorPath().getAccess().getTensorVar(),
                                           -1, "",valsSizeStr, id,
                                           true, true);
        id++;

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


        // Tensor Contraction, collect which type (intersect or union) it is
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

        // Make map of index variables to a list of the tensor variables involved in the tensor contraction
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

        // Output Assignment: Crds ONLY
        map<IndexVar, SamIR> resultWriteIRNodes;
        map<IndexVar, bool> resultHasSource;
        for (int count = 0; count < (int) getOrderedIndexVars().size(); count++) {
            IndexVar indexvar = getOrderedIndexVars().at(getOrderedIndexVars().size() - 1 - count);
            if (std::count(resultVars.begin(), resultVars.end(), indexvar) > 0) {
                size_t mode = getMode(indexvar, getResultTensorPath());
                auto sizeStr = sizeMap.at(indexvar);
                auto node = FiberWrite(indexvar, getResultTensorPath().getAccess().getTensorVar(),
                                       (int) mode,sizeStr.first, sizeStr.second,
                                       id, true);
                id++;
                resultWriteIRNodes[indexvar] = node;
                resultHasSource[indexvar] = true;
            }

        }

        // Computation: Compute primitives
        map<IndexVar, SamIR> inputIterationCrdDst(resultWriteIRNodes);
        SamIR computeNode = SamIR();
        if (!compute.empty()) {
            auto prevComputeNode = resultWriteVals;
            map<int, SamIR> spAccCrds;
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
                        taco_iassert(!reductionOrder.empty()) << "Number of reduction (Sparse Accumulation) nodes does not "
                                                                 "match the number of reduction orders.";

                        for (int i = 0; i <= reductionOrder.back(); i++) {
                            auto indexvar = getOrderedIndexVars().at(getOrderedIndexVars().size() - 1 - i);
                            if (contains(resultWriteIRNodes, indexvar)) {
                                spAccCrds[i] = resultWriteIRNodes[indexvar];
                            }
                        }

                        computeNode = taco::sam::SparseAccumulator(prevComputeNode,
                                                                   spAccCrds, reductionOrder.back(), id);

                        for (int i = 0; i <= reductionOrder.back(); i++) {
                            // FIXME: check if this is always correct for more complicated kernels
                            auto indexvar = getOrderedIndexVars().at(getOrderedIndexVars().size() - 1 - i);
                            inputIterationCrdDst[indexvar] = computeNode;
                            resultHasSource[indexvar] = true;
                        }
                        reductionOrder.pop_back();
                        break;
                    default:
                        break;
                }
                prevComputeNode = computeNode;
                id++;
            }
        }

        // Computation: Create input values Arrays primitives
        map<TensorVar, SamIR> inputValsArrays;

        for (auto tensor : inputTensors) {
            // check if array is scalar
            bool isScalar = tensor.getFormat().getOrder() == 0;
            auto array = taco::sam::Array(compute.empty() ? resultWriteVals : computeNode, tensor, id,
                                          false, isScalar);
            id++;
            inputValsArrays[tensor] = array;
        }

        // Add in crd drop if needed
        // Map that replaces output assignments with CrdDrop blocks if necessary
        bool adjacentContractionLevel = false;
        IndexVar prevContractionVar = IndexVar();
        for (int count = 0; count < (int) getOrderedIndexVars().size(); count++) {
            IndexVar indexVar = getOrderedIndexVars().at(count);

            bool isIntersection = contains(contractionType, contractions.at(indexVar)) &&
                                  contractionType.at(contractions.at(indexVar));
            // FIXME: See if the result var needing to be there is necessary... Think about X(i) = B(i,j)*C(i,j)
            if (std::count(resultVars.begin(), resultVars.end(), indexVar) > 0 and
                contains(contractions, indexVar) and contractions.at(indexVar).size() > 1 and
                isIntersection and adjacentContractionLevel) {

                auto node = CrdDrop(inputIterationCrdDst[prevContractionVar], inputIterationCrdDst[indexVar],
                                    prevContractionVar, indexVar, id);
                id++;

                inputIterationCrdDst[prevContractionVar] = node;
                inputIterationCrdDst[indexVar] = node;

                adjacentContractionLevel = true;
                prevContractionVar = indexVar;
            } else if (std::count(resultVars.begin(), resultVars.end(), indexVar) > 0 and
                        contains(contractions, indexVar) and contractions.at(indexVar).size() > 1) {
                adjacentContractionLevel = true;
                prevContractionVar = indexVar;
            } else {
                adjacentContractionLevel = false;
                prevContractionVar = IndexVar();
            }
        }

        // Input Iteration
        map<IndexVar, vector<SamIR>> nodeMap;
        for (int count = 0; count < numIndexVars; count++) {

            IndexVar indexvar = getOrderedIndexVars().at(numIndexVars - 1 - count);
            bool isRoot = count == numIndexVars - 1;
            IndexVar prevIndexVar = count == 0 ? nullptr : getOrderedIndexVars().at(numIndexVars - count);

            auto crdDest = contains(resultHasSource,indexvar) && resultHasSource.at(indexvar) ?
                           inputIterationCrdDst.at(indexvar) : SamIR();

            bool hasContraction = contractions.at(indexvar).size() > 1;
            bool hasSparseAccumulation = isa<SparseAccumulator>(crdDest);

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

                if (std::count(vars.begin(), vars.end(), indexvar) == 0) {
                    if (count == 0) {
                        node = Repeat(inputValsArrays[tensorVar], indexvar, tensorVar, id, isRoot);
                    } else {
                        auto prevSAMNode = nodeMap[prevIndexVar][ntp];
                        node = Repeat(prevSAMNode, indexvar, tensorVar, id, isRoot);
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
                map<SamIR, string> edgeName;
                if (hasSparseAccumulation) {
                    edgeName[crdDest] = "in-" + to_string(count) + "-" + indexvar.getName() ;
                    crdDest = Broadcast({crdDest, repeatSigGenNode}, sam::SamEdgeType::crd, id, edgeName);
                } else {
                    crdDest = Broadcast({crdDest, repeatSigGenNode}, sam::SamEdgeType::crd, id);
                }
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

                bool isCrdDrop = isa<CrdDrop>(crdDest);
                bool printEdgeName = isCrdDrop || (!isCrdDrop && hasSparseAccumulation);
                auto edgeName = isCrdDrop ? "in-" + indexvar.getName() : hasSparseAccumulation ?
                        "in-" + to_string(count) + "-" + indexvar.getName() : "";
                if (isIntersection)
                    contractNode = taco::sam::Intersect(crdDest, contractOuts, indexvar, id,
                                                        printEdgeName, edgeName);
                else
                    contractNode = taco::sam::Union(crdDest, contractOuts, indexvar, id,
                                                    printEdgeName, edgeName);
                id++;
            }

            for (int ntp = 0; ntp < (int) getTensorPaths().size(); ntp++) {
                SamIR node;

                TensorPath tensorPath = getTensorPaths().at(ntp);
                auto tensorVar = tensorPath.getAccess().getTensorVar();

                auto formats = getFormatMapping(tensorPath);

                auto vars = tensorPath.getVariables();

                if (std::count(vars.begin(), vars.end(), indexvar) > 0) {
                    size_t mode = getMode(indexvar, tensorPath);
                    map<SamIR, string> edgeName;
                    if (hasContraction) {
                        edgeName[contractNode] = "in-" + tensorVar.getName();
                    }
                    else if (hasSparseAccumulation && ! isa<Broadcast>(crdDest)) {
                        edgeName[crdDest] = "in-" + to_string(numIndexVars - 1 - count) + "-" + indexvar.getName();
                    }
                    bool printEdgeName = hasContraction || (!hasContraction && hasSparseAccumulation);
                    if (count == 0) {
                        node = FiberLookup(hasContraction ? contractNode : inputValsArrays[tensorVar], hasContraction ? contractNode : crdDest,
                                           indexvar, tensorVar, mode,  id, edgeName, isRoot, true);
                    } else {
                        auto prevSAMNode = hasContraction ? contractNode : nodeMap[prevIndexVar][ntp];
                        node = FiberLookup(prevSAMNode, hasContraction ? contractNode : crdDest,
                                           indexvar, tensorVar, mode, id, edgeName, isRoot, true);
                    }

                    id++;
                    nodes[ntp] = node;

                    if (count == numIndexVars - 1) {
                        rootNodes.push_back(node);
                    }
                }
            }
            nodeMap[indexvar] = nodes;
        }

        vector<TensorVar> tensors;
        tensors.push_back(getResultTensorPath().getAccess().getTensorVar());
        for (auto tp : getTensorPaths()) {
            tensors.push_back(tp.getAccess().getTensorVar());
        }

        // Trivial case where all inputs and outputs are scalar
        if (numIndexVars == 0)
            for (auto arr : inputValsArrays)
                rootNodes.push_back(arr.second);

        auto root = Root(rootNodes, tensors);
        return root;
    }

    void SAMGraph::printInputIteration(std::ostream& os) {
        for (auto& tensorPath : getTensorPaths()) {
            auto tensorName = tensorPath.getAccess().getTensorVar().getName();

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
        auto sam = makeGraph();
        SAMDotNodePrinter printer(os);
        //printer.setPrintAttributes(false);
        printer.print(sam);

        SAMDotEdgePrinter printerEdge(os);
        //printerEdge.setPrintAttributes(false);
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

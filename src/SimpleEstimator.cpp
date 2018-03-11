//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

using namespace std;

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){

    // works only with SimpleGraph
    graph = g;
    countA = new int[graph->getNoLabels()] {};
    countB = new int[graph->getNoLabels()] {};
    outvertices_to_remove = new int[graph->getNoLabels()] {};
    invertices_to_remove = new int[graph->getNoLabels()] {};
    edgeListA = new int[graph->getNoLabels()] {};
    edgeListB = new int[graph->getNoLabels()] {};
}

void SimpleEstimator::prepare() {

    // do your prep here

    //The idea is to calculate total number of edges that are going 'out' and coming 'in' for each type of edge. i.e. for each label type.
    int Labels_count = graph->getNoLabels();
    int Vertices_count = graph->getNoVertices();

    //Calculating number of edges in each type that are in adj list and reverse_adj list

    for (int i=0; i<Vertices_count; i++){
        for (auto Label_Index: graph->adj[i]) {
            countA[Label_Index.first]++;
        }

        for (auto Label_Index: graph->reverse_adj[i]) {
            countB[Label_Index.first]++;
        }
    }

    //calculating the number of vertices that are commonly referred from both sides
    for (int j = 0; j < Labels_count; j++) {
        if (countA[j] == edgeListA[j])
            outvertices_to_remove[j]++;
        else
            edgeListA[j] = countA[j];

        if (countB[j] == edgeListB[j])
            invertices_to_remove[j]++;
        else
            edgeListB[j] = countB[j];
    }
}

cardStat SimpleEstimator::estimate(RPQTree *q) {

    // perform your estimation here

    // evaluate according to the AST bottom-up

    if(q->isLeaf()) {
        // project out the label in the AST
        std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");

        std::smatch matches;

        uint32_t label;
        bool inverse;

        if(std::regex_search(q->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = false;
            return cardStat{(uint32_t)(graph->getNoVertices() - outvertices_to_remove[label]), (uint32_t)countA[label], (uint32_t)(graph->getNoVertices() - invertices_to_remove[label])};
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = true;
            return cardStat{(uint32_t)(graph -> getNoVertices() - invertices_to_remove[label]), (uint32_t)countA[label], (uint32_t)(graph->getNoVertices() - outvertices_to_remove[label])};
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return cardStat{0, 0, 0};
        }
    }

    if(q->isConcat()) {

        // evaluate the children
        int Vertices_count = graph->getNoVertices();
        auto leftGraph = SimpleEstimator::estimate(q->left);
        auto rightGraph = SimpleEstimator::estimate(q->right);
        double rightout_leftout = (double)rightGraph.noOut * leftGraph.noOut;
        double rightout_rightin = (double)rightGraph.noPaths * rightGraph.noIn;

        uint32_t noOut = static_cast<uint32_t>((int)(rightout_leftout / Vertices_count));
        uint32_t noIn = (int)(rightout_rightin * Vertices_count);
        uint32_t noPaths = leftGraph.noPaths * (rightGraph.noOut/Vertices_count) * (rightGraph.noPaths/rightGraph.noOut);

        return cardStat{noOut, noPaths, noIn};
    }

    return cardStat {0, 0, 0};
}

SimpleEstimator::~SimpleEstimator() {
    delete[] countA;
    delete[] countB;
    delete[] outvertices_to_remove;
    delete[] invertices_to_remove;
    delete[] edgeListA;
    delete[] edgeListB;
}

#pragma once

namespace delaunator {
    class Delaunator;
}


using coord_t = std::pair<int, int>;
using edge_t = size_t;
using v_double_t = std::vector<double>;
using double_pair_t = std::pair<double, double>;
using v_double_pair_t = std::vector<double_pair_t>;

edge_t nextHalfEdge(edge_t edge);;

edge_t triangleOfEdge(edge_t e);;

//returns 3 edge IDs
std::vector<edge_t> edgesOfTriangle(edge_t t);;

//finds 3 points for a given triangle id
std::vector<edge_t> pointsOfTriangle(const delaunator::Delaunator& delaunator, edge_t tri_id);;

std::vector<edge_t> edgesAroundPoint(const delaunator::Delaunator& delaunator, edge_t start);;

//circumcenter of triangle
std::pair<double, double> triangleCenter(delaunator::Delaunator& delaunator, edge_t tri_id);;

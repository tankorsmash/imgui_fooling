#include "imgui_stdafx.h"

#include "voronoi.h"

#include <iterator>
#include <vector>

#include "delaunator.hpp"

edge_t nextHalfEdge(edge_t edge) { return (edge % 3 == 2) ? edge - 2 : edge + 1; }

edge_t triangleOfEdge(edge_t e) { return e / 3; }

std::vector<edge_t> edgesOfTriangle(edge_t t) { return std::vector<edge_t>{3 * t, 3 * t + 1, 3 * t + 2}; }

std::vector<edge_t> pointsOfTriangle(const delaunator::Delaunator& delaunator, edge_t tri_id)
{
    auto edges = edgesOfTriangle(tri_id);

    std::vector<size_t> points;
    points.reserve(3);
    auto find_tri_for_edge = [&delaunator](edge_t e) -> size_t { return delaunator.triangles[e]; };
    std::transform(edges.begin(), edges.end(), std::back_inserter(points), find_tri_for_edge);

    return points;
}

std::vector<edge_t> edgesAroundPoint(const delaunator::Delaunator& delaunator, edge_t start)
{
    std::vector<edge_t> result;

    edge_t incoming = start;
    do {
        result.push_back(incoming);
        const edge_t outgoing = nextHalfEdge(incoming);
        incoming              = delaunator.halfedges[outgoing];
        //} while (incoming != (edge_t)-1 && incoming != start);
    }
    while (incoming != delaunator::INVALID_INDEX && incoming != start);

    return result;
}

std::pair<double, double> triangleCenter(delaunator::Delaunator& delaunator, edge_t tri_id)
{
    auto tri_points = pointsOfTriangle(delaunator, tri_id);
    std::vector<std::pair<double, double>> vertices{};
    std::transform(tri_points.begin(), tri_points.end(), std::back_inserter(vertices), [&delaunator](edge_t tri_point) { return double_pair_t{delaunator.coords.at(tri_point * 2), delaunator.coords.at(tri_point * 2 + 1)}; });

    //auto result = circumcenter(vertices[0], vertices[1], vertices[2]);
    auto result = delaunator::circumcenter(
        vertices[0].first, vertices[0].second,
        vertices[1].first, vertices[1].second,
        vertices[2].first, vertices[2].second
    );
    return std::pair<double, double>(result.first, result.second);
}

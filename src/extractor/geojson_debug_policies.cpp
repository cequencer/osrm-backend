#include "extractor/geojson_debug_policies.hpp"
#include "util/geojson_debug_policy_toolkit.hpp"
#include "util/coordinate.hpp"

#include <algorithm>

namespace osrm
{
namespace extractor
{

IntersectionPrinter::IntersectionPrinter(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const std::vector<extractor::QueryNode> &node_coordinates,
    const extractor::guidance::CoordinateExtractor &coordinate_extractor)
    : node_based_graph(node_based_graph), node_coordinates(node_coordinates),
      coordinate_extractor(coordinate_extractor){};

util::json::Array IntersectionPrinter::
operator()(const NodeID intersection_node,
           const extractor::guidance::Intersection &intersection) const
{
    // request the number of lanes. This process needs to be in sync with what happens over at
    // intersection_generator
    const auto intersection_lanes =
        extractor::guidance::getLaneCountAtIntersection(intersection_node, node_based_graph);

    std::vector<util::Coordinate> coordinates;
    coordinates.reserve(intersection.size());
    coordinates.push_back(node_coordinates[intersection_node]);

    const auto road_to_coordinate = [&](const extractor::guidance::ConnectedRoad &connected_road) {
        const constexpr auto FORWARD = false;
        const auto to_node = node_based_graph.GetTarget(connected_road.turn.eid);
        return coordinate_extractor.GetCoordinateAlongRoad(
            intersection_node, connected_road.turn.eid, FORWARD, to_node, intersection_lanes);
    };

    std::transform(intersection.begin(),
                   intersection.end(),
                   std::back_inserter(coordinates),
                   road_to_coordinate);

    const auto json_coordinates = makeJsonArray(coordinates);
    util::json::Array features;
    features.values.push_back(util::makeFeature("MultiPoint", json_coordinates));

    if (coordinates.size() > 1)
    {
        std::vector<util::Coordinate> line_coordinates(2);
        line_coordinates[0] = coordinates.front();
        const auto coordinate_to_line = [&](const util::Coordinate coordinate) {
            line_coordinates[1] = coordinate;
            return util::makeFeature("LineString", makeJsonArray(line_coordinates));
        };

        std::transform(std::next(coordinates.begin()),
                       coordinates.end(),
                       std::back_inserter(features.values),
                       coordinate_to_line);
    }
    return features;
}

} /* namespace extractor */
} /* namespace osrm */
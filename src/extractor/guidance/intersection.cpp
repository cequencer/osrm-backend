#include "extractor/guidance/intersection.hpp"

#include <limits>
#include <string>

using osrm::util::angularDeviation;

namespace osrm
{
namespace extractor
{
namespace guidance
{

bool IntersectionViewData::CompareByAngle(const IntersectionViewData &other) const
{
    return angle < other.angle;
}

bool ConnectedRoad::compareByAngle(const ConnectedRoad &other) const { return angle < other.angle; }

void ConnectedRoad::mirror()
{
    const constexpr DirectionModifier::Enum mirrored_modifiers[] = {DirectionModifier::UTurn,
                                                                    DirectionModifier::SharpLeft,
                                                                    DirectionModifier::Left,
                                                                    DirectionModifier::SlightLeft,
                                                                    DirectionModifier::Straight,
                                                                    DirectionModifier::SlightRight,
                                                                    DirectionModifier::Right,
                                                                    DirectionModifier::SharpRight};

    static_assert(sizeof(mirrored_modifiers) / sizeof(DirectionModifier::Enum) ==
                      DirectionModifier::MaxDirectionModifier,
                  "The list of mirrored modifiers needs to match the available modifiers in size.");

    if (angularDeviation(angle, 0) > std::numeric_limits<double>::epsilon())
    {
        angle = 360 - angle;
        instruction.direction_modifier = mirrored_modifiers[instruction.direction_modifier];
    }
}

ConnectedRoad ConnectedRoad::getMirroredCopy() const
{
    ConnectedRoad copy(*this);
    copy.mirror();
    return copy;
}

std::string toString(const ConnectedRoad &road)
{
    std::string result = "[connection] ";
    result += std::to_string(road.eid);
    result += " allows entry: ";
    result += std::to_string(road.entry_allowed);
    result += " angle: ";
    result += std::to_string(road.angle);
    result += " bearing: ";
    result += std::to_string(road.bearing);
    result += " instruction: ";
    result += std::to_string(static_cast<std::int32_t>(road.instruction.type)) + " " +
              std::to_string(static_cast<std::int32_t>(road.instruction.direction_modifier)) + " " +
              std::to_string(static_cast<std::int32_t>(road.lane_data_id));
    return result;
}

Intersection::Base::iterator Intersection::findClosestTurn(double angle)
{
    // use the const operator to avoid code duplication
    return begin() +
           std::distance(cbegin(), static_cast<const Intersection *>(this)->findClosestTurn(angle));
}

Intersection::Base::const_iterator Intersection::findClosestTurn(double angle) const
{
    return std::min_element(
        begin(), end(), [angle](const ConnectedRoad &lhs, const ConnectedRoad &rhs) {
            return angularDeviation(lhs.angle, angle) < angularDeviation(rhs.angle, angle);
        });
}
bool Intersection::valid() const
{
    return !empty() &&
           std::is_sorted(begin(), end(), std::mem_fn(&ConnectedRoad::compareByAngle)) &&
           operator[](0).angle < std::numeric_limits<double>::epsilon();
}

std::uint8_t
Intersection::getHighestConnectedLaneCount(const util::NodeBasedDynamicGraph &graph) const
{
    BOOST_ASSERT(valid()); // non empty()

    const std::function<std::uint8_t(const ConnectedRoad &)> to_lane_count =
        [&](const ConnectedRoad &road) {
            return graph.GetEdgeData(road.eid).road_classification.GetNumberOfLanes();
        };

    std::uint8_t max_lanes = 0;
    const auto extract_maximal_value = [&max_lanes](std::uint8_t value) {
        max_lanes = std::max(max_lanes, value);
        return false;
    };

    const auto view = *this | boost::adaptors::transformed(to_lane_count);
    boost::range::find_if(view, extract_maximal_value);
    return max_lanes;
}

// check if all entries in the given range allow entry
bool Intersection::hasValidEntries(std::size_t first, std::size_t last) const
{
    BOOST_ASSERT(last < size());

    for (std::size_t i = first; i <= last; ++i)
    {
        if (!operator[](i).entry_allowed)
        {
            return false;
        }
    }
    // if no entry was found that forbids entry, the intersection entries in the range are all
    // valid.
    return true;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm

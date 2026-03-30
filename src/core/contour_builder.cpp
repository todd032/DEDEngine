#include "core/contour_builder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace cotrx::core
{

namespace
{
struct QuantizedPoint
{
    std::int64_t x = 0;
    std::int64_t y = 0;
    std::int64_t z = 0;

    bool operator==(const QuantizedPoint& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct QuantizedPointHasher
{
    std::size_t operator()(const QuantizedPoint& key) const
    {
        const std::uint64_t a = static_cast<std::uint64_t>(key.x * 73856093LL);
        const std::uint64_t b = static_cast<std::uint64_t>(key.y * 19349663LL);
        const std::uint64_t c = static_cast<std::uint64_t>(key.z * 83492791LL);
        return static_cast<std::size_t>(a ^ b ^ c);
    }
};

struct Node
{
    Vec3 point{};
    std::vector<std::uint32_t> segmentIds;
};

QuantizedPoint QuantizePoint(const Vec3& point, const float grid)
{
    return {
        static_cast<std::int64_t>(std::llround(point.x / grid)),
        static_cast<std::int64_t>(std::llround(point.y / grid)),
        static_cast<std::int64_t>(std::llround(point.z / grid))};
}

float ComputeSignedArea(const std::vector<Vec2>& polygon)
{
    if (polygon.size() < 3)
    {
        return 0.0f;
    }

    double area = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i)
    {
        const auto& a = polygon[i];
        const auto& b = polygon[(i + 1) % polygon.size()];
        area += static_cast<double>(a.x) * static_cast<double>(b.y) -
            static_cast<double>(b.x) * static_cast<double>(a.y);
    }

    return static_cast<float>(0.5 * area);
}

} // namespace

ContourBuildResult BuildContourLoops(
    const std::vector<CutSegment>& segments,
    const PlaneBasis& basis,
    const SliceEpsilon& epsilon)
{
    ContourBuildResult result;
    if (segments.empty())
    {
        return result;
    }

    struct SegmentNodeRef
    {
        std::uint32_t node0 = 0;
        std::uint32_t node1 = 0;
    };

    std::unordered_map<QuantizedPoint, std::uint32_t, QuantizedPointHasher> nodeByKey;
    std::vector<Node> nodes;
    std::vector<SegmentNodeRef> segmentNodeRefs;
    segmentNodeRefs.reserve(segments.size());

    const auto quantGrid = std::max(epsilon.mergeRadius, 1.0e-8f);

    auto getNodeId = [&](const Vec3& point) {
        const auto key = QuantizePoint(point, quantGrid);
        const auto it = nodeByKey.find(key);
        if (it != nodeByKey.end())
        {
            return it->second;
        }

        const auto id = static_cast<std::uint32_t>(nodes.size());
        nodes.push_back({point, {}});
        nodeByKey.emplace(key, id);
        return id;
    };

    for (std::uint32_t i = 0; i < segments.size(); ++i)
    {
        const auto node0 = getNodeId(segments[i].point0);
        const auto node1 = getNodeId(segments[i].point1);
        segmentNodeRefs.push_back({node0, node1});
        nodes[node0].segmentIds.push_back(i);
        nodes[node1].segmentIds.push_back(i);
    }

    std::vector<bool> visited(segments.size(), false);

    for (std::uint32_t startSegmentId = 0; startSegmentId < segments.size(); ++startSegmentId)
    {
        if (visited[startSegmentId])
        {
            continue;
        }

        std::vector<Vec3> chain;
        std::vector<Vec2> chain2D;

        const auto expectedKind = segments[startSegmentId].boundaryKind;
        auto currentSegmentId = startSegmentId;
        auto currentNode = segmentNodeRefs[currentSegmentId].node0;
        auto nextNode = segmentNodeRefs[currentSegmentId].node1;

        chain.push_back(nodes[currentNode].point);

        bool loopClosed = false;
        while (true)
        {
            visited[currentSegmentId] = true;
            chain.push_back(nodes[nextNode].point);

            if (nextNode == segmentNodeRefs[startSegmentId].node0)
            {
                loopClosed = true;
                break;
            }

            std::vector<std::uint32_t> candidates;
            for (const auto candidate : nodes[nextNode].segmentIds)
            {
                if (!visited[candidate] && segments[candidate].boundaryKind == expectedKind)
                {
                    candidates.push_back(candidate);
                }
            }

            if (candidates.empty())
            {
                break;
            }

            std::sort(candidates.begin(), candidates.end());
            currentNode = nextNode;
            currentSegmentId = candidates.front();
            const auto& segRef = segmentNodeRefs[currentSegmentId];
            nextNode = (segRef.node0 == currentNode) ? segRef.node1 : segRef.node0;
        }

        if (!loopClosed || chain.size() < 4)
        {
            continue;
        }

        chain.pop_back();

        bool hasSliver = false;
        for (std::size_t i = 0; i < chain.size(); ++i)
        {
            const auto edgeLengthSq = LengthSquared(chain[(i + 1) % chain.size()] - chain[i]);
            if (edgeLengthSq <= epsilon.segmentLength * epsilon.segmentLength)
            {
                hasSliver = true;
                break;
            }
        }

        if (hasSliver)
        {
            continue;
        }

        chain2D.reserve(chain.size());
        for (const auto& point : chain)
        {
            chain2D.push_back(ProjectPointToPlane(basis, point));
        }

        auto area = ComputeSignedArea(chain2D);
        if (std::abs(area) <= epsilon.segmentLength * epsilon.segmentLength)
        {
            continue;
        }

        if (area < 0.0f)
        {
            std::reverse(chain.begin(), chain.end());
            std::reverse(chain2D.begin(), chain2D.end());
            area = -area;
        }

        result.loops.push_back({
            static_cast<std::uint32_t>(result.loops.size()),
            expectedKind,
            chain,
            chain2D,
            area});
    }

    std::sort(result.loops.begin(), result.loops.end(), [](const ContourLoop& a, const ContourLoop& b) {
        if (a.boundaryKind != b.boundaryKind)
        {
            return static_cast<int>(a.boundaryKind) < static_cast<int>(b.boundaryKind);
        }

        if (a.points3D.empty() || b.points3D.empty())
        {
            return a.points3D.size() < b.points3D.size();
        }

        if (a.points3D.front().x != b.points3D.front().x)
        {
            return a.points3D.front().x < b.points3D.front().x;
        }

        if (a.points3D.front().y != b.points3D.front().y)
        {
            return a.points3D.front().y < b.points3D.front().y;
        }

        return a.points3D.front().z < b.points3D.front().z;
    });

    for (std::uint32_t i = 0; i < result.loops.size(); ++i)
    {
        result.loops[i].loopId = i;
    }

    return result;
}

void contour_builder_module_anchor()
{
}

} // namespace cotrx::core

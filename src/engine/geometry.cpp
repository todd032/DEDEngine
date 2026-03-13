#include "engine/geometry.hpp"

namespace cotrx
{
namespace
{
void AddQuad(MeshData& mesh, const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Color& color)
{
    const auto normal = Normalize(Cross(b - a, c - a));
    if (LengthSquared(normal) <= 1.0e-6f)
    {
        return;
    }

    const auto baseIndex = static_cast<std::uint32_t>(mesh.vertices.size());

    mesh.vertices.push_back({a, normal, color});
    mesh.vertices.push_back({b, normal, color});
    mesh.vertices.push_back({c, normal, color});
    mesh.vertices.push_back({d, normal, color});

    mesh.indices.insert(mesh.indices.end(), {
        baseIndex, baseIndex + 1U, baseIndex + 2U,
        baseIndex, baseIndex + 2U, baseIndex + 3U});

    ExpandBounds(mesh.bounds, a);
    ExpandBounds(mesh.bounds, b);
    ExpandBounds(mesh.bounds, c);
    ExpandBounds(mesh.bounds, d);
}

std::vector<Vec3> SampleGrid(
    int uSegments,
    int vSegments,
    const std::function<Vec3(float, float)>& sampler)
{
    std::vector<Vec3> grid(static_cast<std::size_t>((uSegments + 1) * (vSegments + 1)));
    for (int u = 0; u <= uSegments; ++u)
    {
        for (int v = 0; v <= vSegments; ++v)
        {
            grid[static_cast<std::size_t>((u * (vSegments + 1)) + v)] =
                sampler(static_cast<float>(u) / static_cast<float>(uSegments),
                        static_cast<float>(v) / static_cast<float>(vSegments));
        }
    }

    return grid;
}

const Vec3& SampleAt(const std::vector<Vec3>& grid, int vSegments, int u, int v)
{
    return grid[static_cast<std::size_t>((u * (vSegments + 1)) + v)];
}
} // namespace

void AppendMesh(MeshData& target, const MeshData& source)
{
    const auto baseIndex = static_cast<std::uint32_t>(target.vertices.size());
    target.vertices.insert(target.vertices.end(), source.vertices.begin(), source.vertices.end());

    for (const auto index : source.indices)
    {
        target.indices.push_back(baseIndex + index);
    }

    ExpandBounds(target.bounds, source.bounds.minimum);
    ExpandBounds(target.bounds, source.bounds.maximum);
}

MeshData CreateQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Color& color)
{
    MeshData mesh{};
    AddQuad(mesh, a, b, c, d, color);
    return mesh;
}

MeshData CreateBox(const Vec3& center, float width, float height, float depth, const Color& color)
{
    const auto halfWidth = width * 0.5f;
    const auto halfHeight = height * 0.5f;
    const auto halfDepth = depth * 0.5f;

    return CreateOrientedBox(
        center,
        {halfWidth, 0.0f, 0.0f},
        {0.0f, halfHeight, 0.0f},
        {0.0f, 0.0f, halfDepth},
        color);
}

MeshData CreateOrientedBox(const Vec3& center, const Vec3& halfX, const Vec3& halfY, const Vec3& halfZ, const Color& color)
{
    const auto leftBottomBack = center - halfX - halfY - halfZ;
    const auto leftBottomFront = center - halfX - halfY + halfZ;
    const auto leftTopBack = center - halfX + halfY - halfZ;
    const auto leftTopFront = center - halfX + halfY + halfZ;
    const auto rightBottomBack = center + halfX - halfY - halfZ;
    const auto rightBottomFront = center + halfX - halfY + halfZ;
    const auto rightTopBack = center + halfX + halfY - halfZ;
    const auto rightTopFront = center + halfX + halfY + halfZ;

    MeshData mesh{};
    AddQuad(mesh, leftBottomFront, rightBottomFront, rightTopFront, leftTopFront, color);
    AddQuad(mesh, rightBottomBack, leftBottomBack, leftTopBack, rightTopBack, color);
    AddQuad(mesh, rightBottomFront, rightBottomBack, rightTopBack, rightTopFront, color);
    AddQuad(mesh, leftBottomBack, leftBottomFront, leftTopFront, leftTopBack, color);
    AddQuad(mesh, leftTopBack, leftTopFront, rightTopFront, rightTopBack, color);
    AddQuad(mesh, leftBottomFront, leftBottomBack, rightBottomBack, rightBottomFront, color);
    return mesh;
}

MeshData CreateSurfaceSolid(
    int uSegments,
    int vSegments,
    const std::function<Vec3(float, float)>& topSampler,
    const std::function<Vec3(float, float)>& bottomSampler,
    const Color& color)
{
    MeshData mesh{};
    const auto top = SampleGrid(uSegments, vSegments, topSampler);
    const auto bottom = SampleGrid(uSegments, vSegments, bottomSampler);

    for (int u = 0; u < uSegments; ++u)
    {
        for (int v = 0; v < vSegments; ++v)
        {
            AddQuad(
                mesh,
                SampleAt(top, vSegments, u, v + 1),
                SampleAt(top, vSegments, u + 1, v + 1),
                SampleAt(top, vSegments, u + 1, v),
                SampleAt(top, vSegments, u, v),
                color);
        }
    }

    for (int u = 0; u < uSegments; ++u)
    {
        AddQuad(
            mesh,
            SampleAt(top, vSegments, u, 0),
            SampleAt(top, vSegments, u + 1, 0),
            SampleAt(bottom, vSegments, u + 1, 0),
            SampleAt(bottom, vSegments, u, 0),
            color);
        AddQuad(
            mesh,
            SampleAt(bottom, vSegments, u, vSegments),
            SampleAt(bottom, vSegments, u + 1, vSegments),
            SampleAt(top, vSegments, u + 1, vSegments),
            SampleAt(top, vSegments, u, vSegments),
            color);
    }

    for (int v = 0; v < vSegments; ++v)
    {
        AddQuad(
            mesh,
            SampleAt(top, vSegments, 0, v),
            SampleAt(bottom, vSegments, 0, v),
            SampleAt(bottom, vSegments, 0, v + 1),
            SampleAt(top, vSegments, 0, v + 1),
            color);
        AddQuad(
            mesh,
            SampleAt(bottom, vSegments, uSegments, v),
            SampleAt(top, vSegments, uSegments, v),
            SampleAt(top, vSegments, uSegments, v + 1),
            SampleAt(bottom, vSegments, uSegments, v + 1),
            color);
    }

    return mesh;
}
} // namespace cotrx

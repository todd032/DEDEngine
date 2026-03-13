#include "engine/renderer.hpp"

#include <array>
#include <cctype>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include "engine/gl_api.hpp"

namespace cotrx
{
namespace
{
struct Glyph
{
    std::array<std::uint8_t, 7> rows{};
};

Glyph GetGlyph(char character)
{
    switch (static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(character))))
    {
    case 'A': return {{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}};
    case 'B': return {{0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}};
    case 'C': return {{0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}};
    case 'D': return {{0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}};
    case 'E': return {{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}};
    case 'F': return {{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}};
    case 'G': return {{0x0F, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0F}};
    case 'H': return {{0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}};
    case 'I': return {{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F}};
    case 'J': return {{0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}};
    case 'K': return {{0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}};
    case 'L': return {{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}};
    case 'M': return {{0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}};
    case 'N': return {{0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}};
    case 'O': return {{0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}};
    case 'P': return {{0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}};
    case 'Q': return {{0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}};
    case 'R': return {{0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}};
    case 'S': return {{0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}};
    case 'T': return {{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}};
    case 'U': return {{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}};
    case 'V': return {{0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}};
    case 'W': return {{0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}};
    case 'X': return {{0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}};
    case 'Y': return {{0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}};
    case 'Z': return {{0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}};
    case '0': return {{0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}};
    case '1': return {{0x04, 0x0C, 0x14, 0x04, 0x04, 0x04, 0x1F}};
    case '2': return {{0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}};
    case '3': return {{0x1E, 0x01, 0x01, 0x06, 0x01, 0x01, 0x1E}};
    case '4': return {{0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}};
    case '5': return {{0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}};
    case '6': return {{0x07, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}};
    case '7': return {{0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}};
    case '8': return {{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}};
    case '9': return {{0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x1C}};
    case ':': return {{0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}};
    case '.': return {{0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06}};
    case '-': return {{0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}};
    case '_': return {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F}};
    default: return {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    }
}

std::string BuildShaderSource(bool glesContext, const char* commonBody)
{
    std::ostringstream stream;
    if (glesContext)
    {
        stream << "#version 300 es\nprecision mediump float;\n";
    }
    else
    {
        stream << "#version 330 core\n";
    }

    stream << commonBody;
    return stream.str();
}

bool CompileShader(GLuint shader, const std::string& source, std::string& error)
{
    const char* sourcePtr = source.c_str();
    gl::ShaderSource(shader, 1, &sourcePtr, nullptr);
    gl::CompileShader(shader);

    GLint compiled = GL_FALSE;
    gl::GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE)
    {
        return true;
    }

    GLint logLength = 0;
    gl::GetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::string infoLog(static_cast<std::size_t>(std::max(1, logLength)), '\0');
    gl::GetShaderInfoLog(shader, logLength, nullptr, infoLog.data());
    error = infoLog;
    return false;
}

bool LinkProgram(GLuint program, std::string& error)
{
    gl::LinkProgram(program);
    GLint linked = GL_FALSE;
    gl::GetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE)
    {
        return true;
    }

    GLint logLength = 0;
    gl::GetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::string infoLog(static_cast<std::size_t>(std::max(1, logLength)), '\0');
    gl::GetProgramInfoLog(program, logLength, nullptr, infoLog.data());
    error = infoLog;
    return false;
}
} // namespace

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Initialize(bool glesContext)
{
    glesContext_ = glesContext;
    if (!gl::Load())
    {
        return false;
    }

    if (!CreatePrograms(glesContext))
    {
        return false;
    }

    gl::GenVertexArrays(1, &uiVao_);
    gl::GenBuffers(1, &uiVbo_);
    gl::BindVertexArray(uiVao_);
    gl::BindBuffer(GL_ARRAY_BUFFER, uiVbo_);
    gl::BufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    gl::EnableVertexAttribArray(0);
    gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UiVertex), reinterpret_cast<const void*>(offsetof(UiVertex, x)));
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UiVertex), reinterpret_cast<const void*>(offsetof(UiVertex, r)));
    gl::BindVertexArray(0);
    gl::BindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void Renderer::Shutdown()
{
    DestroyMeshes();

    if (uiVbo_ != 0)
    {
        gl::DeleteBuffers(1, &uiVbo_);
        uiVbo_ = 0;
    }

    if (uiVao_ != 0)
    {
        gl::DeleteVertexArrays(1, &uiVao_);
        uiVao_ = 0;
    }

    if (sceneProgram_ != 0)
    {
        gl::DeleteProgram(sceneProgram_);
        sceneProgram_ = 0;
    }

    if (uiProgram_ != 0)
    {
        gl::DeleteProgram(uiProgram_);
        uiProgram_ = 0;
    }

    gl::Reset();
}

void Renderer::RebuildScene(const SimulationState& state)
{
    DestroyMeshes();
    sceneMeshes_.resize(state.meshes.size());
    for (std::size_t index = 0; index < state.meshes.size(); ++index)
    {
        UploadMesh(state.meshes[index], sceneMeshes_[index]);
    }
}

void Renderer::Render(const SimulationState& state, const Color& clearColor, int uiWidth, int uiHeight, int drawableWidth, int drawableHeight)
{
    glViewport(0, 0, drawableWidth, drawableHeight);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto aspect = static_cast<float>(std::max(1, drawableWidth)) / static_cast<float>(std::max(1, drawableHeight));
    const auto viewProjection = state.camera.ProjectionMatrix(aspect) * state.camera.ViewMatrix();

    gl::UseProgram(sceneProgram_);
    gl::UniformMatrix4fv(sceneViewProjectionLocation_, 1, GL_FALSE, viewProjection.m.data());
    gl::Uniform3f(sceneLightLocation_, -0.45f, 1.0f, -0.28f);

    for (const auto& object : state.sceneObjects)
    {
        if (object.meshIndex >= sceneMeshes_.size())
        {
            continue;
        }

        const auto& gpuMesh = sceneMeshes_[object.meshIndex];
        const auto model = object.transform.ToMatrix();
        gl::UniformMatrix4fv(sceneModelLocation_, 1, GL_FALSE, model.m.data());
        gl::Uniform4f(sceneTintLocation_, object.tint.r, object.tint.g, object.tint.b, object.tint.a);
        gl::BindVertexArray(gpuMesh.vao);
        glDrawElements(GL_TRIANGLES, gpuMesh.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    std::vector<UiVertex> uiVertices;
    BuildUiVertices(state, uiWidth, uiHeight, uiVertices);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const auto projection = Mat4::Orthographic(0.0f, static_cast<float>(uiWidth), static_cast<float>(uiHeight), 0.0f);
    gl::UseProgram(uiProgram_);
    gl::UniformMatrix4fv(uiProjectionLocation_, 1, GL_FALSE, projection.m.data());
    gl::BindVertexArray(uiVao_);
    gl::BindBuffer(GL_ARRAY_BUFFER, uiVbo_);
    gl::BufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(uiVertices.size() * sizeof(UiVertex)),
        uiVertices.empty() ? nullptr : uiVertices.data(),
        GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(uiVertices.size()));

    glDisable(GL_BLEND);
    gl::BindVertexArray(0);
    gl::BindBuffer(GL_ARRAY_BUFFER, 0);
}

bool Renderer::CreatePrograms(bool glesContext)
{
    const auto sceneVertexSource = BuildShaderSource(
        glesContext,
        R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uViewProjection;

out vec3 vNormal;
out vec4 vColor;

void main()
{
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
    vNormal = mat3(uModel) * aNormal;
    vColor = aColor;
}
)");

    const auto sceneFragmentSource = BuildShaderSource(
        glesContext,
        R"(
in vec3 vNormal;
in vec4 vColor;

uniform vec4 uTint;
uniform vec3 uLightDirection;

out vec4 fragmentColor;

void main()
{
    vec3 normal = normalize(vNormal);
    float sun = max(dot(normal, normalize(-uLightDirection)), 0.0);
    float sky = max(dot(normal, normalize(vec3(0.35, 0.65, 0.55))), 0.0) * 0.22;
    vec4 base = vColor * uTint;
    vec3 lit = base.rgb * (0.56 + (sun * 0.72) + sky);
    fragmentColor = vec4(lit, base.a);
}
)");

    const auto uiVertexSource = BuildShaderSource(
        glesContext,
        R"(
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uProjection;

out vec4 vColor;

void main()
{
    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
    vColor = aColor;
}
)");

    const auto uiFragmentSource = BuildShaderSource(
        glesContext,
        R"(
in vec4 vColor;
out vec4 fragmentColor;

void main()
{
    fragmentColor = vColor;
}
)");

    auto compileProgram = [](const std::string& vertexSource, const std::string& fragmentSource, GLuint& programOut) -> bool
    {
        std::string error;
        const auto vertexShader = gl::CreateShader(GL_VERTEX_SHADER);
        const auto fragmentShader = gl::CreateShader(GL_FRAGMENT_SHADER);
        if (!CompileShader(vertexShader, vertexSource, error))
        {
            SDL_Log("Vertex shader compile failed: %s", error.c_str());
            gl::DeleteShader(vertexShader);
            gl::DeleteShader(fragmentShader);
            return false;
        }

        if (!CompileShader(fragmentShader, fragmentSource, error))
        {
            SDL_Log("Fragment shader compile failed: %s", error.c_str());
            gl::DeleteShader(vertexShader);
            gl::DeleteShader(fragmentShader);
            return false;
        }

        const auto program = gl::CreateProgram();
        gl::AttachShader(program, vertexShader);
        gl::AttachShader(program, fragmentShader);
        if (!LinkProgram(program, error))
        {
            SDL_Log("Shader link failed: %s", error.c_str());
            gl::DeleteShader(vertexShader);
            gl::DeleteShader(fragmentShader);
            gl::DeleteProgram(program);
            return false;
        }

        gl::DetachShader(program, vertexShader);
        gl::DetachShader(program, fragmentShader);
        gl::DeleteShader(vertexShader);
        gl::DeleteShader(fragmentShader);
        programOut = program;
        return true;
    };

    if (!compileProgram(sceneVertexSource, sceneFragmentSource, sceneProgram_))
    {
        return false;
    }

    if (!compileProgram(uiVertexSource, uiFragmentSource, uiProgram_))
    {
        return false;
    }

    sceneModelLocation_ = gl::GetUniformLocation(sceneProgram_, "uModel");
    sceneViewProjectionLocation_ = gl::GetUniformLocation(sceneProgram_, "uViewProjection");
    sceneTintLocation_ = gl::GetUniformLocation(sceneProgram_, "uTint");
    sceneLightLocation_ = gl::GetUniformLocation(sceneProgram_, "uLightDirection");
    uiProjectionLocation_ = gl::GetUniformLocation(uiProgram_, "uProjection");
    return true;
}

bool Renderer::UploadMesh(const MeshData& mesh, GpuMesh& gpuMesh)
{
    if (mesh.vertices.empty() || mesh.indices.empty())
    {
        return false;
    }

    gl::GenVertexArrays(1, &gpuMesh.vao);
    gl::GenBuffers(1, &gpuMesh.vbo);
    gl::GenBuffers(1, &gpuMesh.ebo);
    gpuMesh.indexCount = static_cast<GLsizei>(mesh.indices.size());

    gl::BindVertexArray(gpuMesh.vao);
    gl::BindBuffer(GL_ARRAY_BUFFER, gpuMesh.vbo);
    gl::BufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)),
        mesh.vertices.data(),
        GL_STATIC_DRAW);

    gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.ebo);
    gl::BufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)),
        mesh.indices.data(),
        GL_STATIC_DRAW);

    gl::EnableVertexAttribArray(0);
    gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, position)));
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    gl::EnableVertexAttribArray(2);
    gl::VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, color)));

    gl::BindVertexArray(0);
    gl::BindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void Renderer::DestroyMeshes()
{
    for (auto& gpuMesh : sceneMeshes_)
    {
        if (gpuMesh.ebo != 0)
        {
            gl::DeleteBuffers(1, &gpuMesh.ebo);
            gpuMesh.ebo = 0;
        }

        if (gpuMesh.vbo != 0)
        {
            gl::DeleteBuffers(1, &gpuMesh.vbo);
            gpuMesh.vbo = 0;
        }

        if (gpuMesh.vao != 0)
        {
            gl::DeleteVertexArrays(1, &gpuMesh.vao);
            gpuMesh.vao = 0;
        }
    }

    sceneMeshes_.clear();
}

void Renderer::BuildUiVertices(const SimulationState& state, int width, int height, std::vector<UiVertex>& vertices) const
{
    const Color buttonFill{0.12f, 0.15f, 0.18f, 0.88f};
    const Color buttonHover{0.17f, 0.21f, 0.25f, 0.92f};
    const Color buttonPressed{0.91f, 0.47f, 0.15f, 0.95f};
    const Color buttonFrame{0.96f, 0.92f, 0.83f, 0.90f};
    const Color buttonText{0.98f, 0.97f, 0.95f, 1.0f};
    const Color hudFill{0.10f, 0.12f, 0.14f, 0.84f};
    const Color hudFrame{0.34f, 0.38f, 0.41f, 0.95f};
    const Color hudText{0.95f, 0.95f, 0.92f, 1.0f};

    for (const auto& button : state.buttons)
    {
        const auto fill = button.pressed ? buttonPressed : (button.hovered ? buttonHover : buttonFill);
        AppendRect(vertices, button.x, button.y, button.width, button.height, fill);
        AppendFrame(vertices, button.x, button.y, button.width, button.height, 2.0f, buttonFrame);
        AppendText(vertices, button.x + 14.0f, button.y + 16.0f, 3.0f, button.label, buttonText);
    }

    const auto panelWidth = 248.0f;
    const auto lineHeight = 18.0f;
    const auto panelHeight = 18.0f + (lineHeight * static_cast<float>(state.hudLines.size())) + 16.0f;
    const auto panelX = static_cast<float>(width) - panelWidth - 20.0f;
    const auto panelY = 20.0f;

    AppendRect(vertices, panelX, panelY, panelWidth, panelHeight, hudFill);
    AppendFrame(vertices, panelX, panelY, panelWidth, panelHeight, 2.0f, hudFrame);

    auto textY = panelY + 14.0f;
    for (const auto& line : state.hudLines)
    {
        AppendText(vertices, panelX + 14.0f, textY, 2.0f, line, hudText);
        textY += lineHeight;
    }

    const auto footer = std::string("COOKIE ON THE ROOF X");
    AppendText(vertices, 20.0f, static_cast<float>(height) - 28.0f, 2.0f, footer, hudText);
}

void Renderer::AppendRect(std::vector<UiVertex>& vertices, float x, float y, float width, float height, const Color& color) const
{
    const auto push = [&vertices, &color](float px, float py)
    {
        vertices.push_back({px, py, color.r, color.g, color.b, color.a});
    };

    push(x, y);
    push(x + width, y);
    push(x + width, y + height);
    push(x, y);
    push(x + width, y + height);
    push(x, y + height);
}

void Renderer::AppendFrame(std::vector<UiVertex>& vertices, float x, float y, float width, float height, float thickness, const Color& color) const
{
    AppendRect(vertices, x, y, width, thickness, color);
    AppendRect(vertices, x, y + height - thickness, width, thickness, color);
    AppendRect(vertices, x, y + thickness, thickness, height - (thickness * 2.0f), color);
    AppendRect(vertices, x + width - thickness, y + thickness, thickness, height - (thickness * 2.0f), color);
}

void Renderer::AppendText(std::vector<UiVertex>& vertices, float x, float y, float scale, const std::string& text, const Color& color) const
{
    auto cursorX = x;
    for (const auto character : text)
    {
        const auto glyph = GetGlyph(character);
        for (std::size_t row = 0; row < glyph.rows.size(); ++row)
        {
            for (int column = 0; column < 5; ++column)
            {
                if ((glyph.rows[row] & (1 << (4 - column))) == 0)
                {
                    continue;
                }

                AppendRect(
                    vertices,
                    cursorX + (static_cast<float>(column) * scale),
                    y + (static_cast<float>(row) * scale),
                    scale,
                    scale,
                    color);
            }
        }

        cursorX += scale * 6.0f;
    }
}
} // namespace cotrx

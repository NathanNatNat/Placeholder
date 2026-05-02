#pragma once

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <string>

namespace placeholder::renderer
{

/// Usage hint for GPU buffer data.
enum class BufferUsage
{
    Static,
    Dynamic,
    Stream
};

/// Index element type.
enum class IndexType
{
    UInt16,
    UInt32
};

/// Primitive topology for draw calls.
enum class PrimitiveTopology
{
    Triangles,
    Lines,
    Points
};

/// Depth comparison function.
enum class DepthFunc
{
    Less,
    LessEqual,
    Equal,
    Greater,
    GreaterEqual,
    Always,
    Never
};

/// Face culling mode.
enum class CullMode
{
    None,
    Back,
    Front
};

/// Blend factor for source/destination blending.
enum class BlendFactor
{
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantAlpha,
    OneMinusConstantAlpha
};

/// Polygon rasterization mode.
enum class PolygonMode
{
    Fill,
    Line,
    Point
};

/// Aggregate render state descriptor.
struct RenderStateDesc
{
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;
    DepthFunc depthFunc = DepthFunc::LessEqual;
    CullMode cullMode = CullMode::Back;
    bool blendEnabled = false;
    BlendFactor srcBlend = BlendFactor::One;
    BlendFactor dstBlend = BlendFactor::Zero;
    PolygonMode polygonMode = PolygonMode::Fill;
};

/// Vertex attribute data type.
enum class VertexAttribType
{
    Float,
    UByte,
    UByteNormalized,
    UInt
};

/// Per-frame data passed through the render pipeline.
struct FrameContext
{
    int viewportWidth = 0;
    int viewportHeight = 0;
    float deltaTime = 0.0f;
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
};

} // namespace placeholder::renderer

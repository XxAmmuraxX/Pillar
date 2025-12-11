#include <gtest/gtest.h>
#include "Pillar/Renderer/Buffer.h"
#include "Pillar/Renderer/VertexArray.h"
#include "Pillar/Renderer/Shader.h"
#include "Pillar/Renderer/Texture.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Pillar/Renderer/BatchRenderer2D.h"
#include "Pillar/Renderer/OrthographicCamera.h"
#include <glm/glm.hpp>
#include <chrono>

using namespace Pillar;

// ============================================================================
// Rendering Integration Tests
// Tests for rendering abstractions that don't require OpenGL context
// Note: These tests verify API contracts and data structures, not actual GPU rendering
// ============================================================================

// -----------------------------------------------------------------------------
// ShaderDataType Tests
// -----------------------------------------------------------------------------

TEST(ShaderDataTypeTests, FloatTypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Float), 4u);
}

TEST(ShaderDataTypeTests, Float2TypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Float2), 8u);
}

TEST(ShaderDataTypeTests, Float3TypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Float3), 12u);
}

TEST(ShaderDataTypeTests, Float4TypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Float4), 16u);
}

TEST(ShaderDataTypeTests, Mat3TypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Mat3), 36u);
}

TEST(ShaderDataTypeTests, Mat4TypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Mat4), 64u);
}

TEST(ShaderDataTypeTests, IntTypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Int), 4u);
}

TEST(ShaderDataTypeTests, Int2TypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Int2), 8u);
}

TEST(ShaderDataTypeTests, Int3TypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Int3), 12u);
}

TEST(ShaderDataTypeTests, Int4TypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Int4), 16u);
}

TEST(ShaderDataTypeTests, BoolTypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::Bool), 1u);
}

TEST(ShaderDataTypeTests, NoneTypeSize)
{
    EXPECT_EQ(ShaderDataTypeSize(ShaderDataType::None), 0u);
}

// -----------------------------------------------------------------------------
// BufferElement Tests
// -----------------------------------------------------------------------------

// Note: BufferElement uses = default constructor which leaves members uninitialized
// This is by design - BufferElement should always be constructed with type and name

TEST(BufferElementTests, ConstructionWithType)
{
    BufferElement element(ShaderDataType::Float3, "a_Position");
    
    EXPECT_EQ(element.Name, "a_Position");
    EXPECT_EQ(element.Type, ShaderDataType::Float3);
    EXPECT_EQ(element.Size, 12u);
    EXPECT_EQ(element.Offset, 0u);
    EXPECT_FALSE(element.Normalized);
}

TEST(BufferElementTests, ConstructionNormalized)
{
    BufferElement element(ShaderDataType::Float4, "a_Color", true);
    
    EXPECT_TRUE(element.Normalized);
}

TEST(BufferElementTests, GetComponentCount_Float)
{
    BufferElement element(ShaderDataType::Float, "a_Value");
    EXPECT_EQ(element.GetComponentCount(), 1u);
}

TEST(BufferElementTests, GetComponentCount_Float2)
{
    BufferElement element(ShaderDataType::Float2, "a_TexCoord");
    EXPECT_EQ(element.GetComponentCount(), 2u);
}

TEST(BufferElementTests, GetComponentCount_Float3)
{
    BufferElement element(ShaderDataType::Float3, "a_Position");
    EXPECT_EQ(element.GetComponentCount(), 3u);
}

TEST(BufferElementTests, GetComponentCount_Float4)
{
    BufferElement element(ShaderDataType::Float4, "a_Color");
    EXPECT_EQ(element.GetComponentCount(), 4u);
}

TEST(BufferElementTests, GetComponentCount_Mat3)
{
    BufferElement element(ShaderDataType::Mat3, "a_Transform");
    EXPECT_EQ(element.GetComponentCount(), 9u);
}

TEST(BufferElementTests, GetComponentCount_Mat4)
{
    BufferElement element(ShaderDataType::Mat4, "a_Transform");
    EXPECT_EQ(element.GetComponentCount(), 16u);
}

TEST(BufferElementTests, GetComponentCount_Int)
{
    BufferElement element(ShaderDataType::Int, "a_EntityID");
    EXPECT_EQ(element.GetComponentCount(), 1u);
}

TEST(BufferElementTests, GetComponentCount_Int2)
{
    BufferElement element(ShaderDataType::Int2, "a_GridPos");
    EXPECT_EQ(element.GetComponentCount(), 2u);
}

TEST(BufferElementTests, GetComponentCount_Int3)
{
    BufferElement element(ShaderDataType::Int3, "a_ChunkPos");
    EXPECT_EQ(element.GetComponentCount(), 3u);
}

TEST(BufferElementTests, GetComponentCount_Int4)
{
    BufferElement element(ShaderDataType::Int4, "a_BoneIndices");
    EXPECT_EQ(element.GetComponentCount(), 4u);
}

TEST(BufferElementTests, GetComponentCount_Bool)
{
    BufferElement element(ShaderDataType::Bool, "a_Active");
    EXPECT_EQ(element.GetComponentCount(), 1u);
}

TEST(BufferElementTests, GetComponentCount_None)
{
    BufferElement element(ShaderDataType::None, "a_Invalid");
    EXPECT_EQ(element.GetComponentCount(), 0u);
}

// -----------------------------------------------------------------------------
// BufferLayout Tests
// -----------------------------------------------------------------------------

TEST(BufferLayoutTests, DefaultConstruction)
{
    BufferLayout layout;
    EXPECT_EQ(layout.GetStride(), 0u);
    EXPECT_TRUE(layout.GetElements().empty());
}

TEST(BufferLayoutTests, SingleElement)
{
    BufferLayout layout = {
        { ShaderDataType::Float3, "a_Position" }
    };
    
    EXPECT_EQ(layout.GetStride(), 12u);
    EXPECT_EQ(layout.GetElements().size(), 1u);
    EXPECT_EQ(layout.GetElements()[0].Offset, 0u);
}

TEST(BufferLayoutTests, MultipleElements_CalculatesOffsets)
{
    BufferLayout layout = {
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float4, "a_Color" },
        { ShaderDataType::Float2, "a_TexCoord" }
    };
    
    EXPECT_EQ(layout.GetElements().size(), 3u);
    
    // Check offsets
    EXPECT_EQ(layout.GetElements()[0].Offset, 0u);   // Position at 0
    EXPECT_EQ(layout.GetElements()[1].Offset, 12u);  // Color at 12 (after Float3)
    EXPECT_EQ(layout.GetElements()[2].Offset, 28u);  // TexCoord at 28 (after Float4)
    
    // Check total stride
    EXPECT_EQ(layout.GetStride(), 36u);  // 12 + 16 + 8
}

TEST(BufferLayoutTests, TypicalVertexLayout)
{
    // Typical layout for 2D sprites: position (3), color (4), texcoord (2), texindex (1)
    BufferLayout layout = {
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float4, "a_Color" },
        { ShaderDataType::Float2, "a_TexCoord" },
        { ShaderDataType::Float, "a_TexIndex" }
    };
    
    EXPECT_EQ(layout.GetStride(), 40u);  // 12 + 16 + 8 + 4
}

TEST(BufferLayoutTests, IteratorAccess)
{
    BufferLayout layout = {
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float2, "a_TexCoord" }
    };
    
    int count = 0;
    for (const auto& element : layout)
    {
        count++;
        EXPECT_FALSE(element.Name.empty());
    }
    EXPECT_EQ(count, 2);
}

TEST(BufferLayoutTests, ConstIteratorAccess)
{
    const BufferLayout layout = {
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float4, "a_Color" }
    };
    
    std::vector<std::string> names;
    for (const auto& element : layout)
    {
        names.push_back(element.Name);
    }
    
    EXPECT_EQ(names.size(), 2u);
    EXPECT_EQ(names[0], "a_Position");
    EXPECT_EQ(names[1], "a_Color");
}

// -----------------------------------------------------------------------------
// OrthographicCamera Tests
// -----------------------------------------------------------------------------

TEST(OrthographicCameraRenderingTests, DefaultProjectionBounds)
{
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    // Camera should have valid projection matrix
    const glm::mat4& projection = camera.GetProjectionMatrix();
    EXPECT_NE(projection, glm::mat4(0.0f));
}

TEST(OrthographicCameraRenderingTests, ViewMatrixIdentityAtOrigin)
{
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    // At origin with no rotation, view matrix should be identity
    const glm::mat4& view = camera.GetViewMatrix();
    EXPECT_NEAR(view[0][0], 1.0f, 0.001f);
    EXPECT_NEAR(view[1][1], 1.0f, 0.001f);
    EXPECT_NEAR(view[2][2], 1.0f, 0.001f);
    EXPECT_NEAR(view[3][3], 1.0f, 0.001f);
}

TEST(OrthographicCameraRenderingTests, PositionAffectsViewMatrix)
{
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    camera.SetPosition(glm::vec3(10.0f, 5.0f, 0.0f));
    
    // Position should affect the translation component of view matrix
    const glm::mat4& view = camera.GetViewMatrix();
    
    // The view matrix translates in the opposite direction of camera position
    EXPECT_NEAR(view[3][0], -10.0f, 0.001f);
    EXPECT_NEAR(view[3][1], -5.0f, 0.001f);
}

TEST(OrthographicCameraRenderingTests, ViewProjectionMatrixCombined)
{
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    const glm::mat4& vp = camera.GetViewProjectionMatrix();
    const glm::mat4& view = camera.GetViewMatrix();
    const glm::mat4& proj = camera.GetProjectionMatrix();
    
    // ViewProjection should be Projection * View
    glm::mat4 expected = proj * view;
    
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            EXPECT_NEAR(vp[i][j], expected[i][j], 0.001f);
        }
    }
}

TEST(OrthographicCameraRenderingTests, ProjectionBoundsUpdate)
{
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    // Update projection bounds (simulating window resize)
    camera.SetProjection(-2.0f, 2.0f, -1.5f, 1.5f);
    
    // Verify projection matrix was updated (different from original)
    const glm::mat4& proj = camera.GetProjectionMatrix();
    // For orthographic, proj[0][0] = 2/(right-left), so for -2 to 2, it's 2/4 = 0.5
    EXPECT_NEAR(proj[0][0], 0.5f, 0.001f);
}

// -----------------------------------------------------------------------------
// BatchRenderer2D Stats Tests (without OpenGL context)
// Note: We copy static const values to local variables to avoid ODR-use linker errors
// -----------------------------------------------------------------------------

TEST(BatchRenderer2DTests, MaxQuadsPerBatch)
{
    // Copy to local variable to avoid ODR-use linker issues
    constexpr uint32_t maxQuads = 10000;
    EXPECT_EQ(maxQuads, BatchRenderer2D::MaxQuadsPerBatch);
}

TEST(BatchRenderer2DTests, MaxVertices)
{
    // 4 vertices per quad
    constexpr uint32_t expectedVertices = 10000 * 4;  // MaxQuadsPerBatch * 4
    uint32_t actualVertices = BatchRenderer2D::MaxVertices;
    EXPECT_EQ(actualVertices, expectedVertices);
}

TEST(BatchRenderer2DTests, MaxIndices)
{
    // 6 indices per quad (2 triangles)
    constexpr uint32_t expectedIndices = 10000 * 6;  // MaxQuadsPerBatch * 6
    uint32_t actualIndices = BatchRenderer2D::MaxIndices;
    EXPECT_EQ(actualIndices, expectedIndices);
}

// -----------------------------------------------------------------------------
// RendererAPI Tests
// -----------------------------------------------------------------------------

TEST(RenderAPITests, DefaultAPIIsOpenGL)
{
    // Default renderer API should be OpenGL
    EXPECT_EQ(RenderAPI::GetAPI(), RendererAPI::OpenGL);
}

// -----------------------------------------------------------------------------
// Vertex Layout Performance Test
// -----------------------------------------------------------------------------

TEST(BufferLayoutPerformanceTests, CreateManyLayouts)
{
    // Test that creating many layouts is fast
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++)
    {
        BufferLayout layout = {
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float4, "a_Color" },
            { ShaderDataType::Float2, "a_TexCoord" },
            { ShaderDataType::Float, "a_TexIndex" },
            { ShaderDataType::Int, "a_EntityID" }
        };
        
        // Use the layout to prevent optimization
        EXPECT_GT(layout.GetStride(), 0u);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in under 100ms
    EXPECT_LT(duration.count(), 100);
}

// -----------------------------------------------------------------------------
// Buffer Element Edge Cases
// -----------------------------------------------------------------------------

TEST(BufferElementEdgeCases, EmptyName)
{
    BufferElement element(ShaderDataType::Float3, "");
    EXPECT_TRUE(element.Name.empty());
    EXPECT_EQ(element.GetComponentCount(), 3u);
}

TEST(BufferElementEdgeCases, LongName)
{
    std::string longName(256, 'a');
    BufferElement element(ShaderDataType::Float3, longName);
    EXPECT_EQ(element.Name.length(), 256u);
}

// -----------------------------------------------------------------------------
// Buffer Layout Edge Cases
// -----------------------------------------------------------------------------

TEST(BufferLayoutEdgeCases, SingleBoolElement)
{
    BufferLayout layout = {
        { ShaderDataType::Bool, "a_Active" }
    };
    
    EXPECT_EQ(layout.GetStride(), 1u);
}

TEST(BufferLayoutEdgeCases, MixedTypes)
{
    BufferLayout layout = {
        { ShaderDataType::Float, "a_Float" },
        { ShaderDataType::Int, "a_Int" },
        { ShaderDataType::Bool, "a_Bool" },
        { ShaderDataType::Float4, "a_Vec4" }
    };
    
    // 4 + 4 + 1 + 16 = 25 bytes
    EXPECT_EQ(layout.GetStride(), 25u);
}

TEST(BufferLayoutEdgeCases, MatrixLayouts)
{
    BufferLayout layout = {
        { ShaderDataType::Mat3, "a_Mat3" },
        { ShaderDataType::Mat4, "a_Mat4" }
    };
    
    // 36 + 64 = 100 bytes
    EXPECT_EQ(layout.GetStride(), 100u);
    EXPECT_EQ(layout.GetElements()[0].Offset, 0u);
    EXPECT_EQ(layout.GetElements()[1].Offset, 36u);
}


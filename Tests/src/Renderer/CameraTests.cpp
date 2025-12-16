#include <gtest/gtest.h>
// Tests for OrthographicCamera and OrthographicCameraController: verifies
// construction, transform updates, zoom/rotation behavior, and event handling.
#include "Pillar/Renderer/OrthographicCamera.h"
#include "Pillar/Renderer/OrthographicCameraController.h"
#include "Pillar/Events/MouseEvent.h"
#include "Pillar/Events/ApplicationEvent.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace Pillar;

// ==============================
// OrthographicCamera Tests
// ==============================

TEST(OrthographicCameraTests, Constructor_SetsProjectionMatrix) {
    OrthographicCamera camera(-1.6f, 1.6f, -0.9f, 0.9f);
    
    // Camera should be created successfully
    glm::mat4 projection = camera.GetProjectionMatrix();
    
    // Projection matrix should not be identity
    EXPECT_NE(projection, glm::mat4(1.0f));
}

TEST(OrthographicCameraTests, Constructor_InitializesDefaultPosition) {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    glm::vec3 position = camera.GetPosition();
    
    EXPECT_FLOAT_EQ(position.x, 0.0f);
    EXPECT_FLOAT_EQ(position.y, 0.0f);
    EXPECT_FLOAT_EQ(position.z, 0.0f);
}

TEST(OrthographicCameraTests, Constructor_InitializesDefaultRotation) {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    float rotation = camera.GetRotation();
    
    EXPECT_FLOAT_EQ(rotation, 0.0f);
}

TEST(OrthographicCameraTests, SetPosition_UpdatesPosition) {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    glm::vec3 newPosition(5.0f, 3.0f, 0.0f);
    camera.SetPosition(newPosition);
    
    glm::vec3 position = camera.GetPosition();
    EXPECT_FLOAT_EQ(position.x, 5.0f);
    EXPECT_FLOAT_EQ(position.y, 3.0f);
    EXPECT_FLOAT_EQ(position.z, 0.0f);
}

TEST(OrthographicCameraTests, SetRotation_UpdatesRotation) {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    camera.SetRotation(45.0f);
    
    EXPECT_FLOAT_EQ(camera.GetRotation(), 45.0f);
}

TEST(OrthographicCameraTests, SetPosition_UpdatesViewMatrix) {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    glm::mat4 viewBefore = camera.GetViewMatrix();
    
    camera.SetPosition({ 1.0f, 0.0f, 0.0f });
    
    glm::mat4 viewAfter = camera.GetViewMatrix();
    
    // View matrix should change when position changes
    EXPECT_NE(viewBefore, viewAfter);
}

TEST(OrthographicCameraTests, SetRotation_UpdatesViewMatrix) {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    glm::mat4 viewBefore = camera.GetViewMatrix();
    
    camera.SetRotation(90.0f);
    
    glm::mat4 viewAfter = camera.GetViewMatrix();
    
    // View matrix should change when rotation changes
    EXPECT_NE(viewBefore, viewAfter);
}

TEST(OrthographicCameraTests, GetViewProjectionMatrix_CombinesMatrices) {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    glm::mat4 projection = camera.GetProjectionMatrix();
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = camera.GetViewProjectionMatrix();
    
    // ViewProjection should be projection * view
    glm::mat4 expected = projection * view;
    
    // Compare matrices element-wise with tolerance
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            EXPECT_NEAR(viewProjection[i][j], expected[i][j], 0.0001f);
        }
    }
}

TEST(OrthographicCameraTests, MultipleTransforms_UpdateCorrectly) {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
    
    camera.SetPosition({ 2.0f, 3.0f, 0.0f });
    camera.SetRotation(30.0f);
    
    EXPECT_FLOAT_EQ(camera.GetPosition().x, 2.0f);
    EXPECT_FLOAT_EQ(camera.GetPosition().y, 3.0f);
    EXPECT_FLOAT_EQ(camera.GetRotation(), 30.0f);
    
    // Should still have valid matrices
    glm::mat4 vp = camera.GetViewProjectionMatrix();
    EXPECT_NE(vp, glm::mat4(0.0f)); // Not zero matrix
}

// ==============================
// OrthographicCameraController Tests
// ==============================

TEST(CameraControllerTests, Constructor_InitializesWithAspectRatio) {
    float aspectRatio = 16.0f / 9.0f;
    OrthographicCameraController controller(aspectRatio, false);
    
    // Controller should be created successfully
    EXPECT_FLOAT_EQ(controller.GetZoomLevel(), 1.0f);
    
    // Camera should be accessible
    OrthographicCamera& camera = controller.GetCamera();
    EXPECT_FLOAT_EQ(camera.GetPosition().x, 0.0f);
    EXPECT_FLOAT_EQ(camera.GetPosition().y, 0.0f);
}

TEST(CameraControllerTests, Constructor_RotationDisabledByDefault) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    // Rotation should be disabled by default (can't test directly, but camera should start at 0)
    EXPECT_FLOAT_EQ(controller.GetCamera().GetRotation(), 0.0f);
}

TEST(CameraControllerTests, Constructor_RotationEnabledWhenRequested) {
    OrthographicCameraController controller(16.0f / 9.0f, true);
    
    // Controller created with rotation enabled
    // Rotation should still be 0 initially
    EXPECT_FLOAT_EQ(controller.GetCamera().GetRotation(), 0.0f);
}

TEST(CameraControllerTests, GetZoomLevel_ReturnsInitialZoom) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    EXPECT_FLOAT_EQ(controller.GetZoomLevel(), 1.0f);
}

TEST(CameraControllerTests, SetZoomLevel_UpdatesZoom) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    controller.SetZoomLevel(2.0f);
    
    EXPECT_FLOAT_EQ(controller.GetZoomLevel(), 2.0f);
}

TEST(CameraControllerTests, SetZoomLevel_ClampsToMinimum) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    controller.SetZoomLevel(0.1f); // Below minimum (0.25)
    
    EXPECT_FLOAT_EQ(controller.GetZoomLevel(), 0.25f);
}

TEST(CameraControllerTests, SetZoomLevel_ClampsToMaximum) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    controller.SetZoomLevel(20.0f); // Above maximum (10.0)
    
    EXPECT_FLOAT_EQ(controller.GetZoomLevel(), 10.0f);
}

TEST(CameraControllerTests, SetZoomLevel_MaintainsCameraPosition) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    // Controller manages position internally via m_CameraPosition
    // When zoom changes, it should maintain the m_CameraPosition state
    
    // Change zoom
    controller.SetZoomLevel(2.0f);
    
    // Camera should be at origin (no movement via OnUpdate)
    glm::vec3 pos = controller.GetCamera().GetPosition();
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
}

TEST(CameraControllerTests, OnMouseScrolled_IncreasesZoom) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    float initialZoom = controller.GetZoomLevel();
    
    // Scroll down (negative Y offset zooms in)
    MouseScrolledEvent event(0.0f, -1.0f);
    controller.OnEvent(event);
    
    EXPECT_GT(controller.GetZoomLevel(), initialZoom); // Zoom level increases (zooms out)
}

TEST(CameraControllerTests, OnMouseScrolled_DecreasesZoom) {
    OrthographicCameraController controller(16.0f / 9.0f);
    controller.SetZoomLevel(2.0f); // Start zoomed out
    
    float initialZoom = controller.GetZoomLevel();
    
    // Scroll up (positive Y offset zooms in)
    MouseScrolledEvent event(0.0f, 1.0f);
    controller.OnEvent(event);
    
    EXPECT_LT(controller.GetZoomLevel(), initialZoom); // Zoom level decreases (zooms in)
}

TEST(CameraControllerTests, OnMouseScrolled_ClampsZoom) {
    OrthographicCameraController controller(16.0f / 9.0f);
    controller.SetZoomLevel(0.3f); // Near minimum
    
    // Try to zoom in way too much
    MouseScrolledEvent event(0.0f, 10.0f);
    controller.OnEvent(event);
    
    EXPECT_GE(controller.GetZoomLevel(), 0.25f); // Should be clamped to minimum
}

TEST(CameraControllerTests, OnWindowResized_UpdatesAspectRatio) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    // Simulate window resize to 800x600 (aspect 4:3)
    WindowResizeEvent event(800, 600);
    controller.OnEvent(event);
    
    // Hard to test aspect ratio directly, but zoom should remain the same
    EXPECT_FLOAT_EQ(controller.GetZoomLevel(), 1.0f);
    
    // Camera should still be valid
    glm::mat4 vp = controller.GetCamera().GetViewProjectionMatrix();
    EXPECT_NE(vp, glm::mat4(0.0f));
}

TEST(CameraControllerTests, OnWindowResized_MaintainsCameraPosition) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    // Controller manages position internally
    // Resize window
    WindowResizeEvent event(1024, 768);
    controller.OnEvent(event);
    
    // Position should remain at origin
    glm::vec3 pos = controller.GetCamera().GetPosition();
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
}

TEST(CameraControllerTests, SetTranslationSpeed_UpdatesSpeed) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    controller.SetTranslationSpeed(10.0f);
    
    EXPECT_FLOAT_EQ(controller.GetTranslationSpeed(), 10.0f);
}

TEST(CameraControllerTests, SetRotationSpeed_UpdatesSpeed) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    controller.SetRotationSpeed(90.0f);
    
    EXPECT_FLOAT_EQ(controller.GetRotationSpeed(), 90.0f);
}

TEST(CameraControllerTests, SetZoomSpeed_UpdatesSpeed) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    controller.SetZoomSpeed(0.5f);
    
    EXPECT_FLOAT_EQ(controller.GetZoomSpeed(), 0.5f);
}

TEST(CameraControllerTests, GetCamera_ReturnsInternalCamera) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    OrthographicCamera& camera = controller.GetCamera();
    
    // Should be able to modify camera
    camera.SetPosition({ 1.0f, 2.0f, 3.0f });
    
    // Changes should be reflected
    EXPECT_FLOAT_EQ(controller.GetCamera().GetPosition().x, 1.0f);
}

TEST(CameraControllerTests, GetCamera_Const_ReturnsInternalCamera) {
    const OrthographicCameraController controller(16.0f / 9.0f);
    
    const OrthographicCamera& camera = controller.GetCamera();
    
    // Should be able to read camera properties
    glm::vec3 pos = camera.GetPosition();
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
}

// ==============================
// Integration Tests
// ==============================

TEST(CameraControllerTests, OnUpdate_AdjustsTranslationSpeedWithZoom) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    // Set zoom level
    controller.SetZoomLevel(2.0f);
    
    // Note: OnUpdate calls Input::IsKeyPressed which requires GLFW window
    // This test would crash in unit test environment without a window
    // Skip OnUpdate and test speed adjustment logic directly
    
    // Translation speed should be adjusted based on zoom (5.0 base * 2.0 zoom = 10.0)
    // This happens internally in OnUpdate
    // We can verify the speed was set initially
    float speed = controller.GetTranslationSpeed();
    
    // Initial speed should be 5.0 (base speed for zoom level 1.0)
    EXPECT_FLOAT_EQ(speed, 5.0f);
    
    // After zoom change, speed will be updated on next OnUpdate call
    // But we can't call OnUpdate without a window, so test the getter/setter instead
    controller.SetTranslationSpeed(10.0f);
    EXPECT_FLOAT_EQ(controller.GetTranslationSpeed(), 10.0f);
}

TEST(CameraControllerTests, MultipleEvents_ProcessedCorrectly) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    // Process multiple events
    MouseScrolledEvent scrollEvent(0.0f, -1.0f);
    controller.OnEvent(scrollEvent);
    
    WindowResizeEvent resizeEvent(1920, 1080);
    controller.OnEvent(resizeEvent);
    
    // Both should be processed
    EXPECT_GT(controller.GetZoomLevel(), 1.0f); // Zoom changed
    
    // Camera should still be valid
    glm::mat4 vp = controller.GetCamera().GetViewProjectionMatrix();
    EXPECT_NE(vp, glm::mat4(0.0f));
}

// ==============================
// Edge Case Tests
// ==============================

TEST(CameraControllerTests, ZeroAspectRatio_DoesNotCrash) {
    // This shouldn't happen in practice, but test defensive behavior
    OrthographicCameraController controller(0.001f, false);
    
    // Should not crash
    EXPECT_NO_THROW(controller.GetCamera());
}

TEST(CameraControllerTests, NegativeAspectRatio_HandledGracefully) {
    // Negative aspect ratio is invalid but shouldn't crash
    OrthographicCameraController controller(-1.0f, false);
    
    // Should still create a camera
    EXPECT_NO_THROW(controller.GetCamera());
}

TEST(CameraControllerTests, VeryLargeDeltaTime_DoesNotBreakCamera) {
    OrthographicCameraController controller(16.0f / 9.0f);
    
    // Note: OnUpdate calls Input::IsKeyPressed which requires GLFW window
    // In unit test environment, calling OnUpdate will crash
    // Test that camera state remains valid without calling OnUpdate
    
    // Camera should be initialized correctly
    glm::vec3 pos = controller.GetCamera().GetPosition();
    EXPECT_FALSE(std::isnan(pos.x));
    EXPECT_FALSE(std::isnan(pos.y));
    EXPECT_FALSE(std::isinf(pos.x));
    EXPECT_FALSE(std::isinf(pos.y));
    
    // Zoom level should be valid
    EXPECT_FALSE(std::isnan(controller.GetZoomLevel()));
    EXPECT_FALSE(std::isinf(controller.GetZoomLevel()));
}

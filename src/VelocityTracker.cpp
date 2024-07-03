#include "VelocityTracker.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>
#include <Globals.h>
#include <Functions.h>

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZScene.h>
#include <Glacier/ZActor.h>
#include <Glacier/SGameUpdateEvent.h>
#include <Glacier/ZObject.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZEngineAppCommon.h>
#include <Glacier/ZFreeCamera.h>
#include <Glacier/ZRender.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/ZHM5InputManager.h>

void VelocityTracker::Init() {
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &VelocityTracker::OnLoadScene);
}

void VelocityTracker::OnEngineInitialized() {
    Logger::Info("VelocityTracker has been initialized!");

    // Register a function to be called on every game frame while the game is in play mode.
    const ZMemberDelegate<VelocityTracker, void(const SGameUpdateEvent&)> s_Delegate(this, &VelocityTracker::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    // Install a hook to print the name of the scene every time the game loads a new one.
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &VelocityTracker::OnLoadScene);
}

VelocityTracker::VelocityTracker() {
    m_maxFrameAverageWindow = 500;

    m_movingAverageXValues = std::vector<float32>(m_maxFrameAverageWindow, 0);
    m_movingAverageYValues = std::vector<float32>(m_maxFrameAverageWindow, 0);
    m_movingAverageZValues = std::vector<float32>(m_maxFrameAverageWindow, 0);
    m_movingAverageSpeedValues = std::vector<float32>(m_maxFrameAverageWindow, 0);
}

VelocityTracker::~VelocityTracker() {
    // Unregister our frame update function when the mod unloads.
    const ZMemberDelegate<VelocityTracker, void(const SGameUpdateEvent&)> s_Delegate(this, &VelocityTracker::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void VelocityTracker::OnFrameUpdate(const SGameUpdateEvent &p_UpdateEvent) {
    if(!m_agentIsActive || !m_velocityModActive)
        return;

    // Get reference to local hitman
    TEntityRef<ZHitman5> s_LocalHitman;
    Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

    // Check if the reference was supplied
    if(s_LocalHitman) {
        // Query ZSpatialEntity connected to local hitman
        const ZSpatialEntity* agentSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
        std::stringstream agentPositionStream;

        const double timeSinceLastFrame = p_UpdateEvent.m_GameTimeDelta.ToSeconds();

        // Set the agent position
        m_agentPosition = agentSpatialEntity->m_mTransform.Trans;

        // If its the first time checking, set previous to current position, and toggle m_setPrevPosition
        if(!m_setPrevPosition) {
            m_agentPreviousPosition = m_agentPosition;
            m_setPrevPosition = true;
        }
        // Find the delta position for current and previous position
        m_agentDeltaPosition = m_agentPosition - m_agentPreviousPosition;

        const float xSpeed = m_agentDeltaPosition.x / static_cast<float32>(timeSinceLastFrame);
        const float ySpeed = m_agentDeltaPosition.y / static_cast<float32>(timeSinceLastFrame);
        const float zSpeed = m_agentDeltaPosition.z / static_cast<float32>(timeSinceLastFrame);
        const float speed = std::sqrt(xSpeed * xSpeed + ySpeed * ySpeed + zSpeed * zSpeed);

        // Update Moving Average values
        m_movingAverageXValues.insert(m_movingAverageXValues.begin(), xSpeed);
        m_movingAverageXValues.pop_back();

        m_movingAverageYValues.insert(m_movingAverageYValues.begin(), ySpeed);
        m_movingAverageYValues.pop_back();

        m_movingAverageZValues.insert(m_movingAverageZValues.begin(), zSpeed);
        m_movingAverageZValues.pop_back();

        m_movingAverageSpeedValues.insert(m_movingAverageSpeedValues.begin(), speed);
        m_movingAverageSpeedValues.pop_back();

        float32 xSpeedAverage = 0, ySpeedAverage = 0, zSpeedAverage = 0, speedAverage = 0;
        for(int i = 0; i < m_frameAverageWindow; i++) {
            xSpeedAverage += m_movingAverageXValues[i];
            ySpeedAverage += m_movingAverageYValues[i];
            zSpeedAverage += m_movingAverageZValues[i];
            speedAverage += m_movingAverageSpeedValues[i];
        }
        xSpeedAverage = xSpeedAverage / static_cast<float32>(m_frameAverageWindow);
        ySpeedAverage = ySpeedAverage / static_cast<float32>(m_frameAverageWindow);
        zSpeedAverage = zSpeedAverage / static_cast<float32>(m_frameAverageWindow);
        speedAverage = speedAverage / static_cast<float32>(m_frameAverageWindow);

        m_agentSpeed = SVector4(
            xSpeed,
            ySpeed,
            zSpeed,
            speed
            );
        m_agentAverageSpeed = SVector4(
            xSpeedAverage,
            ySpeedAverage,
            zSpeedAverage,
            speedAverage
            );
        // Set previous position to current position for next frame
        m_agentPreviousPosition = m_agentPosition;
    }
}

void VelocityTracker::OnDrawMenu() {
    ImGui::Checkbox("Velocity", &m_velocityModActive);
    if(m_velocityModActive) {
        if(ImGui::BeginMenu("Velocity Settings")) {
            ImGui::SliderInt("Frame Average Window", &m_frameAverageWindow, 1, m_maxFrameAverageWindow, "%d", ImGuiSliderFlags_AlwaysClamp);
            ImGui::Checkbox("Show Raw Speeds", &m_showRawSpeeds);
            ImGui::Checkbox("Show Average Speeds", &m_showAverageSpeeds);
            ImGui::EndMenu();
        }
    }
}


void VelocityTracker::OnDraw3D(IRenderer *p_Renderer) {
    if(!m_agentIsActive || !m_velocityModActive)
        return;

    std::stringstream posTextStream;

    const ImGuiIO& guiIO = ImGui::GetIO();

    const float32 screenWidth = guiIO.DisplaySize.x * guiIO.DisplayFramebufferScale.x;

    // Either 0 or 4 depending on if raw speeds are shown
    int overlayPrintStartIndex = m_showRawSpeeds ? 0 : 4;
    // Either 4 or 8 depending on if average speeds are shown
    int overlayPrintEndIndex = m_showAverageSpeeds ? 8 : 4;
    // Seperate iterator for printing lines - since i can start at either 0 or 4
    int lineCount = 0;

    for(int i = overlayPrintStartIndex; i < overlayPrintEndIndex; i++) {
        switch(i) {
            case 0:
                posTextStream << "v:" << std::fixed << std::setprecision(3) << m_agentSpeed.w;
                break;
            case 1:
                posTextStream << "vX:" << std::fixed << std::setprecision(3) << m_agentSpeed.x;
                break;
            case 2:
                posTextStream << "vY:" << std::fixed << std::setprecision(3) << m_agentSpeed.y;
                break;
            case 3:
                posTextStream << "vZ:" << std::fixed << std::setprecision(3) << m_agentSpeed.z;
                break;
            case 4:
                posTextStream << "aV:" << std::fixed << std::setprecision(3) << m_agentAverageSpeed.w;
                break;
            case 5:
                posTextStream << "avX:" << std::fixed << std::setprecision(3) << m_agentAverageSpeed.x;
                break;
            case 6:
                posTextStream << "avY:" << std::fixed << std::setprecision(3) << m_agentAverageSpeed.y;
                break;
            case 7:
                posTextStream << "avZ:" << std::fixed << std::setprecision(3) << m_agentAverageSpeed.z;
                break;
        }

        const float32 scale = i % 4 == 0 ? 0.7 : 0.5;
        const SVector4 color = i % 4 == 0 ? SVector4(.2, 1, .2, 0) : SVector4(1, 1, 1, 0);

        p_Renderer->DrawText2D(
            ZString(posTextStream.str()),
            SVector2(screenWidth - 150, 20 + (40 * lineCount)),
            color,
            0.0,
            scale,
            TextAlignment::Left);
        posTextStream.str(std::string());
        lineCount++;
    }
}


DEFINE_PLUGIN_DETOUR(VelocityTracker, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData) {
    if(p_SceneData.m_type == "") {
        m_agentIsActive = false;
    } else {
        m_agentIsActive = true;
    }


    return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(VelocityTracker);

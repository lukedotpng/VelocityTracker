#pragma once
#include <vector>
#include <IPluginInterface.h>
#include <Glacier/TArray.h>
#include <Glacier/SGameUpdateEvent.h>

class VelocityTracker : public IPluginInterface {
public:
    void Init() override;
    void OnEngineInitialized() override;
    VelocityTracker();
    ~VelocityTracker() override;

    void OnDraw3D(IRenderer *p_Renderer) override;
    void OnDrawMenu() override;
private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);


    // Declare plugin detour with PluginClass, Return Type, Detour Name, __VA_ARGS__
    DECLARE_PLUGIN_DETOUR(VelocityTracker, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData);

private:
    bool m_velocityModActive = false;
    int m_frameAverageWindow = 1;
    int m_maxFrameAverageWindow = 200;
    bool m_showAverageSpeeds = false;
    bool m_showRawSpeeds = true;
    SVector3 m_agentPosition = SVector3(0, 0, 0);
    SVector3 m_agentPreviousPosition = SVector3(0, 0, 0);
    SVector3 m_agentDeltaPosition = SVector3(0, 0, 0);
    SVector4 m_agentSpeed = SVector4(0, 0, 0, 0);
    SVector4 m_agentAverageSpeed = SVector4(0, 0, 0, 0);
    bool m_setPrevPosition = false;
    bool m_agentIsActive = false;
    std::vector<float32> m_movingAverageXValues;
    std::vector<float32> m_movingAverageYValues;
    std::vector<float32> m_movingAverageZValues;
    std::vector<float32> m_movingAverageSpeedValues;
};

DEFINE_ZHM_PLUGIN(VelocityTracker)

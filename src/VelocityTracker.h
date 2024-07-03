#pragma once

#include <IPluginInterface.h>

#include <Glacier/SGameUpdateEvent.h>

class VelocityTracker : public IPluginInterface {
public:
    void OnEngineInitialized() override;
    ~VelocityTracker() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    DECLARE_PLUGIN_DETOUR(VelocityTracker, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData);

private:
    bool m_ShowMessage = false;
};

DEFINE_ZHM_PLUGIN(VelocityTracker)

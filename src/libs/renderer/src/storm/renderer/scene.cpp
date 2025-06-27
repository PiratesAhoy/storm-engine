#include "storm/renderer/scene.hpp"

#include "core.h"
#include "string_compare.hpp"
#include "vma.hpp"

namespace {

using Scene = storm::Scene;

CREATE_CLASS(Scene)

} // namespace

namespace storm {

bool Scene::Init()
{
    return true;
}

void Scene::ProcessStage(Stage stage, uint32_t delta)
{
    if (stage == Stage::execute)
    {
        if (activeCamera_ != invalid_entity)
        {
            auto *camera = static_cast<Camera*>(core.GetEntityPointerSafe(activeCamera_) );
            if (camera != nullptr)
            {
                const uint32_t delta_time = core.GetDeltaTime();
                if (delta_time > 1e-5f)
                {
                    camera->Execute(core.GetRDeltaTime());
                }
                else
                {
                    camera->Execute(delta_time);
                }
            }
            else
            {
                activeCamera_ = invalid_entity;
            }
        }
    }
}

Scene &Scene::SetActiveCamera(Camera *camera)
{
    if (camera == nullptr) {
        activeCamera_ = invalid_entity;
    }
    else {
        activeCamera_ = camera->GetId();
    }
    return *this;
}

Camera *Scene::GetActiveCamera() const
{

    return static_cast<Camera*>(core.GetEntityPointerSafe(activeCamera_) );
}

uint64_t Scene::ProcessMessage(MESSAGE &msg)
{
    const std::string &command = msg.String();

    if (iEquals(command, "SetActiveCamera"))
    {
        const entid_t camera_id = msg.EntityID();
        activeCamera_ = camera_id;
        return 0;
    }

    return 0;
}

Scene &Scene::GetDefaultScene()
{
    entid_t scene_id = core.GetEntityId("Scene");
    if (scene_id == invalid_entity) {
        scene_id = core.CreateEntity("Scene");
        core.AddToLayer(EXECUTE, scene_id, 0);
        core.AddToLayer(SEA_EXECUTE, scene_id, 0);
    }
    return *dynamic_cast<Scene*>(core.GetEntityPointerSafe(scene_id) );
}

} // namespace storm
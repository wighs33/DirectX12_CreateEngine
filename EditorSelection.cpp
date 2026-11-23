#include "EditorSelection.h"

#include "GameObject.h"
#include "Scene.h"

void EditorSelection::Select(std::size_t objectIndex)
{
    selectedIndex_ = objectIndex;
}

void EditorSelection::Clear()
{
    selectedIndex_ = InvalidIndex;
    isDeleteRequested_ = false;
}

bool EditorSelection::IsSelected(std::size_t objectIndex) const
{
    return selectedIndex_ != InvalidIndex && selectedIndex_ == objectIndex;
}

GameObject* EditorSelection::GetSelected(Scene& scene) const
{
    return selectedIndex_ < scene.GetGameObjectCount()
        ? &scene.GetGameObjects()[selectedIndex_]
        : nullptr;
}

void EditorSelection::RequestDelete()
{
    isDeleteRequested_ = true;
}

void EditorSelection::ProcessDeleteRequest(Scene& scene)
{
    if (!isDeleteRequested_)
    {
        return;
    }

    isDeleteRequested_ = false;
    scene.RemoveGameObject(selectedIndex_);
    Clear();
}

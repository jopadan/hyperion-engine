/* Copyright (c) 2024 No Tomorrow Games. All rights reserved. */

#include <scene/ecs/systems/UISystem.hpp>
#include <scene/ecs/EntityManager.hpp>

namespace hyperion {

void UISystem::OnEntityAdded(const Handle<Entity> &entity)
{
    SystemBase::OnEntityAdded(entity);

}

void UISystem::OnEntityRemoved(ID<Entity> entity)
{
    SystemBase::OnEntityRemoved(entity);
}

void UISystem::Process(GameCounter::TickUnit delta)
{
    // do nothing
}

} // namespace hyperion

/* Copyright (c) 2024 No Tomorrow Games. All rights reserved. */

#include <rendering/HBAO.hpp>
#include <rendering/RenderGroup.hpp>
#include <rendering/RenderEnvironment.hpp>
#include <rendering/Scene.hpp>
#include <rendering/Camera.hpp>
#include <rendering/PlaceholderData.hpp>
#include <rendering/RenderState.hpp>

#include <rendering/backend/RendererGraphicsPipeline.hpp>

#include <system/AppContext.hpp>

#include <core/profiling/ProfileScope.hpp>

#include <Engine.hpp>

namespace hyperion {

#pragma region Render commands

struct RENDER_COMMAND(AddHBAOFinalImagesToGlobalDescriptorSet) : renderer::RenderCommand
{
    FixedArray<ImageViewRef, max_frames_in_flight> pass_image_views;

    RENDER_COMMAND(AddHBAOFinalImagesToGlobalDescriptorSet)(FixedArray<ImageViewRef, max_frames_in_flight> &&pass_image_views)
        : pass_image_views(std::move(pass_image_views))
    {
    }

    virtual ~RENDER_COMMAND(AddHBAOFinalImagesToGlobalDescriptorSet)() override = default;

    virtual RendererResult operator()() override
    {
        for (uint32 frame_index = 0; frame_index < max_frames_in_flight; frame_index++) {
            g_engine->GetGlobalDescriptorTable()->GetDescriptorSet(NAME("Global"), frame_index)
                ->SetElement(NAME("SSAOResultTexture"), pass_image_views[frame_index]);
        }

        HYPERION_RETURN_OK;
    }
};

struct RENDER_COMMAND(RemoveHBAODescriptors) : renderer::RenderCommand
{
    RENDER_COMMAND(RemoveHBAODescriptors)()
    {
    }

    virtual ~RENDER_COMMAND(RemoveHBAODescriptors)() override = default;

    virtual RendererResult operator()() override
    {
        RendererResult result;

        for (uint32 frame_index = 0; frame_index < max_frames_in_flight; frame_index++) {
            g_engine->GetGlobalDescriptorTable()->GetDescriptorSet(NAME("Global"), frame_index)
                ->SetElement(NAME("SSAOResultTexture"), g_engine->GetPlaceholderData()->GetImageView2D1x1R8());
        }

        return result;
    }
};

#pragma endregion Render commands

HBAO::HBAO()
    : FullScreenPass(InternalFormat::RGBA8)
{
}

HBAO::~HBAO()
{
    m_temporal_blending.Reset();

    PUSH_RENDER_COMMAND(RemoveHBAODescriptors);
}

void HBAO::Create()
{
    HYP_SCOPE;
    
    ShaderProperties shader_properties;
    shader_properties.Set("HBIL_ENABLED", g_engine->GetAppContext()->GetConfiguration().Get("rendering.hbil.enabled").ToBool());

    m_shader = g_shader_manager->GetOrCreate(
        NAME("HBAO"),
        shader_properties
    );

    FullScreenPass::Create();

    CreateTemporalBlending();

    PUSH_RENDER_COMMAND(
        AddHBAOFinalImagesToGlobalDescriptorSet,
        FixedArray<ImageViewRef, max_frames_in_flight> {
            m_temporal_blending ? m_temporal_blending->GetResultTexture()->GetImageView() : GetAttachment(0)->GetImageView(),
            m_temporal_blending ? m_temporal_blending->GetResultTexture()->GetImageView() : GetAttachment(0)->GetImageView()
        }
    );
}

void HBAO::CreatePipeline(const RenderableAttributeSet &renderable_attributes)
{
    HYP_SCOPE;

    renderer::DescriptorTableDeclaration descriptor_table_decl = m_shader->GetCompiledShader()->GetDescriptorUsages().BuildDescriptorTable();

    DescriptorTableRef descriptor_table = MakeRenderObject<DescriptorTable>(descriptor_table_decl);
    DeferCreate(descriptor_table, g_engine->GetGPUDevice());

    m_descriptor_table = descriptor_table;

    m_render_group = CreateObject<RenderGroup>(
        m_shader,
        renderable_attributes,
        *m_descriptor_table,
        RenderGroupFlags::NONE
    );

    m_render_group->AddFramebuffer(m_framebuffer);

    InitObject(m_render_group);
}

void HBAO::CreateTemporalBlending()
{
    HYP_SCOPE;

    m_temporal_blending = MakeUnique<TemporalBlending>(
        GetFramebuffer()->GetExtent(),
        InternalFormat::RGBA8,
        TemporalBlendTechnique::TECHNIQUE_3,
        TemporalBlendFeedback::LOW,
        GetFramebuffer()
    );

    m_temporal_blending->Create();
}

void HBAO::Resize_Internal(Vec2u new_size)
{
    HYP_SCOPE;

    FullScreenPass::Resize_Internal(new_size);

    m_temporal_blending.Reset();

    CreateTemporalBlending();

    PUSH_RENDER_COMMAND(
        AddHBAOFinalImagesToGlobalDescriptorSet,
        FixedArray<ImageViewRef, max_frames_in_flight> {
            m_temporal_blending ? m_temporal_blending->GetResultTexture()->GetImageView() : GetAttachment(0)->GetImageView(),
            m_temporal_blending ? m_temporal_blending->GetResultTexture()->GetImageView() : GetAttachment(0)->GetImageView()
        }
    );
}

void HBAO::Record(uint32 frame_index)
{
}

void HBAO::Render(Frame *frame)
{
    HYP_SCOPE;
    Threads::AssertOnThread(ThreadName::THREAD_RENDER);

    const uint32 frame_index = frame->GetFrameIndex();
    const CommandBufferRef &command_buffer = frame->GetCommandBuffer();

    const SceneRenderResources *scene_render_resources = g_engine->GetRenderState()->GetActiveScene();
    const CameraRenderResources *camera_render_resources = &g_engine->GetRenderState()->GetActiveCamera();

    {
        struct alignas(128)
        {
            Vec2u   dimension;
        } push_constants;

        push_constants.dimension = m_extent;

        GetRenderGroup()->GetPipeline()->SetPushConstants(&push_constants, sizeof(push_constants));
        Begin(frame);

        Frame temporary_frame = Frame::TemporaryFrame(GetCommandBuffer(frame_index), frame_index);
        
        GetRenderGroup()->GetPipeline()->GetDescriptorTable()->Bind(
            &temporary_frame,
            GetRenderGroup()->GetPipeline(),
            {
                {
                    NAME("Scene"),
                    {
                        { NAME("ScenesBuffer"), ShaderDataOffset<SceneShaderData>(scene_render_resources) },
                        { NAME("CamerasBuffer"), ShaderDataOffset<CameraShaderData>(camera_render_resources) }
                    }
                }
            }
        );
        
        GetQuadMesh()->Render(GetCommandBuffer(frame_index));
        End(frame);
    }
    
    m_temporal_blending->Render(frame);
}

} // namespace hyperion
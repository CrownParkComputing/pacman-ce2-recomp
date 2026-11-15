#include <cassert>
#include <cstdio>
#include "video_core/renderer_vulkan/vk_render_state.h"

int main() {
    Vulkan::RenderState load{};
    load.width = 1280;
    load.height = 720;
    load.num_layers = 1;
    load.num_color_attachments = 1;
    load.color_attachments[0].image_view = reinterpret_cast<VkImageView>(0x101);
    load.color_attachments[0].image_layout = vk::ImageLayout::eColorAttachmentOptimal;
    auto& depth = load.depth_stencil_attachment;
    depth.image_view = reinterpret_cast<VkImageView>(0x102);
    depth.image_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depth.has_depth = true;
    depth.has_stencil = true;

    auto clear = load;
    clear.color_attachments[0].is_clear = 1;
    clear.color_attachments[0].clear_value = {1, 2, 3, 4};
    clear.depth_stencil_attachment.depth_clear = true;
    clear.depth_stencil_attachment.stencil_clear = true;
    clear.depth_stencil_attachment.clear_value = {0x3f800000, 7, 0, 0};
    auto open_pass = clear;
    open_pass.ConsumeLoadOperations();
    // A clear followed by an otherwise identical load continues the open pass.
    assert(open_pass == load);
    assert(open_pass.depth_stencil_attachment.has_depth);
    assert(open_pass.depth_stencil_attachment.has_stencil);
    // Repeating even the exact same clear must execute it again.
    assert(!(open_pass == clear));
    auto color_only_clear = load;
    color_only_clear.color_attachments[0].is_clear = 1;
    assert(!(open_pass == color_only_clear));
    auto depth_only_clear = load;
    depth_only_clear.depth_stencil_attachment.depth_clear = true;
    assert(!(open_pass == depth_only_clear));
    auto stencil_only_clear = load;
    stencil_only_clear.depth_stencil_attachment.stencil_clear = true;
    assert(!(open_pass == stencil_only_clear));
    // Reuse never hides a changed target, layout, extent or stencil presence.
    auto changed = load;
    changed.color_attachments[0].image_view = reinterpret_cast<VkImageView>(0x103);
    assert(!(open_pass == changed));
    changed = load;
    changed.depth_stencil_attachment.image_layout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    assert(!(open_pass == changed));
    changed = load;
    changed.width = 1920;
    assert(!(open_pass == changed));
    changed = load;
    changed.depth_stencil_attachment.has_stencil = false;
    assert(!(open_pass == changed));
    open_pass.ConsumeLoadOperations();
    assert(open_pass == load);
    std::puts("render-loads-test: OK");
}

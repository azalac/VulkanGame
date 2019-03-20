

list(APPEND HEADER_FILES
"include/VulkanShader.hpp"
"include/VulkanRenderPipeline.hpp"
"include/Window.hpp"
"include/VulkanRenderer.hpp"
"include/VulkanCommandBuffer.hpp"
"include/VulkanSwap.hpp"
"include/VulkanDevice.hpp"
"include/VulkanSingleCommand.hpp"
"include/VulkanDepthBuffer.hpp"
"include/VulkanInst.hpp"
"include/VulkanImage.hpp"
"include/VulkanRenderPass.hpp"
"include/VulkanDescriptor.hpp"
"include/VulkanVertex.hpp"
"include/VulkanBuffer.hpp"
"include/VulkanController.hpp"
"include/game/scene.hpp"
"include/game/ObjectController.hpp"
"include/game/Collision.hpp"
"include/game/RigidBody.hpp"
"include/game/Menu.hpp")

list(APPEND SOURCE_FILES
"src/Window.cpp"
"src/main.cpp"
"src/ObjectController.cpp"
"src/helpers/VulkanRenderPass.cpp"
"src/helpers/VulkanShader.cpp"
"src/helpers/VulkanRenderPipeline.cpp"
"src/helpers/VulkanCommandBuffer.cpp"
"src/helpers/VulkanDescriptor.cpp"
"src/helpers/VulkanSingleCommand.cpp"
"src/helpers/VulkanDepthBuffer.cpp"
"src/helpers/VulkanImage.cpp")

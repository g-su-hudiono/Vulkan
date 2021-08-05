..\lib\VulkanSDK\Bin\glslc.exe shader.vert -o spv/vert.spv
..\lib\VulkanSDK\Bin\glslc.exe shader.frag -o spv/frag.spv

..\lib\VulkanSDK\Bin\glslc.exe raytracing/frag_shader.frag		--target-env=vulkan1.2 -o spv/frag_shader.frag.spv
..\lib\VulkanSDK\Bin\glslc.exe raytracing/passthrough.vert		--target-env=vulkan1.2 -o spv/passthrough.vert.spv
..\lib\VulkanSDK\Bin\glslc.exe raytracing/post.frag				--target-env=vulkan1.2 -o spv/post.frag.spv
..\lib\VulkanSDK\Bin\glslc.exe raytracing/raytrace.rchit		--target-env=vulkan1.2 -o spv/raytrace.rchit.spv
..\lib\VulkanSDK\Bin\glslc.exe raytracing/raytrace.rgen			--target-env=vulkan1.2 -o spv/raytrace.rgen.spv
..\lib\VulkanSDK\Bin\glslc.exe raytracing/raytrace.rmiss		--target-env=vulkan1.2 -o spv/raytrace.rmiss.spv
..\lib\VulkanSDK\Bin\glslc.exe raytracing/raytraceShadow.rmiss	--target-env=vulkan1.2 -o spv/raytraceShadow.rmiss.spv
..\lib\VulkanSDK\Bin\glslc.exe raytracing/vert_shader.vert		--target-env=vulkan1.2 -o spv/vert_shader.vert.spv

pause

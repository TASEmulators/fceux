#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <vector>

#include <QExposeEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <vulkan/vulkan.h>
#include <QVulkanInstance>

#include "Qt/ConsoleViewerVulkan.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/fceuWrapper.h"
#include "Qt/nes_shm.h"
#include "Qt/throttle.h"

extern unsigned int gui_draw_area_width;
extern unsigned int gui_draw_area_height;

class ConsoleVulkanWindow_t : public QWindow
{
public:
	ConsoleVulkanWindow_t()
	{
		setSurfaceType(QSurface::VulkanSurface);
		resize(256, 224);
		instanceReady = instance.create();
		if (instanceReady)
			setVulkanInstance(&instance);
	}

	~ConsoleVulkanWindow_t() override
	{
		cleanup();
	}

	void shutdown()
	{
		exposedCallback = {};
		cleanup();
	}

	bool initialize(QString &error)
	{
		if (!instanceReady)
		{
			error = QStringLiteral("Could not create a Vulkan instance");
			return false;
		}
		create();
		surface = instance.surfaceForWindow(this);
		if (surface == VK_NULL_HANDLE || !selectDevice(error) || !createDevice(error) ||
		    !createCommandPool(error) || !createSync(error) || !recreateSwapchain(error))
		{
			cleanup();
			return false;
		}
		return true;
	}

	void setVsync(bool enabled)
	{
		if (vsyncEnabled != enabled)
		{
			vsyncEnabled = enabled;
			swapchainDirty = true;
		}
	}

	void setExposedCallback(std::function<void()> callback)
	{
		exposedCallback = std::move(callback);
	}

	void render(const uint32_t *pixels, int width, int height, const QColor &background,
	            bool forceAspect, double aspectX, double aspectY, bool linearFilter)
	{
		if (!isExposed() || !device || !pixels || width <= 0 || height <= 0)
			return;
		if (swapchainDirty && !recreateSwapchain(lastError))
			return;
		if (!swapchain || !ensureTexture(width, height) || !ensureStaging(width, height))
			return;

		uint32_t *mapped = nullptr;
		if (vkMapMemory(device, stagingMemory, 0, stagingSize, 0, (void **)&mapped) != VK_SUCCESS)
			return;
		memcpy(mapped, pixels, (size_t)width * height * sizeof(uint32_t));
		vkUnmapMemory(device, stagingMemory);

		uint32_t imageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
			imageAvailable, VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			swapchainDirty = true;
			return;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			return;

		vkResetCommandBuffer(commandBuffer, 0);
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
			return;

		VkImageMemoryBarrier textureToTransfer{};
		textureToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		textureToTransfer.srcAccessMask = textureInitialized ? VK_ACCESS_TRANSFER_READ_BIT : 0;
		textureToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		textureToTransfer.oldLayout = textureInitialized ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
		textureToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		textureToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		textureToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		textureToTransfer.image = textureImage;
		textureToTransfer.subresourceRange = colorRange();
		vkCmdPipelineBarrier(commandBuffer,
			textureInitialized ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &textureToTransfer);

		VkBufferImageCopy copy{};
		copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.layerCount = 1;
		copy.imageExtent = { (uint32_t)width, (uint32_t)height, 1 };
		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		VkImageMemoryBarrier textureToSource = textureToTransfer;
		textureToSource.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		textureToSource.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		textureToSource.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		textureToSource.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &textureToSource);
		textureInitialized = true;

		VkImage image = swapchainImages[imageIndex];
		VkImageMemoryBarrier targetToTransfer{};
		targetToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		targetToTransfer.srcAccessMask = 0;
		targetToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		targetToTransfer.oldLayout = swapchainLayouts[imageIndex];
		targetToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		targetToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		targetToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		targetToTransfer.image = image;
		targetToTransfer.subresourceRange = colorRange();
		vkCmdPipelineBarrier(commandBuffer,
			targetToTransfer.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &targetToTransfer);

		VkClearColorValue clearColor{};
		clearColor.float32[0] = background.redF();
		clearColor.float32[1] = background.greenF();
		clearColor.float32[2] = background.blueF();
		clearColor.float32[3] = 1.0f;
		VkImageSubresourceRange range = colorRange();
		vkCmdClearColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			&clearColor, 1, &range);

		int x = 0, y = 0, drawWidth = (int)swapchainExtent.width, drawHeight = (int)swapchainExtent.height;
		if (forceAspect && aspectX > 0.0 && aspectY > 0.0)
		{
			double ratio = aspectX / aspectY;
			if ((double)drawWidth / drawHeight > ratio)
				drawWidth = (int)(drawHeight * ratio);
			else
				drawHeight = (int)(drawWidth / ratio);
			x = ((int)swapchainExtent.width - drawWidth) / 2;
			y = ((int)swapchainExtent.height - drawHeight) / 2;
		}

		VkImageBlit blit{};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.layerCount = 1;
		blit.srcOffsets[1] = { width, height, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.layerCount = 1;
		blit.dstOffsets[0] = { x, y, 0 };
		blit.dstOffsets[1] = { x + drawWidth, y + drawHeight, 1 };
		vkCmdBlitImage(commandBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
			linearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST);

		VkImageMemoryBarrier targetToPresent = targetToTransfer;
		targetToPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		targetToPresent.dstAccessMask = 0;
		targetToPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		targetToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &targetToPresent);
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
			return;

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &imageAvailable;
		submit.pWaitDstStageMask = &waitStage;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &commandBuffer;
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &renderFinished;
		if (vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS)
			return;

		VkPresentInfoKHR present{};
		present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present.waitSemaphoreCount = 1;
		present.pWaitSemaphores = &renderFinished;
		present.swapchainCount = 1;
		present.pSwapchains = &swapchain;
		present.pImageIndices = &imageIndex;
		result = vkQueuePresentKHR(queue, &present);
		vkQueueWaitIdle(queue);
		swapchainLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
		{
			videoBufferSwapMark();
			if (nes_shm)
				nes_shm->render_count++;
		}
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			swapchainDirty = true;
	}

	void setLinearFilter(bool enabled) { linearFilter = enabled; }
	void setBackground(const QColor &color) { background = color; }
	unsigned int mouseButtons() const { return mouseButtonMask; }
	const QString &errorString() const { return lastError; }

protected:
	void exposeEvent(QExposeEvent *event) override
	{
		QWindow::exposeEvent(event);
		if (isExposed())
		{
			swapchainDirty = true;
			if (exposedCallback)
				exposedCallback();
		}
	}

	void resizeEvent(QResizeEvent *event) override
	{
		QWindow::resizeEvent(event);
		swapchainDirty = true;
	}
	void mousePressEvent(QMouseEvent *event) override
	{
		mouseButtonMask = event->buttons();
		event->accept();
	}
	void mouseReleaseEvent(QMouseEvent *event) override
	{
		mouseButtonMask = event->buttons();
		event->accept();
	}

private:
	VkImageSubresourceRange colorRange() const
	{
		VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.levelCount = 1;
		range.layerCount = 1;
		return range;
	}

	bool selectDevice(QString &error)
	{
		uint32_t count = 0;
		vkEnumeratePhysicalDevices(instance.vkInstance(), &count, nullptr);
		std::vector<VkPhysicalDevice> devices(count);
		if (!count || vkEnumeratePhysicalDevices(instance.vkInstance(), &count, devices.data()) != VK_SUCCESS)
		{
			error = QStringLiteral("No Vulkan-capable device was found");
			return false;
		}
		for (VkPhysicalDevice candidate : devices)
		{
			uint32_t familyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
			std::vector<VkQueueFamilyProperties> families(familyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
			for (uint32_t i = 0; i < familyCount; ++i)
			{
				VkBool32 presentSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &presentSupport);
				if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
				{
					physicalDevice = candidate;
					queueFamily = i;
					return true;
				}
			}
		}
		error = QStringLiteral("No Vulkan queue supports graphics and presentation");
		return false;
	}

	bool createDevice(QString &error)
	{
		float priority = 1.0f;
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &priority;
		const char *extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkDeviceCreateInfo deviceInfo{};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.queueCreateInfoCount = 1;
		deviceInfo.pQueueCreateInfos = &queueInfo;
		deviceInfo.enabledExtensionCount = 1;
		deviceInfo.ppEnabledExtensionNames = extensions;
		if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
		{
			error = QStringLiteral("Could not create a Vulkan device with swapchain support");
			return false;
		}
		vkGetDeviceQueue(device, queueFamily, 0, &queue);
		return true;
	}

	bool createCommandPool(QString &error)
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamily;
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		{
			error = QStringLiteral("Could not create Vulkan command pool");
			return false;
		}
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		return vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) == VK_SUCCESS;
	}

	bool createSync(QString &error)
	{
		VkSemaphoreCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (vkCreateSemaphore(device, &info, nullptr, &imageAvailable) != VK_SUCCESS ||
		    vkCreateSemaphore(device, &info, nullptr, &renderFinished) != VK_SUCCESS)
		{
			error = QStringLiteral("Could not create Vulkan synchronization objects");
			return false;
		}
		return true;
	}

	bool memoryType(uint32_t typeBits, VkMemoryPropertyFlags flags, uint32_t &index)
	{
		VkPhysicalDeviceMemoryProperties properties{};
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
		for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
		{
			if ((typeBits & (1u << i)) &&
			    (properties.memoryTypes[i].propertyFlags & flags) == flags)
			{
				index = i;
				return true;
			}
		}
		return false;
	}

	bool recreateSwapchain(QString &error)
	{
		if (!device || size().width() <= 0 || size().height() <= 0)
			return false;
		vkDeviceWaitIdle(device);
		VkSurfaceCapabilitiesKHR caps{};
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps) != VK_SUCCESS ||
		    !(caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		{
			error = QStringLiteral("Vulkan surface does not support transfer destination images");
			return false;
		}
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
		if (formats.empty())
		{
			error = QStringLiteral("Vulkan surface has no supported formats");
			return false;
		}
		surfaceFormat = formats.front();
		for (const auto &candidate : formats)
		{
			if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM || candidate.format == VK_FORMAT_B8G8R8A8_SRGB)
			{
				surfaceFormat = candidate;
				break;
			}
		}
		if (textureImage && textureFormat != surfaceFormat.format)
			destroyTexture();
		textureFormat = surfaceFormat.format;
		if (caps.currentExtent.width != UINT32_MAX)
			swapchainExtent = caps.currentExtent;
		else
		{
			swapchainExtent.width = std::clamp((uint32_t)size().width(), caps.minImageExtent.width, caps.maxImageExtent.width);
			swapchainExtent.height = std::clamp((uint32_t)size().height(), caps.minImageExtent.height, caps.maxImageExtent.height);
		}
		uint32_t imageCount = caps.minImageCount + 1;
		if (caps.maxImageCount && imageCount > caps.maxImageCount)
			imageCount = caps.maxImageCount;
		uint32_t modeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, nullptr);
		std::vector<VkPresentModeKHR> modes(modeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, modes.data());
		VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
		if (!vsyncEnabled && std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != modes.end())
			mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		VkSwapchainCreateInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		info.surface = surface;
		info.minImageCount = imageCount;
		info.imageFormat = surfaceFormat.format;
		info.imageColorSpace = surfaceFormat.colorSpace;
		info.imageExtent = swapchainExtent;
		info.imageArrayLayers = 1;
		info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.preTransform = caps.currentTransform;
		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		if (!(caps.supportedCompositeAlpha & compositeAlpha))
		{
			const VkCompositeAlphaFlagBitsKHR options[] = {
				VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
				VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
				VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
			};
			for (VkCompositeAlphaFlagBitsKHR option : options)
			{
				if (caps.supportedCompositeAlpha & option)
				{
					compositeAlpha = option;
					break;
				}
			}
		}
		info.compositeAlpha = compositeAlpha;
		info.presentMode = mode;
		info.clipped = VK_TRUE;
		info.oldSwapchain = swapchain;
		VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
		if (vkCreateSwapchainKHR(device, &info, nullptr, &newSwapchain) != VK_SUCCESS)
		{
			error = QStringLiteral("Could not create a Vulkan swapchain");
			return false;
		}
		if (swapchain)
			vkDestroySwapchainKHR(device, swapchain, nullptr);
		swapchain = newSwapchain;
		vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
		swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
		swapchainLayouts.assign(imageCount, VK_IMAGE_LAYOUT_UNDEFINED);
		swapchainDirty = false;
		return true;
	}

	bool ensureTexture(int imageWidth, int imageHeight)
	{
		if (textureImage && textureWidth == imageWidth && textureHeight == imageHeight)
			return true;
		vkDeviceWaitIdle(device);
		destroyTexture();
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = textureFormat;
		imageInfo.extent = { (uint32_t)imageWidth, (uint32_t)imageHeight, 1 };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS)
			return false;
		VkMemoryRequirements requirements{};
		vkGetImageMemoryRequirements(device, textureImage, &requirements);
		uint32_t index = 0;
		if (!memoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index))
			return false;
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = requirements.size;
		allocInfo.memoryTypeIndex = index;
		if (vkAllocateMemory(device, &allocInfo, nullptr, &textureMemory) != VK_SUCCESS ||
		    vkBindImageMemory(device, textureImage, textureMemory, 0) != VK_SUCCESS)
			return false;
		textureWidth = imageWidth;
		textureHeight = imageHeight;
		textureInitialized = false;
		return true;
	}

	bool ensureStaging(int imageWidth, int imageHeight)
	{
		VkDeviceSize needed = (VkDeviceSize)imageWidth * imageHeight * sizeof(uint32_t);
		if (stagingBuffer && stagingSize >= needed)
			return true;
		destroyStaging();
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = needed;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
			return false;
		VkMemoryRequirements requirements{};
		vkGetBufferMemoryRequirements(device, stagingBuffer, &requirements);
		uint32_t index = 0;
		if (!memoryType(requirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, index))
			return false;
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = requirements.size;
		allocInfo.memoryTypeIndex = index;
		if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS ||
		    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0) != VK_SUCCESS)
			return false;
		stagingSize = needed;
		return true;
	}

	void destroyTexture()
	{
		if (textureImage) vkDestroyImage(device, textureImage, nullptr);
		if (textureMemory) vkFreeMemory(device, textureMemory, nullptr);
		textureImage = VK_NULL_HANDLE;
		textureMemory = VK_NULL_HANDLE;
		textureWidth = textureHeight = 0;
		textureInitialized = false;
	}

	void destroyStaging()
	{
		if (stagingBuffer) vkDestroyBuffer(device, stagingBuffer, nullptr);
		if (stagingMemory) vkFreeMemory(device, stagingMemory, nullptr);
		stagingBuffer = VK_NULL_HANDLE;
		stagingMemory = VK_NULL_HANDLE;
		stagingSize = 0;
	}

	void cleanup()
	{
		if (device)
		{
			vkDeviceWaitIdle(device);
			destroyTexture();
			destroyStaging();
			if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
			if (imageAvailable) vkDestroySemaphore(device, imageAvailable, nullptr);
			if (renderFinished) vkDestroySemaphore(device, renderFinished, nullptr);
			if (commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
			vkDestroyDevice(device, nullptr);
		}
		device = VK_NULL_HANDLE;
		if (instance.isValid())
		{
			QWindow::destroy();
			surface = VK_NULL_HANDLE;
			instance.destroy();
		}
		surface = VK_NULL_HANDLE;
	}

	QVulkanInstance instance;
	bool instanceReady = false;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
	uint32_t queueFamily = 0;
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkSemaphore imageAvailable = VK_NULL_HANDLE;
	VkSemaphore renderFinished = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSurfaceFormatKHR surfaceFormat{};
	VkExtent2D swapchainExtent{};
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageLayout> swapchainLayouts;
	VkFormat textureFormat = VK_FORMAT_UNDEFINED;
	VkImage textureImage = VK_NULL_HANDLE;
	VkDeviceMemory textureMemory = VK_NULL_HANDLE;
	int textureWidth = 0;
	int textureHeight = 0;
	bool textureInitialized = false;
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
	VkDeviceSize stagingSize = 0;
	bool swapchainDirty = true;
	bool vsyncEnabled = true;
	bool linearFilter = false;
	unsigned int mouseButtonMask = 0;
	QColor background = Qt::black;
	QString lastError;
	std::function<void()> exposedCallback;
};

ConsoleViewVulkan_t::ConsoleViewVulkan_t(QWidget *parent)
	: QWidget(parent), vulkanWindow(new ConsoleVulkanWindow_t), windowContainer(nullptr),
	  bgColor(nullptr), localBuf(nullptr), localBufSize(0), aspectRatio(1.0),
	  aspectX(1.0), aspectY(1.0), xscale(2.0), yscale(2.0), sx(0), sy(0), rw(256), rh(224),
	  forceAspect(true), autoScaleEna(true), linearFilter(false), vsyncEnabled(true), mouseButtonMask(0), drawQueued(false)
{
	consoleWin_t *win = qobject_cast<consoleWin_t *>(parent);
	if (win)
	{
		bgColor = win->getVideoBgColorPtr();
		bgColor->setRgb(0, 0, 0);
	}
	QWidget::setMinimumSize(256, 224);
	setFocusPolicy(Qt::StrongFocus);
	localBufSize = (4 * GL_NES_WIDTH) * (4 * GL_NES_HEIGHT) * sizeof(uint32_t);
	localBuf = (uint32_t *)malloc(localBufSize);
	if (localBuf)
		memset32(localBuf, alphaMask, localBufSize);
	if (g_config)
	{
		int option = 0;
		g_config->getOption("SDL.OpenGLip", &option);
		linearFilter = option != 0;
		g_config->getOption("SDL.AutoScale", &option);
		autoScaleEna = option != 0;
		g_config->getOption("SDL.XScale", &xscale);
		g_config->getOption("SDL.YScale", &yscale);
		g_config->getOption("SDL.ForceAspect", &forceAspect);
		g_config->getOption("SDL.VideoVsync", &vsyncEnabled);
		if (bgColor)
			fceuLoadConfigColor("SDL.VideoBgColor", bgColor);
	}
	windowContainer = QWidget::createWindowContainer(vulkanWindow, this);
	vulkanWindow->setExposedCallback([this]() { queueRedraw(); });
	windowContainer->setFocusPolicy(Qt::StrongFocus);
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(windowContainer);
}

ConsoleViewVulkan_t::~ConsoleViewVulkan_t()
{
	shutdown();
	if (localBuf)
		free(localBuf);
}

void ConsoleViewVulkan_t::shutdown(void)
{
	if (vulkanWindow)
		vulkanWindow->shutdown();
}

int ConsoleViewVulkan_t::init(void)
{
	QString error;
	if (!vulkanWindow->initialize(error))
	{
		fprintf(stderr, "FCEUX: Vulkan renderer initialization failed: %s\n",
			error.toLocal8Bit().constData());
		return -1;
	}
	return 0;
}

void ConsoleViewVulkan_t::reset(void)
{
	queueRedraw();
}

void ConsoleViewVulkan_t::queueRedraw(void)
{
	if (drawQueued.exchange(true))
		return;
	QTimer::singleShot(0, this, [this]() {
		drawQueued.store(false);
		if (!vulkanWindow || !localBuf)
			return;
		int frameWidth = GL_NES_WIDTH;
		int frameHeight = GL_NES_HEIGHT;
		if (nes_shm && nes_shm->video.ncol > 0 && nes_shm->video.nrow > 0)
		{
			frameWidth = nes_shm->video.ncol;
			frameHeight = nes_shm->video.nrow;
		}
		if (bgColor)
			vulkanWindow->setBackground(*bgColor);
		vulkanWindow->setLinearFilter(linearFilter);
		vulkanWindow->render(localBuf, frameWidth, frameHeight,
			bgColor ? *bgColor : QColor(Qt::black), forceAspect, aspectX, aspectY, linearFilter);
	});
}

void ConsoleViewVulkan_t::transfer2LocalBuffer(void)
{
	if (!localBuf || !nes_shm)
		return;
	int bufferIndex = nes_shm->pixBufIdx - 1;
	if (bufferIndex < 0)
		bufferIndex = NES_VIDEO_BUFLEN - 1;
	unsigned int copySize = nes_shm->video.ncol * nes_shm->video.nrow * 4;
	copySize = std::min(copySize, localBufSize);
	uint8_t *source = (uint8_t *)nes_shm->pixbuf[bufferIndex];
	if ((nes_shm->video.preScaler == 1) || (nes_shm->video.preScaler == 4))
	{
		for (unsigned int i = 0; i < copySize / 4; ++i)
		{
			localBuf[i] = (source[0]) | (source[1] << 8) | (source[2] << 16) | 0xff000000;
			source += 4;
		}
	}
	else
		copyPixels32(localBuf, source, copySize, alphaMask);
}

void ConsoleViewVulkan_t::setVsyncEnable(bool enabled)
{
	vsyncEnabled = enabled;
	if (vulkanWindow)
		vulkanWindow->setVsync(enabled);
}

void ConsoleViewVulkan_t::setLinearFilterEnable(bool enabled)
{
	linearFilter = enabled;
	if (vulkanWindow)
		vulkanWindow->setLinearFilter(enabled);
}

void ConsoleViewVulkan_t::setScaleXY(double xs, double ys)
{
	xscale = xs;
	yscale = ys;
	if (forceAspect)
		xscale = yscale = std::min(xscale, yscale);
}

void ConsoleViewVulkan_t::setAspectXY(double x, double y)
{
	aspectX = x;
	aspectY = y;
	aspectRatio = y / x;
}

void ConsoleViewVulkan_t::getAspectXY(double &x, double &y)
{
	x = aspectX;
	y = aspectY;
}

void ConsoleViewVulkan_t::setCursor(const QCursor &cursor)
{
	QWidget::setCursor(cursor);
	if (windowContainer)
		windowContainer->setCursor(cursor);
}

void ConsoleViewVulkan_t::setCursor(Qt::CursorShape shape)
{
	QWidget::setCursor(shape);
	if (windowContainer)
		windowContainer->setCursor(shape);
}

void ConsoleViewVulkan_t::setBgColor(QColor &color)
{
	if (bgColor)
		*bgColor = color;
	if (vulkanWindow)
		vulkanWindow->setBackground(color);
}

void ConsoleViewVulkan_t::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	gui_draw_area_width = event->size().width();
	gui_draw_area_height = event->size().height();
}

void ConsoleViewVulkan_t::calculateViewport(int frameWidth, int frameHeight, int &x, int &y, int &w, int &h)
{
	w = width();
	h = height();
	if (frameWidth <= 0 || frameHeight <= 0)
		return;
	if (forceAspect && aspectX > 0.0 && aspectY > 0.0)
	{
		double ratio = aspectX / aspectY;
		if ((double)w / h > ratio)
			w = (int)(h * ratio);
		else
			h = (int)(w / ratio);
	}
	x = (width() - w) / 2;
	y = (height() - h) / 2;
	sx = x;
	sy = y;
	rw = w;
	rh = h;
}

void ConsoleViewVulkan_t::getNormalizedCursorPos(double &x, double &y)
{
	QPoint pos = mapFromGlobal(QCursor::pos());
	int frameWidth = nes_shm ? nes_shm->video.ncol : 256;
	int frameHeight = nes_shm ? nes_shm->video.nrow : 224;
	int viewX, viewY, viewWidth, viewHeight;
	calculateViewport(frameWidth, frameHeight, viewX, viewY, viewWidth, viewHeight);
	x = std::clamp((double)(pos.x() - viewX) / std::max(1, viewWidth), 0.0, 1.0);
	y = std::clamp((double)(pos.y() - viewY) / std::max(1, viewHeight), 0.0, 1.0);
}

bool ConsoleViewVulkan_t::getMouseButtonState(unsigned int button)
{
	mouseButtonMask = vulkanWindow ? vulkanWindow->mouseButtons() : mouseButtonMask;
	return (mouseButtonMask & button) != 0;
}
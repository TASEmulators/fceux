#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <vector>

#include <vulkan/vulkan.h>

#include <QExposeEvent>
#include <QMouseEvent>
#include <QPlatformSurfaceEvent>
#include <QVBoxLayout>
#include <QVulkanDeviceFunctions>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <QWindow>

#include "Qt/ConsoleViewerVulkan.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/fceuWrapper.h"
#include "Qt/nes_shm.h"
#include "Qt/throttle.h"

extern unsigned int gui_draw_area_width;
extern unsigned int gui_draw_area_height;

// Largest frame the emulator can produce (see localBuf allocation)
static const uint32_t MAX_FRAME_WIDTH = 4 * GL_NES_WIDTH;
static const uint32_t MAX_FRAME_HEIGHT = 4 * GL_NES_HEIGHT;

// Shared by every viewer instance and kept for the lifetime of the program,
// so windows can be created and destroyed without recreating the instance.
static QVulkanInstance *getVulkanInstance()
{
	static QVulkanInstance instance;
	static bool created = instance.create();
	return created ? &instance : nullptr;
}

// Owns a swapchain created with TRANSFER_DST usage so that the emulator
// frame can be blitted straight into the presentable image. No pipelines,
// render passes or shaders are needed for a 2D surface.
class ConsoleVulkanWindow_t : public QWindow
{
public:
	ConsoleVulkanWindow_t(QVulkanInstance *inst) : vkInst(inst)
	{
		setSurfaceType(QSurface::VulkanSurface);
		setVulkanInstance(inst);
		resize(256, 224);
	}

	~ConsoleVulkanWindow_t() override { cleanup(); }

	// Frame description set by the view before each render()
	const uint32_t *pixels = nullptr;
	int frameWidth = 0;
	int frameHeight = 0;
	QColor background = Qt::black;
	bool forceAspect = true;
	double aspectX = 1.0;
	double aspectY = 1.0;
	bool linearFilter = false;
	bool vsync = true;
	unsigned int mouseButtonMask = 0;
	std::function<void()> exposedCallback;

	bool initialize(QString &error)
	{
		create();
		vkFuncs = vkInst->functions();
		surface = vkInst->surfaceForWindow(this);
		if (surface == VK_NULL_HANDLE || !createDevice(error))
		{
			if (error.isEmpty())
				error = QStringLiteral("Could not create a Vulkan surface");
			cleanup();
			return false;
		}
		return true;
	}

	void render()
	{
		if (!dev || !isExposed() || !pixels || frameWidth <= 0 || frameHeight <= 0 ||
		    frameWidth > (int)MAX_FRAME_WIDTH || frameHeight > (int)MAX_FRAME_HEIGHT)
			return;
		if (swapchainDirty && !createSwapchain())
			return;

		uint32_t index = 0;
		VkResult res = vkAcquireNextImageKHR(dev, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &index);
		if (res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			swapchainDirty = true;
			return;
		}
		if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
			return;

		memcpy(stagingMapped, pixels, (size_t)frameWidth * frameHeight * sizeof(uint32_t));

		VkImage target = images[index];
		VkCommandBuffer cb = cmdBuf;
		df->vkResetCommandBuffer(cb, 0);
		VkCommandBufferBeginInfo begin{};
		begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		df->vkBeginCommandBuffer(cb, &begin);

		layout(cb, texture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VkBufferImageCopy copy{};
		copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copy.imageExtent = { (uint32_t)frameWidth, (uint32_t)frameHeight, 1 };
		df->vkCmdCopyBufferToImage(cb, stagingBuf, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
		layout(cb, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		layout(cb, target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VkClearColorValue clear = { { (float)background.redF(), (float)background.greenF(),
		                              (float)background.blueF(), 1.0f } };
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		df->vkCmdClearColorImage(cb, target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);

		int x = 0, y = 0, w = (int)extent.width, h = (int)extent.height;
		if (forceAspect && aspectX > 0.0 && aspectY > 0.0)
		{
			double ratio = aspectX / aspectY;
			if ((double)w / h > ratio)
				w = (int)(h * ratio);
			else
				h = (int)(w / ratio);
			x = ((int)extent.width - w) / 2;
			y = ((int)extent.height - h) / 2;
		}
		VkImageBlit blit{};
		blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		blit.srcOffsets[1] = { frameWidth, frameHeight, 1 };
		blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		blit.dstOffsets[0] = { x, y, 0 };
		blit.dstOffsets[1] = { x + w, y + h, 1 };
		df->vkCmdBlitImage(cb, texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
			linearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST);
		layout(cb, target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		df->vkEndCommandBuffer(cb);

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &imageAvailable;
		submit.pWaitDstStageMask = &waitStage;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cb;
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &renderFinished;
		if (df->vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS)
			return;

		VkPresentInfoKHR present{};
		present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present.waitSemaphoreCount = 1;
		present.pWaitSemaphores = &renderFinished;
		present.swapchainCount = 1;
		present.pSwapchains = &swapchain;
		present.pImageIndices = &index;
		res = vkQueuePresentKHR(queue, &present);
		df->vkQueueWaitIdle(queue);

		if (res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR)
		{
			videoBufferSwapMark();
			if (nes_shm)
				nes_shm->render_count++;
		}
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
			swapchainDirty = true;
	}

	void setVsync(bool enabled)
	{
		if (vsync != enabled)
		{
			vsync = enabled;
			swapchainDirty = true;
		}
	}

protected:
	void exposeEvent(QExposeEvent *event) override
	{
		QWindow::exposeEvent(event);
		swapchainDirty = true;
		if (isExposed() && exposedCallback)
			exposedCallback();
	}

	void resizeEvent(QResizeEvent *event) override
	{
		QWindow::resizeEvent(event);
		swapchainDirty = true;
	}

	bool event(QEvent *e) override
	{
		if (e->type() == QEvent::PlatformSurface &&
		    static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() ==
		        QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
			destroySwapchain();
		return QWindow::event(e);
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
	void layout(VkCommandBuffer cb, VkImage image, VkImageLayout from, VkImageLayout to)
	{
		VkImageMemoryBarrier b{};
		b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		b.srcAccessMask = (to == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL || to == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			? VK_ACCESS_TRANSFER_WRITE_BIT : 0;
		b.dstAccessMask = to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ? VK_ACCESS_TRANSFER_WRITE_BIT
			: to == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ? VK_ACCESS_TRANSFER_READ_BIT : 0;
		b.oldLayout = from;
		b.newLayout = to;
		b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.image = image;
		b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		df->vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0, 0, nullptr, 0, nullptr, 1, &b);
	}

	uint32_t memoryType(uint32_t allowed, VkMemoryPropertyFlags props) const
	{
		VkPhysicalDeviceMemoryProperties mp;
		vkFuncs->vkGetPhysicalDeviceMemoryProperties(phys, &mp);
		for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
			if ((allowed & (1u << i)) && (mp.memoryTypes[i].propertyFlags & props) == props)
				return i;
		return UINT32_MAX;
	}

	bool createDevice(QString &error)
	{
		uint32_t count = 0;
		vkFuncs->vkEnumeratePhysicalDevices(vkInst->vkInstance(), &count, nullptr);
		std::vector<VkPhysicalDevice> devices(count);
		vkFuncs->vkEnumeratePhysicalDevices(vkInst->vkInstance(), &count, devices.data());
		for (VkPhysicalDevice cand : devices)
		{
			uint32_t n = 0;
			vkFuncs->vkGetPhysicalDeviceQueueFamilyProperties(cand, &n, nullptr);
			std::vector<VkQueueFamilyProperties> fams(n);
			vkFuncs->vkGetPhysicalDeviceQueueFamilyProperties(cand, &n, fams.data());
			for (uint32_t i = 0; i < n; i++)
			{
				VkBool32 present = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(cand, i, surface, &present);
				if (present && (fams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				{
					phys = cand;
					family = i;
					break;
				}
			}
			if (phys)
				break;
		}
		if (!phys)
		{
			error = QStringLiteral("No Vulkan device can present to this window");
			return false;
		}

		float prio = 1.0f;
		VkDeviceQueueCreateInfo q{};
		q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		q.queueFamilyIndex = family;
		q.queueCount = 1;
		q.pQueuePriorities = &prio;
		const char *ext = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		VkDeviceCreateInfo di{};
		di.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		di.queueCreateInfoCount = 1;
		di.pQueueCreateInfos = &q;
		di.enabledExtensionCount = 1;
		di.ppEnabledExtensionNames = &ext;
		if (vkFuncs->vkCreateDevice(phys, &di, nullptr, &dev) != VK_SUCCESS)
		{
			error = QStringLiteral("vkCreateDevice failed");
			return false;
		}
		df = vkInst->deviceFunctions(dev);
		df->vkGetDeviceQueue(dev, family, 0, &queue);

		VkCommandPoolCreateInfo pool{};
		pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool.queueFamilyIndex = family;
		VkCommandBufferAllocateInfo cba{};
		cba.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cba.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cba.commandBufferCount = 1;
		VkSemaphoreCreateInfo sem{};
		sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (df->vkCreateCommandPool(dev, &pool, nullptr, &cmdPool) != VK_SUCCESS ||
		    (cba.commandPool = cmdPool, df->vkAllocateCommandBuffers(dev, &cba, &cmdBuf) != VK_SUCCESS) ||
		    df->vkCreateSemaphore(dev, &sem, nullptr, &imageAvailable) != VK_SUCCESS ||
		    df->vkCreateSemaphore(dev, &sem, nullptr, &renderFinished) != VK_SUCCESS)
		{
			error = QStringLiteral("Could not create Vulkan command resources");
			return false;
		}
		return createFrameResources(error);
	}

	// Texture and staging buffer are sized for the largest frame, so they are created once
	bool createFrameResources(QString &error)
	{
		VkImageCreateInfo ii{};
		ii.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ii.imageType = VK_IMAGE_TYPE_2D;
		ii.format = VK_FORMAT_B8G8R8A8_UNORM;
		ii.extent = { MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, 1 };
		ii.mipLevels = ii.arrayLayers = 1;
		ii.samples = VK_SAMPLE_COUNT_1_BIT;
		ii.tiling = VK_IMAGE_TILING_OPTIMAL;
		ii.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkBufferCreateInfo bi{};
		bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bi.size = (VkDeviceSize)MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT * sizeof(uint32_t);
		bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkMemoryRequirements req;
		VkMemoryAllocateInfo alloc{};
		alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		bool ok = df->vkCreateImage(dev, &ii, nullptr, &texture) == VK_SUCCESS;
		if (ok)
		{
			df->vkGetImageMemoryRequirements(dev, texture, &req);
			alloc.allocationSize = req.size;
			alloc.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			ok = alloc.memoryTypeIndex != UINT32_MAX &&
				df->vkAllocateMemory(dev, &alloc, nullptr, &textureMem) == VK_SUCCESS &&
				df->vkBindImageMemory(dev, texture, textureMem, 0) == VK_SUCCESS;
		}
		if (ok)
			ok = df->vkCreateBuffer(dev, &bi, nullptr, &stagingBuf) == VK_SUCCESS;
		if (ok)
		{
			df->vkGetBufferMemoryRequirements(dev, stagingBuf, &req);
			alloc.allocationSize = req.size;
			alloc.memoryTypeIndex = memoryType(req.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			ok = alloc.memoryTypeIndex != UINT32_MAX &&
				df->vkAllocateMemory(dev, &alloc, nullptr, &stagingMem) == VK_SUCCESS &&
				df->vkBindBufferMemory(dev, stagingBuf, stagingMem, 0) == VK_SUCCESS &&
				df->vkMapMemory(dev, stagingMem, 0, VK_WHOLE_SIZE, 0, (void **)&stagingMapped) == VK_SUCCESS;
		}
		if (!ok)
			error = QStringLiteral("Could not allocate Vulkan frame resources");
		return ok;
	}

	bool createSwapchain()
	{
		df->vkDeviceWaitIdle(dev);
		// Qt may have recreated the native window, so never reuse an old surface
		surface = vkInst->surfaceForWindow(this);
		if (surface == VK_NULL_HANDLE)
			return false;
		VkSurfaceCapabilitiesKHR caps;
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps) != VK_SUCCESS ||
		    !(caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
			return false;
		extent = caps.currentExtent;
		if (extent.width == UINT32_MAX)
		{
			QSize s = size() * devicePixelRatio();
			extent = { (uint32_t)s.width(), (uint32_t)s.height() };
		}
		if (!extent.width || !extent.height)
			return false;

		uint32_t n = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &n, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(n);
		vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &n, formats.data());
		VkSurfaceFormatKHR fmt{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		for (const VkSurfaceFormatKHR &f : formats)
			if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				fmt = f;
		if (fmt.format == VK_FORMAT_UNDEFINED)
			return false;

		VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
		if (!vsync)
		{
			vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &n, nullptr);
			std::vector<VkPresentModeKHR> modes(n);
			vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &n, modes.data());
			for (VkPresentModeKHR m : modes)
				if (m == VK_PRESENT_MODE_IMMEDIATE_KHR)
					mode = m;
		}

		uint32_t count = caps.minImageCount + 1;
		if (caps.maxImageCount && count > caps.maxImageCount)
			count = caps.maxImageCount;
		VkSwapchainCreateInfoKHR sc{};
		sc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		sc.surface = surface;
		sc.minImageCount = count;
		sc.imageFormat = fmt.format;
		sc.imageColorSpace = fmt.colorSpace;
		sc.imageExtent = extent;
		sc.imageArrayLayers = 1;
		sc.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		sc.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		sc.preTransform = caps.currentTransform;
		sc.compositeAlpha = (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
			? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
		sc.presentMode = mode;
		sc.clipped = VK_TRUE;
		if (swapchain)
			vkDestroySwapchainKHR(dev, swapchain, nullptr);
		swapchain = VK_NULL_HANDLE;
		VkResult res = vkCreateSwapchainKHR(dev, &sc, nullptr, &swapchain);
		if (res != VK_SUCCESS)
			return false;

		vkGetSwapchainImagesKHR(dev, swapchain, &n, nullptr);
		images.resize(n);
		vkGetSwapchainImagesKHR(dev, swapchain, &n, images.data());
		swapchainDirty = false;
		return true;
	}

	// Must run before the surface it was created from goes away
	void destroySwapchain()
	{
		if (dev && swapchain)
		{
			df->vkDeviceWaitIdle(dev);
			vkDestroySwapchainKHR(dev, swapchain, nullptr);
		}
		swapchain = VK_NULL_HANDLE;
		images.clear();
		surface = VK_NULL_HANDLE;
		swapchainDirty = true;
	}

	void cleanup()
	{
		if (!dev)
			return;
		destroySwapchain();
		if (stagingMapped)
			df->vkUnmapMemory(dev, stagingMem);
		df->vkDestroyBuffer(dev, stagingBuf, nullptr);
		df->vkFreeMemory(dev, stagingMem, nullptr);
		df->vkDestroyImage(dev, texture, nullptr);
		df->vkFreeMemory(dev, textureMem, nullptr);
		df->vkDestroySemaphore(dev, imageAvailable, nullptr);
		df->vkDestroySemaphore(dev, renderFinished, nullptr);
		df->vkDestroyCommandPool(dev, cmdPool, nullptr);
		df->vkDestroyDevice(dev, nullptr);
		stagingMapped = nullptr;
		dev = VK_NULL_HANDLE;
	}

	QVulkanInstance *vkInst;
	QVulkanFunctions *vkFuncs = nullptr;
	QVulkanDeviceFunctions *df = nullptr;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkPhysicalDevice phys = VK_NULL_HANDLE;
	uint32_t family = 0;
	VkDevice dev = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
	VkCommandPool cmdPool = VK_NULL_HANDLE;
	VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
	VkSemaphore imageAvailable = VK_NULL_HANDLE;
	VkSemaphore renderFinished = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkExtent2D extent{};
	std::vector<VkImage> images;
	VkImage texture = VK_NULL_HANDLE;
	VkDeviceMemory textureMem = VK_NULL_HANDLE;
	VkBuffer stagingBuf = VK_NULL_HANDLE;
	VkDeviceMemory stagingMem = VK_NULL_HANDLE;
	uint32_t *stagingMapped = nullptr;
	bool swapchainDirty = true;
};

ConsoleViewVulkan_t::ConsoleViewVulkan_t(QWidget *parent)
	: QWidget(parent), vulkanWindow(nullptr), windowContainer(nullptr),
	  bgColor(nullptr), localBuf(nullptr), localBufSize(0), aspectRatio(1.0),
	  aspectX(1.0), aspectY(1.0), xscale(2.0), yscale(2.0), sx(0), sy(0), rw(256), rh(224),
	  forceAspect(true), autoScaleEna(true), linearFilter(false), vsyncEnabled(true), mouseButtonMask(0),
	  shuttingDown(false)
{
	consoleWin_t *win = qobject_cast<consoleWin_t *>(parent);
	if (win)
	{
		bgColor = win->getVideoBgColorPtr();
		bgColor->setRgb(0, 0, 0);
	}
	QWidget::setMinimumSize(256, 224);
	setFocusPolicy(Qt::StrongFocus);
	localBufSize = MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT * sizeof(uint32_t);
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
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	if (QVulkanInstance *instance = getVulkanInstance())
	{
		vulkanWindow = new ConsoleVulkanWindow_t(instance);
		vulkanWindow->vsync = vsyncEnabled;
		vulkanWindow->exposedCallback = [this]() { queueRedraw(); };
		windowContainer = QWidget::createWindowContainer(vulkanWindow, this);
		windowContainer->setFocusPolicy(Qt::StrongFocus);
		layout->addWidget(windowContainer);
	}
}

ConsoleViewVulkan_t::~ConsoleViewVulkan_t()
{
	shutdown();
	if (windowContainer)
	{
		windowContainer->deleteLater();
	}
	windowContainer = nullptr;
	vulkanWindow = nullptr;
	if (localBuf)
		free(localBuf);
}

void ConsoleViewVulkan_t::shutdown(void)
{
	shuttingDown = true;
	// Late expose events must not call back into this view or read localBuf
	if (vulkanWindow)
	{
		vulkanWindow->exposedCallback = nullptr;
		vulkanWindow->pixels = nullptr;
	}
}

int ConsoleViewVulkan_t::init(void)
{
	QString error;
	if (!vulkanWindow)
		error = QStringLiteral("could not create a Vulkan instance");
	else if (vulkanWindow->initialize(error))
		return 0;
	fprintf(stderr, "FCEUX: Vulkan renderer initialization failed: %s\n", error.toLocal8Bit().constData());
	return -1;
}

void ConsoleViewVulkan_t::reset(void)
{
	queueRedraw();
}

void ConsoleViewVulkan_t::queueRedraw(void)
{
	if (shuttingDown || !vulkanWindow || !localBuf)
		return;
	int frameWidth = GL_NES_WIDTH;
	int frameHeight = GL_NES_HEIGHT;
	if (nes_shm && nes_shm->video.ncol > 0 && nes_shm->video.nrow > 0)
	{
		frameWidth = nes_shm->video.ncol;
		frameHeight = nes_shm->video.nrow;
	}
	vulkanWindow->pixels = localBuf;
	vulkanWindow->frameWidth = frameWidth;
	vulkanWindow->frameHeight = frameHeight;
	vulkanWindow->background = bgColor ? *bgColor : QColor(Qt::black);
	vulkanWindow->forceAspect = forceAspect;
	vulkanWindow->aspectX = aspectX;
	vulkanWindow->aspectY = aspectY;
	vulkanWindow->linearFilter = linearFilter;
	vulkanWindow->render();
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

QImage ConsoleViewVulkan_t::getFrameImage(void)
{
	if (!nes_shm)
		return QImage();
	return copyLocalBufferToImage(localBuf, localBufSize,
		nes_shm->video.ncol, nes_shm->video.nrow);
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
	mouseButtonMask = vulkanWindow ? vulkanWindow->mouseButtonMask : mouseButtonMask;
	return (mouseButtonMask & button) != 0;
}
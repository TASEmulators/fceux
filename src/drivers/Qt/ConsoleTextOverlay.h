// ConsoleTextOverlay.h
//
// Shared text overlay for all of the Qt video drivers.
//
// Text is kept as persistent items that any thread can create and update. A video
// driver hands this object the size of its drawing surface (in device pixels) and
// the rectangle the emulator image occupies in it. The overlay lays the items out,
// rasterizes them with a built-in 8x8 bitmap font into a window sized ARGB layer,
// and returns the layer. The driver then composites the layer over the video as
// the very last step of presenting a frame. The layer never touches the video
// buffer, so screenshots and AVI recording are not affected.
//
// Glyph pixels are fully opaque or fully transparent, there is no antialiasing.
// That lets a driver that cannot alpha blend (Vulkan transfer commands) draw the
// text as a list of solid rectangles.

#pragma once

#include <stdint.h>

#include <functional>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

#include <QColor>
#include <QRect>
#include <QSize>
#include <QString>

class ConsoleTextOverlay
{
public:
	typedef int Handle;
	static constexpr Handle INVALID_HANDLE = -1;

	// Where an item is allowed to appear. The bars are the unused space around the
	// image when the window aspect ratio does not match the video.
	enum Region
	{
		REGION_TOP_BAR = 0,
		REGION_BOTTOM_BAR,
		REGION_LEFT_BAR,
		REGION_RIGHT_BAR,
		REGION_IMAGE
	};

	// Anchor point inside the chosen region
	enum Anchor
	{
		ANCHOR_TOP_LEFT = 0,
		ANCHOR_TOP_CENTER,
		ANCHOR_TOP_RIGHT,
		ANCHOR_CENTER_LEFT,
		ANCHOR_CENTER,
		ANCHOR_CENTER_RIGHT,
		ANCHOR_BOTTOM_LEFT,
		ANCHOR_BOTTOM_CENTER,
		ANCHOR_BOTTOM_RIGHT
	};

	// Integer scale of the 8x8 font. SCALE_AUTO picks from the surface height and
	// device pixel ratio.
	static constexpr int SCALE_AUTO = 0;

	static constexpr int GLYPH_SIZE = 8;

	static ConsoleTextOverlay *instance(void);

	// ---- Item API, safe to call from any thread ----

	Handle createItem(void);
	void   destroyItem(Handle h);

	// Characters outside printable ASCII are drawn as '?'. One line per item.
	void setText(Handle h, const QString &text);

	// Only the RGB part is used, glyph pixels are always opaque
	void setColor(Handle h, const QColor &color);

	void setScale(Handle h, int scale);
	void setVisible(Handle h, bool visible);

	// Draws a solid black border one glyph pixel wide around the glyphs so the text stays
	// readable over the video. On by default. The border is part of the item's size.
	void setOutline(Handle h, bool outline);

	// The regions are tried in order and the first one the text fits in is used. When
	// none fit, the item falls back to REGION_IMAGE. Offsets are margins from the
	// anchored edges in glyph pixels (a glyph pixel is scale device pixels), and a
	// signed shift (+x right, +y down) for centered anchors. Items that end up with
	// the same region and anchor are stacked in creation order.
	void setPlacement(Handle h, const std::vector<Region> &regions, Anchor anchor,
		int offsetX = 2, int offsetY = 2);

	// Called, possibly from any thread, when the overlay needs to be drawn again.
	// Once per change, further calls are held back until the next render().
	void setChangedCallback(std::function<void(void)> cb);

	// ---- Driver API, GUI thread only ----

	struct Layer
	{
		int         width  = 0;      // device pixels
		int         height = 0;
		double      dpr    = 1.0;
		const uint32_t *pixels = nullptr;  // width * height ARGB32, rows are width pixels long
		uint64_t    revision = 0;
		// True when this render() call laid the items out again. Only the pixels inside
		// textRects are ever drawn, so a driver that keeps the layer in a texture only has
		// to upload those rectangles when this is set, and after it recreates the texture.
		bool        changed = false;
		bool        empty = true;    // nothing is currently visible

		// Where the text is, so a driver only has to draw those areas
		std::vector<QRect> textRects;

		// Opaque rectangles that together cover every pixel of the text and its outline,
		// for drivers that cannot blend. Each one is a single solid color, taken from the
		// pixels, and none of them overlap.
		std::vector<QRect> solidRects;
	};

	// Lays out and rasterizes when items or geometry changed since the last call, which
	// is cheap to call every frame otherwise. surface is the size in device pixels and
	// image is the rectangle the emulator image occupies in it (top-left origin).
	const Layer &render(const QSize &surface, double dpr, const QRect &image);

	// Counts changes to the items. A driver that only repaints part of the window can
	// compare this with the revision of the layer it last drew to see if the text changed.
	uint64_t revision(void);

	// Makes the next render() lay out again and report it as changed, for use when a
	// driver loses its GPU resources.
	void invalidate(void);

private:
	ConsoleTextOverlay(void);

	struct Item
	{
		bool    used = false;
		bool    visible = true;
		QString text;
		QColor  color = Qt::white;
		int     scale = SCALE_AUTO;
		bool    outline = true;
		std::vector<Region> regions;
		Anchor  anchor = ANCHOR_TOP_LEFT;
		int     offsetX = 2;
		int     offsetY = 2;
	};

	void markChanged(std::unique_lock<std::mutex> &lock);
	int  autoScale(int surfaceHeight, double dpr) const;
	void fillRect(const QRect &r, uint32_t argb);

	std::mutex mutex;
	std::vector<Item> items;
	std::function<void(void)> changedCb;
	bool itemsDirty = true;
	bool redrawPending = false;
	bool forceFull = true;

	// Geometry used for the current layer
	QSize  lastSurface;
	double lastDpr = 0.0;
	QRect  lastImage;

	std::vector<uint32_t> pixels;
	Layer layer;
	uint64_t itemRevision = 0;
};

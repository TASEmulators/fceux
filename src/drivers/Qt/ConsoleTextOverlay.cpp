// ConsoleTextOverlay.cpp
//
#include <algorithm>
#include <cmath>
#include <string.h>

#include "../../profiler.h"

#include "Qt/ConsoleTextOverlay.h"
#include "Qt/TextOverlayFont.h"

//----------------------------------------------------------
ConsoleTextOverlay::ConsoleTextOverlay(void)
{
}
//----------------------------------------------------------
ConsoleTextOverlay *ConsoleTextOverlay::instance(void)
{
	static ConsoleTextOverlay overlay;
	return &overlay;
}
//----------------------------------------------------------
void ConsoleTextOverlay::markChanged(std::unique_lock<std::mutex> &lock)
{
	itemsDirty = true;
	itemRevision++;

	if (!redrawPending && changedCb)
	{
		redrawPending = true;

		// The callback runs without the lock so it is free to call back into the overlay
		std::function<void(void)> cb = changedCb;
		lock.unlock();
		cb();
	}
}
//----------------------------------------------------------
ConsoleTextOverlay::Handle ConsoleTextOverlay::createItem(void)
{
	std::unique_lock<std::mutex> lock(mutex);

	Item item;
	item.used = true;
	item.regions = { REGION_IMAGE };

	items.push_back(item);

	// Nothing is visible until the item gets text, so no redraw is needed
	return static_cast<Handle>(items.size() - 1);
}
//----------------------------------------------------------
void ConsoleTextOverlay::destroyItem(Handle h)
{
	std::unique_lock<std::mutex> lock(mutex);

	if ((h < 0) || (h >= static_cast<Handle>(items.size())) || !items[h].used)
	{
		return;
	}
	items[h] = Item();
	markChanged(lock);
}
//----------------------------------------------------------
void ConsoleTextOverlay::setText(Handle h, const QString &text)
{
	std::unique_lock<std::mutex> lock(mutex);

	if ((h < 0) || (h >= static_cast<Handle>(items.size())) || !items[h].used || (items[h].text == text))
	{
		return;
	}
	items[h].text = text;
	markChanged(lock);
}
//----------------------------------------------------------
void ConsoleTextOverlay::setColor(Handle h, const QColor &color)
{
	std::unique_lock<std::mutex> lock(mutex);

	if ((h < 0) || (h >= static_cast<Handle>(items.size())) || !items[h].used || (items[h].color.rgb() == color.rgb()))
	{
		return;
	}
	items[h].color = color;
	markChanged(lock);
}
//----------------------------------------------------------
void ConsoleTextOverlay::setScale(Handle h, int scale)
{
	std::unique_lock<std::mutex> lock(mutex);

	scale = std::max(scale, 0);

	if ((h < 0) || (h >= static_cast<Handle>(items.size())) || !items[h].used || (items[h].scale == scale))
	{
		return;
	}
	items[h].scale = scale;
	markChanged(lock);
}
//----------------------------------------------------------
void ConsoleTextOverlay::setVisible(Handle h, bool visible)
{
	std::unique_lock<std::mutex> lock(mutex);

	if ((h < 0) || (h >= static_cast<Handle>(items.size())) || !items[h].used || (items[h].visible == visible))
	{
		return;
	}
	items[h].visible = visible;
	markChanged(lock);
}
//----------------------------------------------------------
void ConsoleTextOverlay::setOutline(Handle h, bool outline)
{
	std::unique_lock<std::mutex> lock(mutex);

	if ((h < 0) || (h >= static_cast<Handle>(items.size())) || !items[h].used || (items[h].outline == outline))
	{
		return;
	}
	items[h].outline = outline;
	markChanged(lock);
}
//----------------------------------------------------------
void ConsoleTextOverlay::setPlacement(Handle h, const std::vector<Region> &regions, Anchor anchor,
	int offsetX, int offsetY)
{
	std::unique_lock<std::mutex> lock(mutex);

	if ((h < 0) || (h >= static_cast<Handle>(items.size())) || !items[h].used)
	{
		return;
	}
	Item &item = items[h];

	if ((item.regions == regions) && (item.anchor == anchor) && (item.offsetX == offsetX) && (item.offsetY == offsetY))
	{
		return;
	}
	item.regions = regions;
	item.anchor  = anchor;
	item.offsetX = offsetX;
	item.offsetY = offsetY;
	markChanged(lock);
}
//----------------------------------------------------------
void ConsoleTextOverlay::setChangedCallback(std::function<void(void)> cb)
{
	std::unique_lock<std::mutex> lock(mutex);
	changedCb = cb;
}
//----------------------------------------------------------
uint64_t ConsoleTextOverlay::revision(void)
{
	std::unique_lock<std::mutex> lock(mutex);
	return itemRevision;
}
//----------------------------------------------------------
void ConsoleTextOverlay::invalidate(void)
{
	std::unique_lock<std::mutex> lock(mutex);
	forceFull = true;
}
//----------------------------------------------------------
int ConsoleTextOverlay::autoScale(int surfaceHeight, double dpr) const
{
	// A 480 line surface gets 1x text, and the scale grows from there. A high dpi
	// display never gets less than its own pixel ratio.
	const int byHeight = static_cast<int>(std::lround(surfaceHeight / 480.0));
	const int byDpr    = static_cast<int>(std::lround(dpr));

	return std::max(1, std::max(byHeight, byDpr));
}
//----------------------------------------------------------
void ConsoleTextOverlay::fillRect(const QRect &r, uint32_t argb)
{
	const QRect c = r.intersected(QRect(0, 0, layer.width, layer.height));

	for (int y = c.top(); y <= c.bottom(); y++)
	{
		uint32_t *row = &pixels[static_cast<size_t>(y) * layer.width];

		for (int x = c.left(); x <= c.right(); x++)
		{
			row[x] = argb;
		}
	}
}
//----------------------------------------------------------
static bool glyphPixelOn(const QString &text, int col, int row)
{
	int ch = text[col / ConsoleTextOverlay::GLYPH_SIZE].unicode();

	if ((ch < TEXT_OVERLAY_FONT_FIRST) || (ch > TEXT_OVERLAY_FONT_LAST))
	{
		ch = '?';
	}
	return (textOverlayFont8x8[ch - TEXT_OVERLAY_FONT_FIRST][row] >> (col % ConsoleTextOverlay::GLYPH_SIZE)) & 1;
}
//----------------------------------------------------------
// Converts a grid of set cells into rectangles that cover exactly those cells. Runs of cells
// in a row are merged horizontally, and then vertically when the same run repeats on the
// next row. Each cell is scale pixels square and origin is the top left of the grid.
static void buildSolidRects(const std::vector<uint8_t> &grid, int gridW, int gridH,
	const QPoint &origin, int scale, std::vector<QRect> &out)
{
	std::vector<QRect> open;   // rectangles that end on the previous row

	for (int row = 0; row < gridH; row++)
	{
		std::vector<QRect> next;
		int col = 0;

		while (col < gridW)
		{
			if (!grid[static_cast<size_t>(row) * gridW + col])
			{
				col++;
				continue;
			}
			const int start = col;

			while ((col < gridW) && grid[static_cast<size_t>(row) * gridW + col])
			{
				col++;
			}
			QRect r(origin.x() + start * scale, origin.y() + row * scale, (col - start) * scale, scale);
			bool merged = false;

			for (size_t i = 0; i < open.size(); i++)
			{
				if ((open[i].left() == r.left()) && (open[i].width() == r.width()))
				{
					open[i].setHeight(open[i].height() + scale);
					next.push_back(open[i]);
					open.erase(open.begin() + i);
					merged = true;
					break;
				}
			}
			if (!merged)
			{
				next.push_back(r);
			}
		}
		out.insert(out.end(), open.begin(), open.end());
		open.swap(next);
	}
	out.insert(out.end(), open.begin(), open.end());
}
//----------------------------------------------------------
const ConsoleTextOverlay::Layer &ConsoleTextOverlay::render(const QSize &surface, double dpr, const QRect &image)
{
	FCEU_PROFILE_FUNC("Text overlay render");

	std::unique_lock<std::mutex> lock(mutex);

	redrawPending = false;

	const QRect surfaceRect(QPoint(0, 0), surface);
	QRect img = image.intersected(surfaceRect);

	if (img.isEmpty())
	{
		img = surfaceRect;
	}
	const bool geometryChanged = (surface != lastSurface) || (dpr != lastDpr) || (img != lastImage);

	if (!itemsDirty && !geometryChanged && !forceFull)
	{
		layer.changed = false;
		return layer;
	}
	itemsDirty = false;

	// Only reached when the items or the geometry changed
	FCEU_PROFILE_FUNC("Text overlay layout and rasterize");

	const bool fullRedraw = forceFull || (surface != lastSurface);
	forceFull = false;

	if (fullRedraw)
	{
		const size_t count = static_cast<size_t>(std::max(surface.width(), 0)) * std::max(surface.height(), 0);
		pixels.assign(count, 0);
	}
	else
	{
		for (const QRect &old : layer.textRects)
		{
			fillRect(old, 0);
		}
	}
	lastSurface = surface;
	lastDpr = dpr;
	lastImage = img;

	layer.width  = surface.width();
	layer.height = surface.height();
	layer.dpr    = dpr;
	layer.pixels = pixels.empty() ? nullptr : pixels.data();
	layer.revision = itemRevision;
	layer.textRects.clear();
	layer.solidRects.clear();

	const int W = surface.width();
	const int H = surface.height();

	const QRect regionRects[5] =
	{
		QRect(0, 0, W, img.top()),                                              // REGION_TOP_BAR
		QRect(0, img.bottom() + 1, W, H - img.bottom() - 1),                    // REGION_BOTTOM_BAR
		QRect(0, img.top(), img.left(), img.height()),                          // REGION_LEFT_BAR
		QRect(img.right() + 1, img.top(), W - img.right() - 1, img.height()),   // REGION_RIGHT_BAR
		img                                                                     // REGION_IMAGE
	};
	const int defaultScale = autoScale(H, dpr);

	// Space already used by earlier items at the same region and anchor
	std::map<std::pair<int, int>, int> stackUsed;

	for (const Item &item : items)
	{
		if (!item.used || !item.visible || item.text.isEmpty())
		{
			continue;
		}
		const int scale = (item.scale > 0) ? item.scale : defaultScale;
		const int cols  = item.text.size();
		const int pad   = item.outline ? 1 : 0;   // outline width in glyph pixels
		const int w = (cols * GLYPH_SIZE + 2 * pad) * scale;
		const int h = (GLYPH_SIZE + 2 * pad) * scale;

		std::vector<Region> order = item.regions;
		if (std::find(order.begin(), order.end(), REGION_IMAGE) == order.end())
		{
			order.push_back(REGION_IMAGE);
		}

		auto place = [&](Region region, int used) -> QRect
		{
			const QRect &r = regionRects[region];
			const int ox = item.offsetX * scale;
			const int oy = item.offsetY * scale;
			int x, y;

			switch (item.anchor)
			{
				case ANCHOR_TOP_LEFT: case ANCHOR_CENTER_LEFT: case ANCHOR_BOTTOM_LEFT:
					x = r.left() + ox; break;
				case ANCHOR_TOP_RIGHT: case ANCHOR_CENTER_RIGHT: case ANCHOR_BOTTOM_RIGHT:
					x = r.right() + 1 - ox - w; break;
				default:
					x = r.left() + (r.width() - w) / 2 + ox; break;
			}
			switch (item.anchor)
			{
				case ANCHOR_TOP_LEFT: case ANCHOR_TOP_CENTER: case ANCHOR_TOP_RIGHT:
					y = r.top() + oy + used; break;
				case ANCHOR_BOTTOM_LEFT: case ANCHOR_BOTTOM_CENTER: case ANCHOR_BOTTOM_RIGHT:
					y = r.bottom() + 1 - oy - h - used; break;
				default:
					y = r.top() + (r.height() - h) / 2 + oy + used; break;
			}
			return QRect(x, y, w, h);
		};

		Region chosen = REGION_IMAGE;
		QRect  rect = place(REGION_IMAGE, stackUsed[{REGION_IMAGE, item.anchor}]);

		for (Region region : order)
		{
			const QRect candidate = place(region, stackUsed[{region, item.anchor}]);

			if (regionRects[region].contains(candidate))
			{
				chosen = region;
				rect = candidate;
				break;
			}
		}
		// When nothing fits the item stays in the image and is clipped to the surface

		stackUsed[{chosen, item.anchor}] += h + scale;

		// Glyph pixels, and the cells around them for the outline, as grids of glyph pixels
		const int gridW = cols * GLYPH_SIZE;
		const int gridH = GLYPH_SIZE;
		std::vector<uint8_t> glyphs(static_cast<size_t>(gridW) * gridH, 0);

		for (int row = 0; row < gridH; row++)
		{
			for (int col = 0; col < gridW; col++)
			{
				glyphs[static_cast<size_t>(row) * gridW + col] = glyphPixelOn(item.text, col, row) ? 1 : 0;
			}
		}

		if (item.outline)
		{
			const int outW = gridW + 2;
			const int outH = gridH + 2;
			std::vector<uint8_t> border(static_cast<size_t>(outW) * outH, 0);

			auto glyphAt = [&](int col, int row) -> bool
			{
				return (col >= 0) && (col < gridW) && (row >= 0) && (row < gridH) &&
					glyphs[static_cast<size_t>(row) * gridW + col];
			};

			for (int row = 0; row < outH; row++)
			{
				for (int col = 0; col < outW; col++)
				{
					// Cell in glyph coordinates is (col - 1, row - 1)
					bool near = false;
					for (int dy = -1; dy <= 1 && !near; dy++)
						for (int dx = -1; dx <= 1 && !near; dx++)
							near = glyphAt(col - 1 + dx, row - 1 + dy);

					border[static_cast<size_t>(row) * outW + col] = (near && !glyphAt(col - 1, row - 1)) ? 1 : 0;
				}
			}
			const size_t first = layer.solidRects.size();

			buildSolidRects(border, outW, outH, rect.topLeft(), scale, layer.solidRects);

			for (size_t i = first; i < layer.solidRects.size(); i++)
			{
				fillRect(layer.solidRects[i], 0xFF000000u);
			}
		}
		{
			const uint32_t argb = 0xFF000000u | (item.color.rgb() & 0x00FFFFFFu);
			const size_t first = layer.solidRects.size();

			buildSolidRects(glyphs, gridW, gridH, QPoint(rect.left() + pad * scale, rect.top() + pad * scale),
				scale, layer.solidRects);

			for (size_t i = first; i < layer.solidRects.size(); i++)
			{
				fillRect(layer.solidRects[i], argb);
			}
		}

		const QRect clipped = rect.intersected(surfaceRect);

		if (!clipped.isEmpty())
		{
			layer.textRects.push_back(clipped);
		}
	}
	layer.changed = true;
	layer.empty = layer.textRects.empty();

	return layer;
}
//----------------------------------------------------------

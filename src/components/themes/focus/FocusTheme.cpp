// #include "FocusTheme.h"

// #include <GfxRenderer.h>
// #include <HalClock.h>
// #include <HalPowerManager.h>
// #include <I18n.h>

// #include <algorithm>
// #include <string>

// #include "RecentBooksStore.h"
// #include "components/UITheme.h"
// #include "components/icons/book.h"
// #include "components/icons/chart.h"
// #include "components/icons/cover.h"
// #include "components/icons/folder.h"
// #include "components/icons/recent.h"
// #include "components/icons/settings2.h"
// #include "components/icons/transfer.h"
// #include "fontIds.h"

// namespace {
// // Icon lookup helper hoàn toàn độc lập
// const uint8_t* getFocusIcon(UIIcon icon, uint32_t size) {
//   if (size == 32) {
//     switch (icon) {
//       case UIIcon::Folder:
//         return FolderIcon;
//       case UIIcon::Transfer:
//         return TransferIcon;
//       case UIIcon::Chart:
//         return ChartIcon;
//       case UIIcon::Settings:
//         return Settings2Icon;
//       case UIIcon::Book:
//         return BookIcon;
//       case UIIcon::Recent:
//         return RecentIcon;
//       default:
//         return nullptr;
//     }
//   }
//   return nullptr;
// }

// int focusedRowY(const int rowY, const int rowHeight, const int contentHeight) {
//   return rowY + std::max(0, rowHeight - contentHeight) / 2;
// }

// constexpr int homeMenuMargin = 20;
// constexpr int homeMarginTop = 30;
// constexpr int subtitleY = 738;

// Rect fittedBitmapRect(const Bitmap& bitmap, const Rect& target) {
//   if (bitmap.getWidth() <= 0 || bitmap.getHeight() <= 0 || target.width <= 0 || target.height <= 0) {
//     return target;
//   }

//   const float widthScale = static_cast<float>(target.width) / static_cast<float>(bitmap.getWidth());
//   const float heightScale = static_cast<float>(target.height) / static_cast<float>(bitmap.getHeight());
//   const float scale = std::min(1.0f, std::min(widthScale, heightScale));
//   const int drawnW = std::min(target.width, std::max(1, static_cast<int>(std::ceil(bitmap.getWidth() * scale))));
//   const int drawnH = std::min(target.height, std::max(1, static_cast<int>(std::ceil(bitmap.getHeight() * scale))));
//   return Rect{target.x + (target.width - drawnW) / 2, target.y + (target.height - drawnH) / 2, drawnW, drawnH};
// }
// }  // namespace

// void FocusTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle) const {
//   (void)subtitle;
//   renderer.fillRect(rect.x, rect.y, rect.width, rect.height, false);

//   // Clock trái
//   char timeBuf[10];
//   const int sidePad = FocusMetrics::values.contentSidePadding;
//   const int clockX = rect.x + sidePad;
//   const int clockY = rect.y;
//   if (halClock.formatTime(timeBuf, sizeof(timeBuf), SETTINGS.clockUtcOffsetQ, SETTINGS.clockFormat == 1)) {
//     renderer.drawText(SMALL_FONT_ID, clockX, clockY, timeBuf);
//   }

//   // Hide last battery draw
//   constexpr int maxBatteryWidth = 80;
//   renderer.fillRect(rect.x + rect.width - maxBatteryWidth, rect.y, maxBatteryWidth,
//                     FocusMetrics::values.batteryHeight + 10, false);

//   const bool showBatteryPercentage =
//       SETTINGS.hideBatteryPercentage != CrossPointSettings::HIDE_BATTERY_PERCENTAGE::HIDE_ALWAYS;
//   // Position icon at right edge, drawBatteryRight will place text to the left
//   const int batteryX = rect.x + rect.width - sidePad - FocusMetrics::values.batteryWidth;
//   drawBatteryRight(renderer,
//                    Rect{batteryX, rect.y, FocusMetrics::values.batteryWidth, FocusMetrics::values.batteryHeight},
//                    showBatteryPercentage);

//   if (title) {
//     int padding = rect.width - batteryX + FocusMetrics::values.batteryWidth;
//     auto truncatedTitle = renderer.truncatedText(UI_12_FONT_ID, title,
//                                                  rect.width - padding * 2 - FocusMetrics::values.contentSidePadding *
//                                                  2, EpdFontFamily::BOLD);
//     // renderer.drawCenteredText(UI_12_FONT_ID, rect.y + FocusMetrics::values.batteryBarHeight,
//     truncatedTitle.c_str(),
//     //                           true, EpdFontFamily::BOLD);
//     renderer.drawText(UI_12_FONT_ID, rect.x + padding, rect.y + FocusMetrics::values.batteryBarHeight,
//                       truncatedTitle.c_str(), true, EpdFontFamily::BOLD);
//     // renderer.drawLine(rect.x, rect.y + rect.height + FocusMetrics::values.batteryBarHeight, rect.x + rect.width,
//     //                   rect.y + rect.height + FocusMetrics::values.batteryBarHeight, 3, true);
//   }

//   if (subtitle) {
//     auto truncatedSubtitle = renderer.truncatedText(
//         SMALL_FONT_ID, subtitle, rect.width - FocusMetrics::values.contentSidePadding * 2, EpdFontFamily::REGULAR);
//     int truncatedSubtitleWidth = renderer.getTextWidth(SMALL_FONT_ID, truncatedSubtitle.c_str());
//     renderer.drawText(SMALL_FONT_ID,
//                       rect.x + rect.width - FocusMetrics::values.contentSidePadding - truncatedSubtitleWidth,
//                       subtitleY + 10, truncatedSubtitle.c_str(), true);
//   }
// }

// // Draw cover - Title, Author, Progress %
// void FocusTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
//                                      int selectorIndex, bool& coverRendered, bool& coverBufferStored,
//                                      bool& bufferRestored, const std::function<bool()>& storeCoverBuffer,
//                                      const BookReadingStats* stats, float progressPercent) const {
//   (void)selectorIndex;
//   (void)bufferRestored;
//   (void)stats;

//   const int sidePad = FocusMetrics::values.contentSidePadding;
//   const int coverH = FocusMetrics::values.homeCoverHeight;
//   const int coverW = coverH * 2 / 3;
//   const int coverX = rect.x + (rect.width - coverW) / 2;
//   const int coverY = rect.y + FocusMetrics::values.homeTopPadding;

//   if (recentBooks.empty()) {
//     renderer.drawCenteredText(UI_12_FONT_ID, coverY + coverH / 2 - 10, tr(STR_NO_OPEN_BOOK));
//     return;
//   }

//   const RecentBook& book = recentBooks[0];
//   if (!coverRendered) {
//     bool hasCover = false;
//     if (!book.coverBmpPath.empty()) {
//       std::string path = UITheme::getCoverThumbPath(book.coverBmpPath, coverH);
//       // std::string path = book.coverBmpPath;
//       FsFile file;
//       if (Storage.openFileForRead("HOME", path, file)) {
//         Bitmap bmp(file);
//         if (bmp.parseHeaders() == BmpReaderError::Ok) {
//           // // // Tính crop để giữ aspect ratio
//           // float cropX = 0.0f, cropY = 0.0f;
//           // Rect targetRect(coverX, coverY, coverW, coverH);
//           // const Rect fittedRect = fittedBitmapRect(bmp, targetRect);

//           // // Vẽ với tham số crop
//           // renderer.drawBitmap(bmp, fittedRect.x, fittedRect.y, fittedRect.width, fittedRect.height, cropX, cropY);

//           // Tính crop để lấp đầy khung (cover behavior)
//           float srcAspect = static_cast<float>(bmp.getWidth()) / static_cast<float>(bmp.getHeight());
//           float tgtAspect = static_cast<float>(coverW) / static_cast<float>(coverH);

//           float cropX = 0.0f, cropY = 0.0f;

//           if (srcAspect > tgtAspect) {
//             // Ảnh nguồn rộng hơn khung đích → crop trái/phải
//             float excess = (srcAspect - tgtAspect) / srcAspect;
//             cropX = excess / 2.0f;  // Căn giữa phần crop ngang
//           } else if (srcAspect < tgtAspect) {
//             // Ảnh nguồn cao hơn khung đích → crop trên/dưới
//             float excess = (tgtAspect - srcAspect) / tgtAspect;
//             cropY = excess / 2.0f;  // Căn giữa phần crop dọc
//           }

//           // Vẽ trực tiếp vào khung đích với tham số crop
//           renderer.drawBitmap(bmp, coverX, coverY, coverW, coverH, cropX, cropY);

//           hasCover = true;
//         }
//         file.close();
//       }
//     }

//     if (hasCover) {
//       coverBufferStored = storeCoverBuffer();
//       coverRendered = coverBufferStored;
//     }

//     if (!hasCover) {
//       // renderer.fillRect(coverX, coverY + coverH / 3, coverW, 2 * coverH / 3, true);
//       // renderer.drawIcon(CoverIcon, coverX + coverW / 2 - 16, coverY + coverH / 6, 32, 32);
//       renderer.drawCenteredText(UI_12_FONT_ID, coverH / 2, "No Cover", true, EpdFontFamily::BOLD);
//     }
//     renderer.drawRoundedRect(coverX, coverY, coverW, coverH, 1, 2, true);
//     // coverBufferStored = storeCoverBuffer();
//     // coverRendered = coverBufferStored;
//   }

//   // Info sách bên dưới cover
//   int infoY = coverY + coverH + 30;
//   int infoX = rect.x + 55;
//   constexpr int TITLE_FONT_UI = UI_10_FONT_ID;

//   // Render title
//   auto titleLines = renderer.wrappedText(TITLE_FONT_UI, book.title.c_str(), coverW, 2);
//   for (const auto& line : titleLines) {
//     renderer.drawText(TITLE_FONT_UI, infoX, infoY, line.c_str(), true, EpdFontFamily::BOLD);
//     infoY += renderer.getLineHeight(TITLE_FONT_UI);
//   }

//   // Render author
//   if (!book.author.empty()) {
//     infoY += 12;
//     renderer.drawText(SMALL_FONT_ID, infoX, infoY, book.author.c_str(), true);
//     infoY += renderer.getLineHeight(SMALL_FONT_ID) + 12;
//   }

//   // Render progress
//   if (progressPercent >= 0.0f) {
//     // infoY += 8;
//     char buf[16];
//     snprintf(buf, sizeof(buf), "%d%% read", static_cast<int>(progressPercent + 0.5f));
//     renderer.drawText(SMALL_FONT_ID, infoX, infoY, buf);
//   }
// }

// void FocusTheme::drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
//                                  const char* btn4, bool allowInvertedText) const {}

// // draw 5 icon : Browser - Recent - File Transfer - Stats - Setting
// void FocusTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
//                                 const std::function<std::string(int index)>& buttonLabel,
//                                 const std::function<UIIcon(int index)>& rowIcon) const {
//   if (buttonCount <= 0) {
//     return;
//   }

//   const int pageWidth = renderer.getScreenWidth();
//   const int pageHeight = renderer.getScreenHeight();
//   const int footerY = pageHeight - FocusMetrics::values.buttonHintsHeight;
//   const int iconSize = 32;

//   // Chia đều cho 5 icon
//   const int slotWidth = pageWidth / 5;

//   // Vẽ nền footer
//   renderer.fillRect(0, footerY, pageWidth, FocusMetrics::values.buttonHintsHeight - iconSize, false);

//   // 5 Icon cố định: Browser | Recent | Transfer | Stats | Settings
//   const UIIcon icons[] = {UIIcon::Folder, UIIcon::Recent, UIIcon::Transfer, UIIcon::Chart, UIIcon::Settings};

//   for (int i = 0; i < 5; i++) {
//     // Tính vị trí center của mỗi slot
//     const int cx = slotWidth * i + slotWidth / 2;
//     const int cy = footerY + (FocusMetrics::values.buttonHintsHeight - iconSize) / 2;

//     // Vẽ icon
//     const uint8_t* bmp = getFocusIcon(icons[i], iconSize);
//     if (bmp) {
//       renderer.drawIcon(bmp, cx - iconSize / 2, cy, iconSize, iconSize);
//     }

//     // Vẽ đường gạch chân nếu được chọn
//     if (i == selectedIndex && selectedIndex >= 0 && selectedIndex < 5) {
//       renderer.drawIcon(bmp, cx - iconSize / 2 + 1, cy, iconSize, iconSize);
//       renderer.drawIcon(bmp, cx - iconSize / 2 + 1, cy + 1, iconSize, iconSize);
//     }
//   }
// }

// void FocusTheme::drawList(const GfxRenderer& renderer, Rect rect, int itemCount, int selectedIndex,
//                           const std::function<std::string(int index)>& rowTitle,
//                           const std::function<std::string(int index)>& rowSubtitle,
//                           const std::function<UIIcon(int index)>& rowIcon,
//                           const std::function<std::string(int index)>& rowValue, bool highlightValue,
//                           const std::function<bool(int index)>& rowDimmed,
//                           const std::function<bool(int index)>& isHeader) const {
//   int rowHeight =
//       (rowSubtitle != nullptr) ? FocusMetrics::values.listWithSubtitleRowHeight : FocusMetrics::values.listRowHeight;
//   int pageItems = rect.height / rowHeight;
//   constexpr int sectionHeaderTopPadding = 15;
//   constexpr int listIndent = 12;

//   // const int totalPages = (itemCount + pageItems - 1) / pageItems;
//   // if (totalPages > 1) {
//   //   constexpr int indicatorWidth = 20;
//   //   constexpr int arrowSize = 6;
//   //   constexpr int margin = 15;  // Offset from right edge

//   //   const int centerX = rect.x + rect.width - indicatorWidth / 2 - margin;
//   //   const int indicatorTop = rect.y;  // Offset to avoid overlapping side button hints
//   //   const int indicatorBottom = rect.y + rect.height - arrowSize;

//   //   Draw up arrow at top(^) - narrow point at top, wide base at bottom for (int i = 0; i < arrowSize; ++i) {
//   //     const int lineWidth = 1 + i * 2;
//   //     const int startX = centerX - i;
//   //     renderer.drawLine(startX, indicatorTop + i, startX + lineWidth - 1, indicatorTop + i);
//   //   }

//   //   Draw down arrow at bottom(v) - wide base at top, narrow point at bottom for (int i = 0; i < arrowSize; ++i) {
//   //     const int lineWidth = 1 + (arrowSize - 1 - i) * 2;
//   //     const int startX = centerX - (arrowSize - 1 - i);
//   //     renderer.drawLine(startX, indicatorBottom - arrowSize + 1 + i, startX + lineWidth - 1,
//   //                       indicatorBottom - arrowSize + 1 + i);
//   //   }
//   // }

//   // Draw selection (skip header rows)
//   int contentWidth = rect.width - 5;
//   constexpr int maxValueWidth = 200;
//   constexpr int minValueGap = 12;

//   // Draw all items
//   const auto pageStartIndex = selectedIndex / pageItems * pageItems;
//   const int rectBottom = rect.y + rect.height;

//   // Draw all visible page items
//   for (int i = pageStartIndex; i < itemCount && i < pageStartIndex + pageItems; i++) {
//     const int itemY = rect.y + (i % pageItems) * rowHeight;

//     const int indicatorX = rect.x + FocusMetrics::values.contentSidePadding + 2;
//     const int indicatorY = itemY + 2;
//     constexpr int indicatorW = 3;
//     constexpr int indicatorH = 30;

//     int rowTextWidth = contentWidth - FocusMetrics::values.contentSidePadding * 2;
//     std::string valueText;
//     if (rowValue != nullptr) {
//       valueText = rowValue(i);
//       if (!valueText.empty()) {
//         int maxValW = std::max(0, rowTextWidth - 40 - minValueGap);
//         valueText = renderer.truncatedText(UI_10_FONT_ID, valueText.c_str(), maxValW);
//         int valueWidth = renderer.getTextWidth(UI_10_FONT_ID, valueText.c_str()) + minValueGap;
//         rowTextWidth -= valueWidth;
//       }
//     }

//     auto itemName = rowTitle(i);
//     auto font = UI_12_FONT_ID;
//     auto item = renderer.truncatedText(font, itemName.c_str(), rowTextWidth);
//     if (isHeader && isHeader(i)) {
//       renderer.drawText(font, rect.x + FocusMetrics::values.contentSidePadding + listIndent, itemY, item.c_str(),
//       true,
//                         EpdFontFamily::BOLD);
//       continue;
//     }
//     renderer.drawText(font, rect.x + FocusMetrics::values.contentSidePadding + listIndent, itemY + 2, item.c_str(),
//                       true, (i == selectedIndex) ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);

//     // Apply checkerboard dither to create gray text effect for dimmed items
//     if (rowDimmed && rowDimmed(i) && i != selectedIndex) {
//       const int titleWidth = renderer.getTextWidth(font, item.c_str());
//       const int lineH = renderer.getLineHeight(font);
//       const int tx = rect.x + FocusMetrics::values.contentSidePadding;
//       for (int py = itemY; py < itemY + lineH; py++)
//         for (int px = tx; px < tx + titleWidth; px++)
//           if ((px + py) % 2 == 0) renderer.drawPixel(px, py, false);
//     }

//     if (rowSubtitle != nullptr) {
//       std::string subtitleText = rowSubtitle(i);
//       if (!subtitleText.empty()) {
//         auto subtitle = renderer.truncatedText(UI_10_FONT_ID, subtitleText.c_str(), rowTextWidth);
//         renderer.drawText(UI_10_FONT_ID, rect.x + FocusMetrics::values.contentSidePadding + listIndent, itemY + 28,
//                           subtitle.c_str(), true);
//       }
//     }

//     if (!valueText.empty()) {
//       const auto valueTextWidth = renderer.getTextWidth(UI_10_FONT_ID, valueText.c_str());
//       int valueY = itemY;
//       if (rowSubtitle != nullptr) {
//         valueY = itemY;
//       }
//       renderer.drawText(UI_10_FONT_ID, rect.x + contentWidth - FocusMetrics::values.contentSidePadding -
//       valueTextWidth,
//                         itemY + 6, valueText.c_str(), true);
//     }
//   }
// }

#include "FocusTheme.h"

#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalPowerManager.h>
#include <I18n.h>

#include <algorithm>
#include <string>

#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/icons/book.h"
#include "components/icons/chart.h"
#include "components/icons/cover.h"
#include "components/icons/folder.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/transfer.h"
#include "fontIds.h"

namespace {

// ============================================================================
// CONSTANTS
// ============================================================================

constexpr int kIconSize = 32;
constexpr int kCornerRadius = 2;

constexpr int kCoverAspectW = 2;
constexpr int kCoverAspectH = 3;

constexpr int kContentPadding = 20;
constexpr int kSectionIndent = 12;

constexpr int kHeaderTitleYOffset = 24;
constexpr int kContentTopSpacing = 24;
constexpr int kHeaderTitleIndent = 52;

constexpr int kTextSpacing = 12;
constexpr int kSubtitleY = 738;

constexpr int kBatteryAreaWidth = 80;

constexpr int kTitleYOffset = 40;
constexpr int kListTextYOffset = 2;
constexpr int kListSubtitleYOffset = 28;
constexpr int kListValueYOffset = 6;

constexpr int kFooterIconShadowOffset = 1;

constexpr int kTitleFont = UI_12_FONT_ID;
constexpr int kSubtitleFont = SMALL_FONT_ID;
constexpr int kValueFont = UI_10_FONT_ID;

constexpr auto kBold = EpdFontFamily::BOLD;
constexpr auto kRegular = EpdFontFamily::REGULAR;

// ============================================================================
// ICONS
// ============================================================================

const uint8_t* getFocusIcon(UIIcon icon) {
  switch (icon) {
    case UIIcon::Folder:
      return FolderIcon;

    case UIIcon::Transfer:
      return TransferIcon;

    case UIIcon::Chart:
      return ChartIcon;

    case UIIcon::Settings:
      return Settings2Icon;

    case UIIcon::Book:
      return BookIcon;

    case UIIcon::Recent:
      return RecentIcon;

    default:
      return nullptr;
  }
}

// ============================================================================
// HELPERS
// ============================================================================

Rect makeCoverRect(const Rect& parent, int height) {
  const int width = height * kCoverAspectW / kCoverAspectH;

  return Rect(parent.x + (parent.width - width) / 2, parent.y, width, height);
}

struct Crop {
  float x = 0.f;
  float y = 0.f;
};

Crop calcCrop(float srcAspect, float targetAspect) {
  Crop crop;

  if (srcAspect > targetAspect) {
    crop.x = (srcAspect - targetAspect) / srcAspect / 2.f;
  } else if (srcAspect < targetAspect) {
    crop.y = (targetAspect - srcAspect) / targetAspect / 2.f;
  }

  return crop;
}

void drawText(GfxRenderer& renderer, int font, int x, int y, const std::string& text,
              EpdFontFamily::Style style = kRegular) {
  renderer.drawText(font, x, y, text.c_str(), true, style);
}

void drawDither(GfxRenderer& renderer, int font, int x, int y, const std::string& text) {
  const int width = renderer.getTextWidth(font, text.c_str());
  const int height = renderer.getLineHeight(font);

  for (int py = y; py < y + height; ++py) {
    for (int px = x; px < x + width; ++px) {
      if ((px + py) % 2 == 0) {
        renderer.drawPixel(px, py, false);
      }
    }
  }
}

}  // namespace

// ============================================================================
// HEADER
// ============================================================================

void FocusTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle) const {
  const auto& m = FocusMetrics::values;

  renderer.fillRect(rect.x, rect.y, rect.width, rect.height, false);

  // Clock
  char timeBuf[10];

  if (halClock.formatTime(timeBuf, sizeof(timeBuf), SETTINGS.clockUtcOffsetQ, SETTINGS.clockFormat == 1)) {
    renderer.drawText(kSubtitleFont, rect.x + kContentPadding, rect.y, timeBuf);
  }

  // Battery
  renderer.fillRect(rect.x + rect.width - kBatteryAreaWidth, rect.y, kBatteryAreaWidth, m.batteryHeight + 10, false);

  const bool showBatteryPercent =
      SETTINGS.hideBatteryPercentage != CrossPointSettings::HIDE_BATTERY_PERCENTAGE::HIDE_ALWAYS;

  const int batteryX = rect.x + rect.width - kContentPadding - m.batteryWidth;

  drawBatteryRight(renderer, Rect(batteryX, rect.y, m.batteryWidth, m.batteryHeight), showBatteryPercent);

  // Title
  if (title) {
    const int maxWidth = rect.width - kContentPadding * 4;

    const auto text = renderer.truncatedText(kTitleFont, title, maxWidth, kBold);

    renderer.drawText(kTitleFont, rect.x + kHeaderTitleIndent, rect.y + kHeaderTitleYOffset, text.c_str(), true, kBold);
  }

  // Subtitle
  if (subtitle) {
    const auto text = renderer.truncatedText(kSubtitleFont, subtitle, rect.width - kContentPadding * 2);

    const int width = renderer.getTextWidth(kSubtitleFont, text.c_str());

    renderer.drawText(kSubtitleFont, rect.x + rect.width - kContentPadding - width, kSubtitleY, text.c_str(), true);
  }
}

// ============================================================================
// RECENT BOOK COVER
// ============================================================================

void FocusTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                     int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                     bool& bufferRestored, const std::function<bool()>& storeCoverBuffer,
                                     const BookReadingStats* stats, float progressPercent) const {
  (void)selectorIndex;
  (void)bufferRestored;
  (void)stats;

  const auto& m = FocusMetrics::values;

  if (recentBooks.empty()) {
    renderer.drawCenteredText(kTitleFont, rect.y + m.homeCoverHeight / 2, tr(STR_NO_OPEN_BOOK));

    return;
  }

  const RecentBook& book = recentBooks[0];

  const Rect coverRect =
      makeCoverRect(Rect(rect.x, rect.y + m.homeTopPadding, rect.width, m.homeCoverHeight), m.homeCoverHeight);

  // Cover
  if (!coverRendered) {
    bool hasCover = false;

    if (!book.coverBmpPath.empty()) {
      const std::string path = UITheme::getCoverThumbPath(book.coverBmpPath, m.homeCoverHeight);

      FsFile file;

      if (Storage.openFileForRead("HOME", path, file)) {
        Bitmap bmp(file);

        if (bmp.parseHeaders() == BmpReaderError::Ok) {
          const float srcAspect = static_cast<float>(bmp.getWidth()) / bmp.getHeight();

          const float targetAspect = static_cast<float>(coverRect.width) / coverRect.height;

          const Crop crop = calcCrop(srcAspect, targetAspect);

          renderer.drawBitmap(bmp, coverRect.x, coverRect.y, coverRect.width, coverRect.height, crop.x, crop.y);

          hasCover = true;
        }

        file.close();
      }
    }

    if (hasCover) {
      coverBufferStored = storeCoverBuffer();
      coverRendered = coverBufferStored;
    } else {
      renderer.drawCenteredText(kTitleFont, coverRect.y + coverRect.height / 2, "No Cover", true, kBold);
    }

    renderer.drawRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, 1, kCornerRadius, true);
  }

  // Book info
  int textY = coverRect.y + coverRect.height + 30;

  const int textX = rect.x + 55;

  auto titleLines = renderer.wrappedText(kValueFont, book.title.c_str(), coverRect.width, 2, kBold);

  for (const auto& line : titleLines) {
    drawText(renderer, kValueFont, textX, textY, line, kBold);
    textY += renderer.getLineHeight(kValueFont);
  }

  if (!book.author.empty()) {
    textY += kTextSpacing;

    drawText(renderer, kSubtitleFont, textX, textY, book.author);

    textY += renderer.getLineHeight(kSubtitleFont);
  }

  if (progressPercent >= 0.f) {
    char progress[16];

    snprintf(progress, sizeof(progress), "%d%% read", static_cast<int>(progressPercent + 0.5f));

    textY += kTextSpacing;

    drawText(renderer, kSubtitleFont, textX, textY, progress);
  }
}

// ============================================================================
// BUTTON MENU
// ============================================================================

void FocusTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                const std::function<std::string(int)>& buttonLabel,
                                const std::function<UIIcon(int)>& rowIcon) const {
  (void)rect;
  (void)buttonLabel;
  (void)rowIcon;

  if (buttonCount <= 0) {
    return;
  }

  const auto& m = FocusMetrics::values;

  constexpr UIIcon icons[] = {UIIcon::Folder, UIIcon::Recent, UIIcon::Transfer, UIIcon::Chart, UIIcon::Settings};

  constexpr int iconCount = sizeof(icons) / sizeof(icons[0]);

  const int screenWidth = renderer.getScreenWidth();

  const int footerY = renderer.getScreenHeight() - m.buttonHintsHeight;

  const int slotWidth = screenWidth / iconCount;

  renderer.fillRect(0, footerY, screenWidth, m.buttonHintsHeight - kIconSize, false);

  for (int i = 0; i < iconCount; ++i) {
    const int centerX = slotWidth * i + slotWidth / 2;

    const int iconX = centerX - kIconSize / 2;

    const int iconY = footerY + (m.buttonHintsHeight - kIconSize) / 2;

    const uint8_t* bmp = getFocusIcon(icons[i]);

    if (!bmp) {
      continue;
    }

    if (i == selectedIndex) {
      renderer.drawIcon(bmp, iconX + kFooterIconShadowOffset, iconY, kIconSize, kIconSize);

      renderer.drawIcon(bmp, iconX + kFooterIconShadowOffset, iconY + kFooterIconShadowOffset, kIconSize, kIconSize);
    }

    renderer.drawIcon(bmp, iconX, iconY, kIconSize, kIconSize);
  }
}

// ============================================================================
// BUTTON HINT
// ============================================================================

void FocusTheme::drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                                 const char* btn4, bool allowInvertedText) const {
  // Các màn khác vẫn vẽ bình thường
  BaseTheme::drawButtonHints(renderer, btn1, btn2, btn3, btn4, allowInvertedText);
}

// ============================================================================
// LIST
// ============================================================================

void FocusTheme::drawList(const GfxRenderer& renderer, Rect rect, int itemCount, int selectedIndex,
                          const std::function<std::string(int)>& rowTitle,
                          const std::function<std::string(int)>& rowSubtitle, const std::function<UIIcon(int)>& rowIcon,
                          const std::function<std::string(int)>& rowValue, bool highlightValue,
                          const std::function<bool(int)>& rowDimmed, const std::function<bool(int)>& isHeader) const {
  (void)rowIcon;
  (void)highlightValue;

  const auto& m = FocusMetrics::values;

  const int rowHeight = rowSubtitle ? m.listWithSubtitleRowHeight : m.listRowHeight;

  const int pageItems = std::max(1, rect.height / rowHeight);

  const int pageStart = selectedIndex / pageItems * pageItems;

  const int contentWidth = rect.width - 5;

  constexpr int valueGap = 12;

  for (int i = pageStart; i < itemCount && i < pageStart + pageItems; ++i) {
    const int itemY = rect.y + kContentTopSpacing + (i % pageItems) * rowHeight;

    const bool selected = i == selectedIndex;

    const bool header = isHeader && isHeader(i);

    int textWidth = contentWidth - kContentPadding * 2;

    // Value
    std::string valueText;

    if (rowValue) {
      valueText = rowValue(i);

      if (!valueText.empty()) {
        valueText = renderer.truncatedText(kValueFont, valueText.c_str(), textWidth / 2);

        textWidth -= renderer.getTextWidth(kValueFont, valueText.c_str()) + valueGap;
      }
    }

    // Title
    const int textX = rect.x + kContentPadding + (header ? kSectionIndent : 0);

    const auto titleText = renderer.truncatedText(kTitleFont, rowTitle(i).c_str(), textWidth);

    const auto titleStyle = selected ? kBold : kRegular;

    renderer.drawText(kTitleFont, textX, itemY + kListTextYOffset, titleText.c_str(), true,
                      header ? kBold : titleStyle);

    // Dither
    if (rowDimmed && rowDimmed(i) && !selected) {
      drawDither(const_cast<GfxRenderer&>(renderer), kTitleFont, textX, itemY + kListTextYOffset, titleText);
    }

    // Subtitle
    if (rowSubtitle) {
      const auto subtitle = rowSubtitle(i);

      if (!subtitle.empty()) {
        const auto subtitleText = renderer.truncatedText(kSubtitleFont, subtitle.c_str(), textWidth);

        renderer.drawText(kSubtitleFont, textX, itemY + kListSubtitleYOffset, subtitleText.c_str(), true);
      }
    }

    // Value
    if (!valueText.empty()) {
      const int valueWidth = renderer.getTextWidth(kValueFont, valueText.c_str());

      renderer.drawText(kValueFont, rect.x + contentWidth - kContentPadding - valueWidth, itemY + kListValueYOffset,
                        valueText.c_str(), true);
    }
  }
}

// ============================================================================
// TAB BAR
// ============================================================================

void FocusTheme::drawTabBar(const GfxRenderer& renderer, const Rect rect, const std::vector<TabInfo>& tabs,
                            bool selected) const {
  (void)selected;

  constexpr int underlineHeight = 3;
  constexpr int underlineGap = 22;
  constexpr int textYOffset = 20;

  const int lineHeight = renderer.getLineHeight(kTitleFont);

  const int tabWidth = rect.width / tabs.size();

  for (size_t i = 0; i < tabs.size(); ++i) {
    const auto& tab = tabs[i];

    const auto style = tab.selected ? kBold : kRegular;

    const int textWidth = renderer.getTextWidth(kTitleFont, tab.label, style);

    // center text inside slot
    const int textX = rect.x + i * tabWidth + (tabWidth - textWidth) / 2;

    renderer.drawText(kTitleFont, textX, rect.y + textYOffset, tab.label, true, style);

    // underline
    if (tab.selected) {
      renderer.fillRect(textX, rect.y + lineHeight + underlineGap, textWidth, underlineHeight, true);
    }
  }
}
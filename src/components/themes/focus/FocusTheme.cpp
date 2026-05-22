#include "FocusTheme.h"

#include <Arduino.h>
#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "RecentBooksStore.h"
#include "activities/reader/BookReadingStats.h"
#include "components/UITheme.h"
#include "components/icons/cover.h"
#include "fontIds.h"

namespace {

struct FocusQuote {
  const char* text;
  const char* author;
};

constexpr FocusQuote kQuotes[] = {
    {"\"A book is a dream you hold in your hands.\"", "Neil Gaiman"},
    {"\"I have always imagined that Paradise will be a kind of library.\"", "Jorge Luis Borges"},
    {"\"A reader lives a thousand lives before he dies. The man who never reads lives only one.\"",
     "George R.R. Martin"},
    {"\"So many books, so little time.\"", "Frank Zappa"},
    {"\"If you only read the books that everyone else is reading, you can only think what everyone else is thinking.\"",
     "Haruki Murakami"},
};

constexpr int kCoverCornerRadius = 8;
constexpr int kProgressBarHeight = 6;

constexpr int kFileBrowserIconSize = 24;
constexpr int kFileBrowserRowVerticalPadding = 6;
constexpr int kFileBrowserTextGap = 8;
constexpr int kFileBrowserValueMaxWidth = 76;

int homeButtonHintSelection = -1;

Rect coverRectForScreen(const GfxRenderer& renderer, const Rect& rect) {
  const int coverH = FocusMetrics::values.homeCoverHeight;
  const int coverW = static_cast<int>((static_cast<int64_t>(coverH) * 3 + 2) / 5);
  const int coverX = (renderer.getScreenWidth() - coverW) / 2;
  const int coverY = rect.y + 24;
  return Rect{coverX, coverY, coverW, coverH};
}

Rect fittedBitmapRect(const Bitmap& bitmap, const Rect& target) {
  if (bitmap.getWidth() <= 0 || bitmap.getHeight() <= 0 || target.width <= 0 || target.height <= 0) {
    return target;
  }

  const float widthScale = static_cast<float>(target.width) / static_cast<float>(bitmap.getWidth());

  const float heightScale = static_cast<float>(target.height) / static_cast<float>(bitmap.getHeight());

  const float scale = std::min(1.0f, std::min(widthScale, heightScale));

  const int drawnW = std::min(target.width, std::max(1, static_cast<int>(std::ceil(bitmap.getWidth() * scale))));

  const int drawnH = std::min(target.height, std::max(1, static_cast<int>(std::ceil(bitmap.getHeight() * scale))));

  return Rect{target.x + (target.width - drawnW) / 2, target.y + (target.height - drawnH) / 2, drawnW, drawnH};
}

uint8_t selectedQuoteIndex() {
  static bool initialized = false;
  static uint8_t index = 0;

  if (!initialized) {
    index = static_cast<uint8_t>((millis() / 137u) % (sizeof(kQuotes) / sizeof(kQuotes[0])));
    initialized = true;
  }

  return index;
}

void drawProgressBlock(const GfxRenderer& renderer, int x, int y, int width, const BookReadingStats* stats,
                       float progressPercent) {
  if ((stats == nullptr || stats->totalReadingSeconds == 0) && progressPercent < 0.0f) {
    return;
  }

  if (stats != nullptr && stats->totalReadingSeconds > 0) {
    char duration[32];

    BookReadingStats::formatDuration(stats->totalReadingSeconds, duration, sizeof(duration));

    renderer.drawText(UI_10_FONT_ID, x, y, duration);

    y += renderer.getLineHeight(UI_10_FONT_ID) + 6;
  }

  if (progressPercent < 0.0f) {
    return;
  }

  const int progress = std::clamp(static_cast<int>(progressPercent + 0.5f), 0, 100);

  renderer.fillRectDither(x, y, width, kProgressBarHeight, Color::LightGray);

  const int fillW = (width * progress) / 100;

  if (fillW > 0) {
    renderer.fillRectDither(x, y, fillW, kProgressBarHeight, Color::DarkGray);
  }

  char progressLabel[12];

  snprintf(progressLabel, sizeof(progressLabel), "%d%%", progress);

  const int labelW = renderer.getTextWidth(UI_10_FONT_ID, progressLabel);

  renderer.drawText(UI_10_FONT_ID, x + width - labelW, y + kProgressBarHeight + 4, progressLabel);
}

void drawMissingBookCover(const GfxRenderer& renderer, const Rect& coverRect, const RecentBook& book) {
  renderer.fillRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, kCoverCornerRadius,
                           Color::White);

  renderer.drawRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, 1, kCoverCornerRadius, true);

  constexpr int iconSize = 40;

  renderer.drawIcon(CoverIcon, coverRect.x + (coverRect.width - iconSize) / 2, coverRect.y + 80, iconSize, iconSize);

  const std::string title = book.title.empty() ? book.path : book.title;

  auto titleLines = renderer.wrappedText(UI_12_FONT_ID, title.c_str(), coverRect.width - 32, 4);

  int textY = coverRect.y + 150;

  for (const auto& line : titleLines) {
    const int lineW = renderer.getTextWidth(UI_12_FONT_ID, line.c_str());

    renderer.drawText(UI_12_FONT_ID, coverRect.x + (coverRect.width - lineW) / 2, textY, line.c_str(), true,
                      EpdFontFamily::BOLD);

    textY += renderer.getLineHeight(UI_12_FONT_ID);
  }

  if (!book.author.empty()) {
    textY += 12;

    auto authorLines = renderer.wrappedText(UI_10_FONT_ID, book.author.c_str(), coverRect.width - 32, 2);

    for (const auto& line : authorLines) {
      const int lineW = renderer.getTextWidth(UI_10_FONT_ID, line.c_str());

      renderer.drawText(UI_10_FONT_ID, coverRect.x + (coverRect.width - lineW) / 2, textY, line.c_str());

      textY += renderer.getLineHeight(UI_10_FONT_ID);
    }
  }
}

}  // namespace

void FocusTheme::setHomeButtonHintSelection(const int selectedIndex) { homeButtonHintSelection = selectedIndex; }

void FocusTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle) const {
  (void)subtitle;

  renderer.fillRect(rect.x, rect.y, rect.width, rect.height, false);

  const bool showBatteryPercentage =
      SETTINGS.hideBatteryPercentage != CrossPointSettings::HIDE_BATTERY_PERCENTAGE::HIDE_ALWAYS;

  const int batteryX = rect.x + rect.width - 12 - FocusMetrics::values.batteryWidth;

  drawBatteryRight(renderer,
                   Rect{batteryX, rect.y + 5, FocusMetrics::values.batteryWidth, FocusMetrics::values.batteryHeight},
                   showBatteryPercentage);

  if (title) {
    constexpr int titleInsetX = 12;

    const int maxTitleWidth = batteryX - rect.x - titleInsetX - FocusMetrics::values.contentSidePadding;

    auto truncatedTitle = renderer.truncatedText(UI_12_FONT_ID, title, maxTitleWidth, EpdFontFamily::BOLD);

    renderer.drawText(UI_12_FONT_ID, rect.x + titleInsetX, rect.y + FocusMetrics::values.batteryBarHeight + 3,
                      truncatedTitle.c_str(), true, EpdFontFamily::BOLD);

    renderer.drawLine(rect.x, rect.y + rect.height - 3, rect.x + rect.width - 1, rect.y + rect.height - 3, 3, true);
  }
}

void FocusTheme::drawTabBar(const GfxRenderer& renderer, Rect rect, const std::vector<TabInfo>& tabs,
                            const bool selected) const {
  (void)selected;

  if (tabs.empty()) {
    return;
  }

  const int tabCount = static_cast<int>(tabs.size());

  const int lineY = rect.y + rect.height - 1;

  renderer.drawLine(rect.x, rect.y, rect.x + rect.width - 1, rect.y, true);

  renderer.drawLine(rect.x, lineY, rect.x + rect.width - 1, lineY, true);

  for (int i = 0; i < tabCount; i++) {
    const int slotX = rect.x + (i * rect.width) / tabCount;

    const int nextSlotX = rect.x + ((i + 1) * rect.width) / tabCount;

    const int slotWidth = nextSlotX - slotX;

    const auto& tab = tabs[i];

    const auto fontStyle = tab.selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;

    const int textWidth = renderer.getTextWidth(UI_10_FONT_ID, tab.label, fontStyle);

    const int textX = slotX + (slotWidth - textWidth) / 2;

    const int textY = rect.y + (rect.height - renderer.getLineHeight(UI_10_FONT_ID)) / 2;

    renderer.drawText(UI_10_FONT_ID, textX, textY, tab.label, true, fontStyle);
  }
}

int FocusTheme::compactFileBrowserRowHeight(const GfxRenderer& renderer) const {
  const int textHeight = renderer.getLineHeight(UI_10_FONT_ID) * 2 + kFileBrowserRowVerticalPadding;

  return std::max(kFileBrowserIconSize + kFileBrowserRowVerticalPadding, textHeight);
}

void FocusTheme::drawList(const GfxRenderer& renderer, Rect rect, int itemCount, int selectedIndex,
                          const std::function<std::string(int index)>& rowTitle,
                          const std::function<std::string(int index)>& rowSubtitle,
                          const std::function<UIIcon(int index)>& rowIcon,
                          const std::function<std::string(int index)>& rowValue, bool highlightValue,
                          const std::function<bool(int index)>& rowDimmed,
                          const std::function<bool(int index)>& isHeader) const {
  LyraTheme::drawList(renderer, rect, itemCount, selectedIndex, rowTitle, rowSubtitle, rowIcon, rowValue,
                      highlightValue, rowDimmed, isHeader);
}

void FocusTheme::drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                                 const char* btn4, const bool allowInvertedText) const {
  (void)allowInvertedText;

  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();

  constexpr int barH = 58;
  constexpr int iconSize = 20;

  const int barY = screenH - barH;

  renderer.fillRect(0, barY, screenW, barH, false);

  renderer.drawLine(0, barY, screenW, barY, true);

  const char* labels[4] = {btn1, btn2, btn3, btn4};

  const UIIcon icons[4] = {UIIcon::Folder, UIIcon::Recent, UIIcon::Wifi, UIIcon::Settings};

  const int sectionW = screenW / 4;

  for (int i = 0; i < 4; i++) {
    if (labels[i] == nullptr || labels[i][0] == '\0') {
      continue;
    }

    const int centerX = i * sectionW + sectionW / 2;

    const uint8_t* iconBitmap = iconForName(icons[i], iconSize);

    if (iconBitmap != nullptr) {
      renderer.drawIcon(iconBitmap, centerX - iconSize / 2, barY + 5, iconSize, iconSize);
    }

    const int textW = renderer.getTextWidth(SMALL_FONT_ID, labels[i]);

    renderer.drawText(SMALL_FONT_ID, centerX - textW / 2, barY + 32, labels[i]);
  }
}

void FocusTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                     int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                     bool& bufferRestored, const std::function<bool()>& storeCoverBuffer,
                                     const BookReadingStats* stats, float progressPercent) const {
  (void)selectorIndex;
  (void)bufferRestored;
  (void)stats;

  // const int screenW = renderer.getScreenWidth();
  // const int screenH = renderer.getScreenHeight();

  const Rect coverRect = coverRectForScreen(renderer, rect);

  // =========================================================
  // EMPTY STATE
  // =========================================================

  if (recentBooks.empty()) {
    renderer.drawRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, 1, kCoverCornerRadius, true);

    const FocusQuote& quote = kQuotes[selectedQuoteIndex()];

    const int quoteW = coverRect.width - 40;

    auto lines = renderer.wrappedText(UI_12_FONT_ID, quote.text, quoteW, 6);

    int y = coverRect.y + 80;

    for (const auto& line : lines) {
      renderer.drawText(UI_12_FONT_ID, coverRect.x + 20, y, line.c_str());

      y += renderer.getLineHeight(UI_12_FONT_ID);
    }

    renderer.drawText(UI_10_FONT_ID, coverRect.x + 20, coverRect.y + coverRect.height - 40, quote.author, true,
                      EpdFontFamily::ITALIC);

    coverRendered = false;
    coverBufferStored = false;

    return;
  }

  // =========================================================
  // DRAW COVER
  // =========================================================

  const RecentBook& book = recentBooks[0];

  if (!coverRendered) {
    bool hasCover = false;

    if (!book.coverBmpPath.empty()) {
      std::string coverBmpPath = UITheme::getCoverThumbPath(book.coverBmpPath, coverRect.height);

      if (coverBmpPath.empty() || !Storage.exists(coverBmpPath.c_str())) {
        coverBmpPath = UITheme::getCoverThumbPath(book.coverBmpPath, coverRect.width, coverRect.height);
      }

      FsFile file;

      if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
        Bitmap bitmap(file);

        if (bitmap.parseHeaders() == BmpReaderError::Ok) {
          renderer.fillRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, kCoverCornerRadius,
                                   Color::White);

          const Rect bitmapRect = fittedBitmapRect(bitmap, coverRect);

          renderer.drawBitmap(bitmap, bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height);

          renderer.maskRoundedRectOutsideCorners(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height,
                                                 kCoverCornerRadius);

          renderer.drawRoundedRect(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height, 1,
                                   kCoverCornerRadius, true);

          hasCover = true;
        }

        file.close();
      }
    }

    if (!hasCover) {
      drawMissingBookCover(renderer, coverRect, book);
    }

    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  }
  // =========================================================
  // BOOK INFO
  // =========================================================

  const int infoX = coverRect.x;

  int infoY = coverRect.y + coverRect.height + 40;

  const int infoW = coverRect.width;

  std::string title = book.title.empty() ? book.path : book.title;

  auto titleLines = renderer.wrappedText(UI_12_FONT_ID, title.c_str(), infoW, 2);

  for (const auto& line : titleLines) {
    renderer.drawText(UI_12_FONT_ID, infoX, infoY, line.c_str(), true, EpdFontFamily::BOLD);

    infoY += renderer.getLineHeight(UI_12_FONT_ID);
  }

  // =========================================================
  // AUTHOR
  // =========================================================

  if (!book.author.empty()) {
    infoY += 14;

    auto authorLines = renderer.wrappedText(UI_10_FONT_ID, book.author.c_str(), infoW, 2);

    for (const auto& line : authorLines) {
      renderer.drawText(UI_10_FONT_ID, infoX, infoY, line.c_str(), true, EpdFontFamily::ITALIC);

      infoY += renderer.getLineHeight(UI_10_FONT_ID);
    }
  }

  // =========================================================
  // PROGRESS
  // =========================================================

  if (progressPercent >= 0.0f) {
    infoY += 14;

    const int progress = std::clamp(static_cast<int>(progressPercent + 0.5f), 0, 100);

    char label[32];

    snprintf(label, sizeof(label), "%d%% completed", progress);

    renderer.drawText(UI_10_FONT_ID, infoX, infoY, label);
  }

  // =========================================================
  // CACHE
  // =========================================================

  // coverBufferStored = storeCoverBuffer();
  // coverRendered = coverBufferStored;
}

void FocusTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                const std::function<std::string(int index)>& buttonLabel,
                                const std::function<UIIcon(int index)>& rowIcon) const {
  (void)renderer;
  (void)rect;
  (void)buttonCount;
  (void)selectedIndex;
  (void)buttonLabel;
  (void)rowIcon;

  // disable popup menu
}

#include <Arduino.h>
#include <Epub.h>
#include <FontCacheManager.h>
#include <FontDecompressor.h>
#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalDisplay.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <HalSystem.h>
#include <HalTiltSensor.h>
#include <I18n.h>
#include <Logging.h>
#include <SPI.h>
#include <builtinFonts/all.h>

#ifdef SIMULATOR
using esp_reset_reason_t = int;
using esp_sleep_wakeup_cause_t = int;
enum : int {
  ESP_RST_UNKNOWN = 0,
  ESP_RST_POWERON,
  ESP_RST_EXT,
  ESP_RST_SW,
  ESP_RST_PANIC,
  ESP_RST_INT_WDT,
  ESP_RST_TASK_WDT,
  ESP_RST_WDT,
  ESP_RST_DEEPSLEEP,
  ESP_RST_BROWNOUT,
  ESP_RST_SDIO,
  ESP_RST_USB,
  ESP_RST_JTAG,
  ESP_RST_EFUSE,
  ESP_RST_PWR_GLITCH,
  ESP_RST_CPU_LOCKUP
};
enum : int {
  ESP_SLEEP_WAKEUP_UNDEFINED = 0,
  ESP_SLEEP_WAKEUP_ALL,
  ESP_SLEEP_WAKEUP_EXT0,
  ESP_SLEEP_WAKEUP_EXT1,
  ESP_SLEEP_WAKEUP_TIMER,
  ESP_SLEEP_WAKEUP_TOUCHPAD,
  ESP_SLEEP_WAKEUP_ULP,
  ESP_SLEEP_WAKEUP_GPIO,
  ESP_SLEEP_WAKEUP_UART,
  ESP_SLEEP_WAKEUP_WIFI,
  ESP_SLEEP_WAKEUP_COCPU,
  ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG,
  ESP_SLEEP_WAKEUP_BT
};
inline esp_reset_reason_t esp_reset_reason() { return ESP_RST_UNKNOWN; }
inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() { return ESP_SLEEP_WAKEUP_UNDEFINED; }
#else
#include <esp_sleep.h>
#include <esp_system.h>
#endif

#include <algorithm>
#include <cstring>

#include "AppVersion.h"
#include "CrossPointSettings.h"
#include "CrossPointState.h"
#include "GlobalActions.h"
#include "KOReaderCredentialStore.h"
#include "MappedInputManager.h"
#include "OpdsServerStore.h"
#include "RecentBooksStore.h"
#include "SdCardFontSystem.h"
#include "activities/Activity.h"
#include "activities/ActivityManager.h"
#include "activities/reader/KOReaderSyncActivity.h"
#include "activities/settings/KOReaderSettingsActivity.h"
#include "activities/settings/SdFirmwareUpdateActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"
#ifdef SIMULATOR
#include "simulator/SimulatorSmokeTest.h"
#endif
#include "images/LoadingIcon.h"
#include "util/ButtonNavigator.h"
#include "util/ScreenshotUtil.h"

MappedInputManager mappedInputManager(gpio);
GfxRenderer renderer(display);
ActivityManager activityManager(renderer, mappedInputManager);
FontDecompressor fontDecompressor;
SdCardFontSystem sdFontSystem;
FontCacheManager fontCacheManager(renderer.getFontMap(), renderer.getSdCardFonts());
static unsigned long allowSleepAt = 0;

// Fonts
#ifndef OMIT_TEENSY_FONT
EpdFont bookerly8RegularFont(&bookerly_8_regular);
EpdFont bookerly8BoldFont(&bookerly_8_bold);
EpdFont bookerly8ItalicFont(&bookerly_8_italic);
EpdFont bookerly8BoldItalicFont(&bookerly_8_bolditalic);
EpdFontFamily bookerly8FontFamily(&bookerly8RegularFont, &bookerly8BoldFont, &bookerly8ItalicFont, &bookerly8BoldItalicFont);
#endif
#ifndef OMIT_TINY_FONT
EpdFont bookerly10RegularFont(&bookerly_10_regular);
EpdFont bookerly10BoldFont(&bookerly_10_bold);
EpdFont bookerly10ItalicFont(&bookerly_10_italic);
EpdFont bookerly10BoldItalicFont(&bookerly_10_bolditalic);
EpdFontFamily bookerly10FontFamily(&bookerly10RegularFont, &bookerly10BoldFont, &bookerly10ItalicFont,
                                  &bookerly10BoldItalicFont);
#endif
#ifndef OMIT_SMALL_FONT
EpdFont bookerly12RegularFont(&bookerly_12_regular);
EpdFont bookerly12BoldFont(&bookerly_12_bold);
EpdFont bookerly12ItalicFont(&bookerly_12_italic);
EpdFont bookerly12BoldItalicFont(&bookerly_12_bolditalic);
EpdFontFamily bookerly12FontFamily(&bookerly12RegularFont, &bookerly12BoldFont, &bookerly12ItalicFont,
                                  &bookerly12BoldItalicFont);
#endif
#ifndef OMIT_MEDIUM_FONT
EpdFont bookerly14RegularFont(&bookerly_14_regular);
EpdFont bookerly14BoldFont(&bookerly_14_bold);
EpdFont bookerly14ItalicFont(&bookerly_14_italic);
EpdFont bookerly14BoldItalicFont(&bookerly_14_bolditalic);
EpdFontFamily bookerly14FontFamily(&bookerly14RegularFont, &bookerly14BoldFont, &bookerly14ItalicFont,
                                  &bookerly14BoldItalicFont);
EpdFont bookerly15RegularFont(&bookerly_15_regular);
EpdFont bookerly15BoldFont(&bookerly_15_bold);
EpdFont bookerly15ItalicFont(&bookerly_15_italic);
EpdFont bookerly15BoldItalicFont(&bookerly_15_bolditalic);
EpdFontFamily bookerly15FontFamily(&bookerly15RegularFont, &bookerly15BoldFont, &bookerly15ItalicFont,
                                  &bookerly15BoldItalicFont);
#endif
#ifndef OMIT_LARGE_FONT
EpdFont bookerly16RegularFont(&bookerly_16_regular);
EpdFont bookerly16BoldFont(&bookerly_16_bold);
EpdFont bookerly16ItalicFont(&bookerly_16_italic);
EpdFont bookerly16BoldItalicFont(&bookerly_16_bolditalic);
EpdFontFamily bookerly16FontFamily(&bookerly16RegularFont, &bookerly16BoldFont, &bookerly16ItalicFont,
                                  &bookerly16BoldItalicFont);
#endif
#ifndef OMIT_XLARGE_FONT
EpdFont bookerly18RegularFont(&bookerly_18_regular);
EpdFont bookerly18BoldFont(&bookerly_18_bold);
EpdFont bookerly18ItalicFont(&bookerly_18_italic);
EpdFont bookerly18BoldItalicFont(&bookerly_18_bolditalic);
EpdFontFamily bookerly18FontFamily(&bookerly18RegularFont, &bookerly18BoldFont, &bookerly18ItalicFont,
                                  &bookerly18BoldItalicFont);
#endif
#ifndef OMIT_HUGE_FONT
EpdFont bookerly20RegularFont(&bookerly_20_regular);
EpdFont bookerly20BoldFont(&bookerly_20_bold);
EpdFont bookerly20ItalicFont(&bookerly_20_italic);
EpdFont bookerly20BoldItalicFont(&bookerly_20_bolditalic);
EpdFontFamily bookerly20FontFamily(&bookerly20RegularFont, &bookerly20BoldFont, &bookerly20ItalicFont,
                                  &bookerly20BoldItalicFont);
#endif
#ifndef OMIT_TEENSY_FONT
EpdFont lexenddeca8RegularFont(&lexenddeca_8_regular);
EpdFont lexenddeca8BoldFont(&lexenddeca_8_bold);
EpdFont lexenddeca8ItalicFont(&lexenddeca_8_italic);
EpdFont lexenddeca8BoldItalicFont(&lexenddeca_8_bolditalic);
EpdFontFamily lexenddeca8FontFamily(&lexenddeca8RegularFont, &lexenddeca8BoldFont, &lexenddeca8ItalicFont,
                                    &lexenddeca8BoldItalicFont);
#endif
#ifndef OMIT_ITTY_BITTY_FONT
EpdFont lexenddeca9RegularFont(&lexenddeca_9_regular);
EpdFont lexenddeca9BoldFont(&lexenddeca_9_bold);
EpdFont lexenddeca9ItalicFont(&lexenddeca_9_italic);
EpdFont lexenddeca9BoldItalicFont(&lexenddeca_9_bolditalic);
EpdFontFamily lexenddeca9FontFamily(&lexenddeca9RegularFont, &lexenddeca9BoldFont, &lexenddeca9ItalicFont,
                                    &lexenddeca9BoldItalicFont);
#endif
#ifndef OMIT_TINY_FONT
EpdFont lexenddeca10RegularFont(&lexenddeca_10_regular);
EpdFont lexenddeca10BoldFont(&lexenddeca_10_bold);
EpdFont lexenddeca10ItalicFont(&lexenddeca_10_italic);
EpdFont lexenddeca10BoldItalicFont(&lexenddeca_10_bolditalic);
EpdFontFamily lexenddeca10FontFamily(&lexenddeca10RegularFont, &lexenddeca10BoldFont, &lexenddeca10ItalicFont,
                                     &lexenddeca10BoldItalicFont);
#endif
#ifndef OMIT_SMALL_FONT
EpdFont lexenddeca12RegularFont(&lexenddeca_12_regular);
EpdFont lexenddeca12BoldFont(&lexenddeca_12_bold);
EpdFont lexenddeca12ItalicFont(&lexenddeca_12_italic);
EpdFont lexenddeca12BoldItalicFont(&lexenddeca_12_bolditalic);
EpdFontFamily lexenddeca12FontFamily(&lexenddeca12RegularFont, &lexenddeca12BoldFont, &lexenddeca12ItalicFont,
                                     &lexenddeca12BoldItalicFont);
#endif
#ifndef OMIT_MEDIUM_FONT
EpdFont lexenddeca14RegularFont(&lexenddeca_14_regular);
EpdFont lexenddeca14BoldFont(&lexenddeca_14_bold);
EpdFont lexenddeca14ItalicFont(&lexenddeca_14_italic);
EpdFont lexenddeca14BoldItalicFont(&lexenddeca_14_bolditalic);
EpdFontFamily lexenddeca14FontFamily(&lexenddeca14RegularFont, &lexenddeca14BoldFont, &lexenddeca14ItalicFont,
                                     &lexenddeca14BoldItalicFont);
EpdFont lexenddeca15RegularFont(&lexenddeca_15_regular);
EpdFont lexenddeca15BoldFont(&lexenddeca_15_bold);
EpdFont lexenddeca15ItalicFont(&lexenddeca_15_italic);
EpdFont lexenddeca15BoldItalicFont(&lexenddeca_15_bolditalic);
EpdFontFamily lexenddeca15FontFamily(&lexenddeca15RegularFont, &lexenddeca15BoldFont, &lexenddeca15ItalicFont,
                                     &lexenddeca15BoldItalicFont);
#endif
#ifndef OMIT_LARGE_FONT
EpdFont lexenddeca16RegularFont(&lexenddeca_16_regular);
EpdFont lexenddeca16BoldFont(&lexenddeca_16_bold);
EpdFont lexenddeca16ItalicFont(&lexenddeca_16_italic);
EpdFont lexenddeca16BoldItalicFont(&lexenddeca_16_bolditalic);
EpdFontFamily lexenddeca16FontFamily(&lexenddeca16RegularFont, &lexenddeca16BoldFont, &lexenddeca16ItalicFont,
                                     &lexenddeca16BoldItalicFont);
#endif
#ifndef OMIT_XLARGE_FONT
EpdFont lexenddeca18RegularFont(&lexenddeca_18_regular);
EpdFont lexenddeca18BoldFont(&lexenddeca_18_bold);
EpdFont lexenddeca18ItalicFont(&lexenddeca_18_italic);
EpdFont lexenddeca18BoldItalicFont(&lexenddeca_18_bolditalic);
EpdFontFamily lexenddeca18FontFamily(&lexenddeca18RegularFont, &lexenddeca18BoldFont, &lexenddeca18ItalicFont,
                                     &lexenddeca18BoldItalicFont);
#endif
#ifndef OMIT_HUGE_FONT
EpdFont lexenddeca20RegularFont(&lexenddeca_20_regular);
EpdFont lexenddeca20BoldFont(&lexenddeca_20_bold);
EpdFont lexenddeca20ItalicFont(&lexenddeca_20_italic);
EpdFont lexenddeca20BoldItalicFont(&lexenddeca_20_bolditalic);
EpdFontFamily lexenddeca20FontFamily(&lexenddeca20RegularFont, &lexenddeca20BoldFont, &lexenddeca20ItalicFont,
                                     &lexenddeca20BoldItalicFont);
#endif

#ifndef OMIT_TEENSY_FONT
EpdFont bookerlam8RegularFont(&bookerlam_8_regular);
EpdFont bookerlam8BoldFont(&bookerlam_8_bold);
EpdFont bookerlam8ItalicFont(&bookerlam_8_italic);
EpdFont bookerlam8BoldItalicFont(&bookerlam_8_bolditalic);
EpdFontFamily bookerlam8FontFamily(&bookerlam8RegularFont, &bookerlam8BoldFont, &bookerlam8ItalicFont, &bookerlam8BoldItalicFont);
#endif
#ifndef OMIT_TINY_FONT
EpdFont bookerlam10RegularFont(&bookerlam_10_regular);
EpdFont bookerlam10BoldFont(&bookerlam_10_bold);
EpdFont bookerlam10ItalicFont(&bookerlam_10_italic);
EpdFont bookerlam10BoldItalicFont(&bookerlam_10_bolditalic);
EpdFontFamily bookerlam10FontFamily(&bookerlam10RegularFont, &bookerlam10BoldFont, &bookerlam10ItalicFont, &bookerlam10BoldItalicFont);
#endif
#ifndef OMIT_SMALL_FONT
EpdFont bookerlam12RegularFont(&bookerlam_12_regular);
EpdFont bookerlam12BoldFont(&bookerlam_12_bold);
EpdFont bookerlam12ItalicFont(&bookerlam_12_italic);
EpdFont bookerlam12BoldItalicFont(&bookerlam_12_bolditalic);
EpdFontFamily bookerlam12FontFamily(&bookerlam12RegularFont, &bookerlam12BoldFont, &bookerlam12ItalicFont, &bookerlam12BoldItalicFont);
#endif
#ifndef OMIT_MEDIUM_FONT
EpdFont bookerlam14RegularFont(&bookerlam_14_regular);
EpdFont bookerlam14BoldFont(&bookerlam_14_bold);
EpdFont bookerlam14ItalicFont(&bookerlam_14_italic);
EpdFont bookerlam14BoldItalicFont(&bookerlam_14_bolditalic);
EpdFontFamily bookerlam14FontFamily(&bookerlam14RegularFont, &bookerlam14BoldFont, &bookerlam14ItalicFont, &bookerlam14BoldItalicFont);
EpdFont bookerlam15RegularFont(&bookerlam_15_regular);
EpdFont bookerlam15BoldFont(&bookerlam_15_bold);
EpdFont bookerlam15ItalicFont(&bookerlam_15_italic);
EpdFont bookerlam15BoldItalicFont(&bookerlam_15_bolditalic);
EpdFontFamily bookerlam15FontFamily(&bookerlam15RegularFont, &bookerlam15BoldFont, &bookerlam15ItalicFont, &bookerlam15BoldItalicFont);
#endif
#ifndef OMIT_LARGE_FONT
EpdFont bookerlam16RegularFont(&bookerlam_16_regular);
EpdFont bookerlam16BoldFont(&bookerlam_16_bold);
EpdFont bookerlam16ItalicFont(&bookerlam_16_italic);
EpdFont bookerlam16BoldItalicFont(&bookerlam_16_bolditalic);
EpdFontFamily bookerlam16FontFamily(&bookerlam16RegularFont, &bookerlam16BoldFont, &bookerlam16ItalicFont, &bookerlam16BoldItalicFont);
#endif
#ifndef OMIT_XLARGE_FONT
EpdFont bookerlam18RegularFont(&bookerlam_18_regular);
EpdFont bookerlam18BoldFont(&bookerlam_18_bold);
EpdFont bookerlam18ItalicFont(&bookerlam_18_italic);
EpdFont bookerlam18BoldItalicFont(&bookerlam_18_bolditalic);
EpdFontFamily bookerlam18FontFamily(&bookerlam18RegularFont, &bookerlam18BoldFont, &bookerlam18ItalicFont, &bookerlam18BoldItalicFont);
#endif
#ifndef OMIT_HUGE_FONT
EpdFont bookerlam20RegularFont(&bookerlam_20_regular);
EpdFont bookerlam20BoldFont(&bookerlam_20_bold);
EpdFont bookerlam20ItalicFont(&bookerlam_20_italic);
EpdFont bookerlam20BoldItalicFont(&bookerlam_20_bolditalic);
EpdFontFamily bookerlam20FontFamily(&bookerlam20RegularFont, &bookerlam20BoldFont, &bookerlam20ItalicFont, &bookerlam20BoldItalicFont);
#endif

EpdFont smallFont(&inter_8_regular);
EpdFontFamily smallFontFamily(&smallFont);

EpdFont ui10RegularFont(&inter_10_regular);
EpdFont ui10BoldFont(&inter_10_bold);
EpdFontFamily ui10FontFamily(&ui10RegularFont, &ui10BoldFont);

EpdFont ui12RegularFont(&inter_12_regular);
EpdFont ui12BoldFont(&inter_12_bold);
EpdFontFamily ui12FontFamily(&ui12RegularFont, &ui12BoldFont);

// measurement of power button press duration calibration value
unsigned long t1 = 0;
unsigned long t2 = 0;

// Set when the screenshot combo (Power + Volume Down) fires, so the subsequent
// power button release does not also trigger a short-press action (e.g. sleep).
static bool screenshotComboHandled = false;

const char* resetReasonName(const esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:
      return "POWERON";
    case ESP_RST_EXT:
      return "EXT";
    case ESP_RST_SW:
      return "SW";
    case ESP_RST_PANIC:
      return "PANIC";
    case ESP_RST_INT_WDT:
      return "INT_WDT";
    case ESP_RST_TASK_WDT:
      return "TASK_WDT";
    case ESP_RST_WDT:
      return "WDT";
    case ESP_RST_DEEPSLEEP:
      return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:
      return "BROWNOUT";
    case ESP_RST_SDIO:
      return "SDIO";
    case ESP_RST_USB:
      return "USB";
    case ESP_RST_JTAG:
      return "JTAG";
    case ESP_RST_EFUSE:
      return "EFUSE";
    case ESP_RST_PWR_GLITCH:
      return "PWR_GLITCH";
    case ESP_RST_CPU_LOCKUP:
      return "CPU_LOCKUP";
    case ESP_RST_UNKNOWN:
    default:
      return "UNKNOWN";
  }
}

const char* wakeupCauseName(const esp_sleep_wakeup_cause_t cause) {
  switch (cause) {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
      return "UNDEFINED";
    case ESP_SLEEP_WAKEUP_ALL:
      return "ALL";
    case ESP_SLEEP_WAKEUP_EXT0:
      return "EXT0";
    case ESP_SLEEP_WAKEUP_EXT1:
      return "EXT1";
    case ESP_SLEEP_WAKEUP_TIMER:
      return "TIMER";
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      return "TOUCHPAD";
    case ESP_SLEEP_WAKEUP_ULP:
      return "ULP";
    case ESP_SLEEP_WAKEUP_GPIO:
      return "GPIO";
    case ESP_SLEEP_WAKEUP_UART:
      return "UART";
    case ESP_SLEEP_WAKEUP_WIFI:
      return "WIFI";
    case ESP_SLEEP_WAKEUP_COCPU:
      return "COCPU";
    case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
      return "COCPU_TRAP";
    case ESP_SLEEP_WAKEUP_BT:
      return "BT";
    default:
      return "UNKNOWN";
  }
}

const char* wakeupRouteName(const HalGPIO::WakeupReason reason) {
  switch (reason) {
    case HalGPIO::WakeupReason::PowerButton:
      return "PowerButton";
    case HalGPIO::WakeupReason::AfterFlash:
      return "AfterFlash";
    case HalGPIO::WakeupReason::AfterUSBPower:
      return "AfterUSBPower";
    case HalGPIO::WakeupReason::Other:
    default:
      return "Other";
  }
}

// Definitions for SilentRestart.h. RTC_NOINIT survives ESP.restart() but not power loss.
RTC_NOINIT_ATTR uint32_t silentRebootMagic;
RTC_NOINIT_ATTR uint32_t silentRebootTarget;
constexpr uint32_t SILENT_REBOOT_MAGIC = 0xC1EAB007;
constexpr uint32_t SILENT_REBOOT_TARGET_HOME = 0;
constexpr uint32_t SILENT_REBOOT_TARGET_READER = 1;

// How the device is coming back to life, resolved once at boot. Both resume
// flows suppress the splash and leave the panel holding its pre-boot frame; a
// plain boot shows the splash. See setup() for the resolution.
enum class BootResume : uint8_t {
  Splash,       // cold boot, flash, panic, or plain reboot
  Silent,       // heap-defrag ESP.restart() (RTC flag; lost on power loss)
  QuickResume,  // wake from a quick-resume deep sleep (SD flag; survives power loss)
};

// Latched true once enterDeepSleep() commits to sleeping, before it tears down
// the current activity. WiFi activities call silentRestart() in onExit() to
// clear heap fragmentation on the way out, but deep sleep is a full chip reset
// on wake and already clears the heap, so rebooting here would just power the
// device back up against the user's sleep gesture. Never cleared:
// startDeepSleep() does not return, so a set latch only ends at the wakeup reset.
static bool deepSleepInProgress = false;

void silentRestart() {
  if (deepSleepInProgress) return;  // sleeping supersedes the heap-defrag reboot
  silentRebootTarget = SILENT_REBOOT_TARGET_HOME;
  silentRebootMagic = SILENT_REBOOT_MAGIC;
  LOG_DBG("MAIN", "Silent restart (target=home)");
  // E-ink retains the previous frame until Home's first paint lands (~2-3s).
  // Without an overlay, users don't see the reboot and fire input through to
  // Home. Select on the default selectorIndex=0 then opens the most-recent
  // book, looking like a trampoline back to the reader they just exited.
  GUI.drawPopup(renderer, tr(STR_LOADING_POPUP));
  delay(50);
  ESP.restart();
}

void silentRestartToReader() {
  if (deepSleepInProgress) return;  // sleeping supersedes the heap-defrag reboot
  silentRebootTarget = SILENT_REBOOT_TARGET_READER;
  silentRebootMagic = SILENT_REBOOT_MAGIC;
  LOG_DBG("MAIN", "Silent restart (target=reader)");
  GUI.drawPopup(renderer, tr(STR_LOADING_POPUP));
  delay(50);
  ESP.restart();
}

void waitForPowerRelease() {
  gpio.update();
  while (gpio.isPressed(HalGPIO::BTN_POWER)) {
    delay(50);
    gpio.update();
  }
}

bool isGlobalPowerButtonAction(const CrossPointSettings::SHORT_PWRBTN action) {
  return isPowerButtonActionAvailableOutsideReader(action);
}

bool startGlobalSyncProgress() {
  if (!KOREADER_STORE.hasCredentials()) {
    activityManager.pushActivity(std::make_unique<KOReaderSettingsActivity>(renderer, mappedInputManager));
    return true;
  }

  const std::string epubPath = APP_STATE.openEpubPath;
  if (epubPath.empty() || !FsHelpers::hasEpubExtension(epubPath) || !Storage.exists(epubPath.c_str())) {
    LOG_DBG("MAIN", "No syncable EPUB open, opening KOReader settings instead");
    activityManager.pushActivity(std::make_unique<KOReaderSettingsActivity>(renderer, mappedInputManager));
    return true;
  }

  auto epub = std::make_shared<Epub>(epubPath, "/.crosspoint");
  if (!epub->load(true, SETTINGS.embeddedStyle == 0)) {
    LOG_ERR("MAIN", "Failed to load EPUB for global sync: %s", epubPath.c_str());
    activityManager.pushActivity(std::make_unique<KOReaderSettingsActivity>(renderer, mappedInputManager));
    return true;
  }

  epub->setupCacheDir();

  int spineIndex = 0;
  int pageNumber = 0;
  int totalPagesInSpine = 1;
  FsFile progressFile;
  if (Storage.openFileForRead("MAIN", epub->getCachePath() + "/progress.bin", progressFile)) {
    uint8_t data[6];
    const int dataSize = progressFile.read(data, sizeof(data));
    if (dataSize >= 4) {
      spineIndex = data[0] | (data[1] << 8);
      pageNumber = data[2] | (data[3] << 8);
      if (pageNumber == UINT16_MAX) {
        pageNumber = 0;
      }
    }
    if (dataSize >= 6) {
      totalPagesInSpine = std::max(1, static_cast<int>(data[4] | (data[5] << 8)));
    }
    progressFile.close();
  }

  if (spineIndex < 0 || spineIndex >= epub->getSpineItemsCount()) {
    spineIndex = 0;
  }

  CrossPointPosition localPos = {spineIndex, pageNumber, totalPagesInSpine};
  KOReaderPosition localKoPos = ProgressMapper::toKOReader(epub, localPos);
  const int tocIdx = epub->getTocIndexForSpineIndex(spineIndex);
  std::string localChapterName = (tocIdx >= 0) ? epub->getTocItem(tocIdx).title : "";

  activityManager.pushActivity(
      std::make_unique<KOReaderSyncActivity>(renderer, mappedInputManager, epubPath, spineIndex, pageNumber,
                                             totalPagesInSpine, std::move(localKoPos), std::move(localChapterName)));
  return true;
}

CrossPointSettings::SHORT_PWRBTN getPowerButtonAction() {
  static bool longPowerButtonHandled = false;

  if (mappedInputManager.wasReleased(MappedInputManager::Button::Power)) {
    if (longPowerButtonHandled) {
      longPowerButtonHandled = false;
      screenshotComboHandled = false;
      return CrossPointSettings::SHORT_PWRBTN::IGNORE;
    }

    if (screenshotComboHandled) {
      screenshotComboHandled = false;
      return CrossPointSettings::SHORT_PWRBTN::IGNORE;
    }

    return mappedInputManager.getHeldTime() < SETTINGS.getPowerButtonLongPressDuration()
               ? static_cast<CrossPointSettings::SHORT_PWRBTN>(SETTINGS.shortPwrBtn)
               : static_cast<CrossPointSettings::SHORT_PWRBTN>(SETTINGS.longPwrBtn);
  }

  if (longPowerButtonHandled || !mappedInputManager.isPressed(MappedInputManager::Button::Power) ||
      mappedInputManager.getHeldTime() < SETTINGS.getPowerButtonLongPressDuration()) {
    return CrossPointSettings::SHORT_PWRBTN::IGNORE;
  }

  const auto action = static_cast<CrossPointSettings::SHORT_PWRBTN>(SETTINGS.longPwrBtn);
  if (!isGlobalPowerButtonAction(action)) {
    return CrossPointSettings::SHORT_PWRBTN::IGNORE;
  }

  longPowerButtonHandled = true;
  return action;
}

bool handleGlobalPowerButtonAction(const CrossPointSettings::SHORT_PWRBTN action) {
  switch (action) {
    case CrossPointSettings::SHORT_PWRBTN::SLEEP:
      enterDeepSleep();
      return true;
    case CrossPointSettings::SHORT_PWRBTN::FORCE_REFRESH: {
      LOG_DBG("MAIN", "Manual screen refresh triggered");
      RenderLock lock;
      renderer.displayBuffer(HalDisplay::HALF_REFRESH);
      return true;
    }
    case CrossPointSettings::SHORT_PWRBTN::SCREENSHOT: {
      if (activityManager.canSnapshotForSleepOverlay()) {
        return false;
      }
      RenderLock lock;
      ScreenshotUtil::takeScreenshot(renderer);
      return true;
    }
    case CrossPointSettings::SHORT_PWRBTN::SYNC_PROGRESS:
      if (activityManager.canSnapshotForSleepOverlay()) {
        return false;
      }
      return startGlobalSyncProgress();
    case CrossPointSettings::SHORT_PWRBTN::FILE_TRANSFER:
      if (activityManager.canSnapshotForSleepOverlay()) {
        return false;
      }
      activityManager.goToFileTransfer();
      return true;
    default:
      return false;
  }
}

namespace {
constexpr uint16_t POST_SLEEP_SCREEN_SETTLE_MS = 500;
constexpr uint8_t TILT_SLEEP_MAX_ATTEMPTS = 3;
constexpr uint16_t TILT_SLEEP_RETRY_DELAY_MS = 10;

void putTiltSensorToSleepForDeepSleep() {
  if (!halTiltSensor.isAvailable()) {
    return;
  }

  for (uint8_t attempt = 0; attempt < TILT_SLEEP_MAX_ATTEMPTS; ++attempt) {
    if (halTiltSensor.deepSleep()) {
      return;
    }
    delay(TILT_SLEEP_RETRY_DELAY_MS);
  }
  LOG_ERR("MAIN", "Tilt sensor did not confirm sleep before deep sleep");
}
}  // namespace

constexpr char SLEEP_FRAME_FILE[] = "/.crosspoint/sleep_frame.bin";

static void saveSleepFrameBuffer() {
  FsFile file;
  if (!Storage.openFileForWrite("SLP", SLEEP_FRAME_FILE, file)) return;
  file.write(renderer.getFrameBuffer(), renderer.getBufferSize());
  file.close();
}

static bool loadSleepFrameBuffer() {
  FsFile file;
  if (!Storage.openFileForRead("SLP", SLEEP_FRAME_FILE, file)) return false;
  const size_t bufferSize = display.getBufferSize();
  const size_t bytesRead = file.read(display.getFrameBuffer(), bufferSize);
  file.close();
  if (bytesRead != bufferSize) {
    Storage.remove(SLEEP_FRAME_FILE);
    return false;
  }
  Storage.remove(SLEEP_FRAME_FILE);
  return true;
}

// Enter deep sleep mode
void enterDeepSleep(bool fromTimeout) {
  HalPowerManager::Lock powerLock;  // Ensure we are at normal CPU frequency for sleep preparation
  APP_STATE.lastSleepFromReader = activityManager.isReaderActivity();

  const bool isQuickResumeSleep =
      SETTINGS.sleepScreen == CrossPointSettings::SLEEP_SCREEN_MODE::QUICK_RESUME ||
      (fromTimeout &&
       SETTINGS.quickResumeSleepScreen == CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_AFTER_TIMEOUT);
  APP_STATE.showBootScreen = !isQuickResumeSleep;

  APP_STATE.saveToFile();

  // Commit to sleeping before goToSleep() runs the outgoing activity's onExit():
  // a WiFi activity would otherwise silentRestart() here and reboot instead.
  deepSleepInProgress = true;
  activityManager.goToSleep(fromTimeout);

  if (isQuickResumeSleep) {
    saveSleepFrameBuffer();
  } else {
    delay(POST_SLEEP_SCREEN_SETTLE_MS);
  }

  putTiltSensorToSleepForDeepSleep();
  display.deepSleep();
  LOG_DBG("MAIN", "Entering deep sleep");

  powerManager.startDeepSleep(gpio);
}

void setupDisplayAndFonts(bool seamless = false) {
#ifdef SIMULATOR
  (void)seamless;
  display.begin();
#else
  display.begin(seamless);
#endif
  renderer.begin();
  activityManager.begin();
  LOG_DBG("MAIN", "Display initialized");

  // Initialize font decompressor for compressed reader fonts
  if (!fontDecompressor.init()) {
    LOG_ERR("MAIN", "Font decompressor init failed");
  }
  fontCacheManager.setFontDecompressor(&fontDecompressor);
  renderer.setFontCacheManager(&fontCacheManager);

#ifndef OMIT_TEENSY_FONT
  renderer.insertFont(BOOKERLY_8_FONT_ID, bookerly8FontFamily);
#endif
#ifndef OMIT_TINY_FONT
  renderer.insertFont(BOOKERLY_10_FONT_ID, bookerly10FontFamily);
#endif
#ifndef OMIT_SMALL_FONT
  renderer.insertFont(BOOKERLY_12_FONT_ID, bookerly12FontFamily);
#endif
#ifndef OMIT_MEDIUM_FONT
  renderer.insertFont(BOOKERLY_14_FONT_ID, bookerly14FontFamily);
  renderer.insertFont(BOOKERLY_15_FONT_ID, bookerly15FontFamily);
#endif
#ifndef OMIT_LARGE_FONT
  renderer.insertFont(BOOKERLY_16_FONT_ID, bookerly16FontFamily);
#endif
#ifndef OMIT_XLARGE_FONT
  renderer.insertFont(BOOKERLY_18_FONT_ID, bookerly18FontFamily);
#endif
#ifndef OMIT_HUGE_FONT
  renderer.insertFont(BOOKERLY_20_FONT_ID, bookerly20FontFamily);
#endif

#ifndef OMIT_TEENSY_FONT
  renderer.insertFont(LEXENDDECA_8_FONT_ID, lexenddeca8FontFamily);
#endif
#ifndef OMIT_TINY_FONT
  renderer.insertFont(LEXENDDECA_10_FONT_ID, lexenddeca10FontFamily);
#endif
#ifndef OMIT_SMALL_FONT
  renderer.insertFont(LEXENDDECA_12_FONT_ID, lexenddeca12FontFamily);
#endif
#ifndef OMIT_MEDIUM_FONT
  renderer.insertFont(LEXENDDECA_14_FONT_ID, lexenddeca14FontFamily);
  renderer.insertFont(LEXENDDECA_15_FONT_ID, lexenddeca15FontFamily);
#endif
#ifndef OMIT_LARGE_FONT
  renderer.insertFont(LEXENDDECA_16_FONT_ID, lexenddeca16FontFamily);
#endif
#ifndef OMIT_XLARGE_FONT
  renderer.insertFont(LEXENDDECA_18_FONT_ID, lexenddeca18FontFamily);
#endif
#ifndef OMIT_HUGE_FONT
  renderer.insertFont(LEXENDDECA_20_FONT_ID, lexenddeca20FontFamily);
#endif

#ifndef OMIT_TEENSY_FONT
  renderer.insertFont(BOOKERLAM_8_FONT_ID, bookerlam8FontFamily);
#endif
#ifndef OMIT_TINY_FONT
  renderer.insertFont(BOOKERLAM_10_FONT_ID, bookerlam10FontFamily);
#endif
#ifndef OMIT_SMALL_FONT
  renderer.insertFont(BOOKERLAM_12_FONT_ID, bookerlam12FontFamily);
#endif
#ifndef OMIT_MEDIUM_FONT
  renderer.insertFont(BOOKERLAM_14_FONT_ID, bookerlam14FontFamily);
  renderer.insertFont(BOOKERLAM_15_FONT_ID, bookerlam15FontFamily);
#endif
#ifndef OMIT_LARGE_FONT
  renderer.insertFont(BOOKERLAM_16_FONT_ID, bookerlam16FontFamily);
#endif
#ifndef OMIT_XLARGE_FONT
  renderer.insertFont(BOOKERLAM_18_FONT_ID, bookerlam18FontFamily);
#endif
#ifndef OMIT_HUGE_FONT
  renderer.insertFont(BOOKERLAM_20_FONT_ID, bookerlam20FontFamily);
#endif
  renderer.insertFont(UI_10_FONT_ID, ui10FontFamily);
  renderer.insertFont(UI_12_FONT_ID, ui12FontFamily);
  renderer.insertFont(SMALL_FONT_ID, smallFontFamily);

  // Discover and load SD card fonts
  sdFontSystem.begin(renderer);

  LOG_DBG("MAIN", "Fonts setup");
}

void setup() {
  t1 = millis();

  const esp_reset_reason_t rawResetReason = esp_reset_reason();
  const esp_sleep_wakeup_cause_t rawWakeupCause = esp_sleep_get_wakeup_cause();

#ifdef ENABLE_SERIAL_LOG
  // Earliest possible Serial setup. The 250 ms stall before begin() lets the
  // USB Serial/JTAG peripheral finish power-on and lets the host complete USB
  // enumeration before we touch the CDC state — otherwise cold boot races
  // and the host has to be physically replugged for logs to flow. Warm reboot
  // worked without the delay because USB was already enumerated.
  delay(250);
  Serial.begin(115200);
#ifndef SIMULATOR
  logSerial.setTxTimeoutMs(1);  // This is a load-bearing 1. Do not modify.
#endif
#endif

  HalSystem::begin();
  LOG_INF("BOOT", "Reset diagnostic: reset=%d(%s) sleepWake=%d(%s)", static_cast<int>(rawResetReason),
          resetReasonName(rawResetReason), static_cast<int>(rawWakeupCause), wakeupCauseName(rawWakeupCause));

  // Read-and-clear so a panic later in setup() doesn't loop into silent reboot.
  // Bound the target range too — RTC_NOINIT memory is uninitialized on cold boot.
  const bool isSilentReboot = (silentRebootMagic == SILENT_REBOOT_MAGIC);
  const uint32_t snapshotTarget =
      (isSilentReboot && silentRebootTarget <= SILENT_REBOOT_TARGET_READER) ? silentRebootTarget : 0;
  silentRebootMagic = 0;
  silentRebootTarget = 0;

  gpio.begin();
  powerManager.begin();
  halTiltSensor.begin();
  halClock.begin();

  LOG_INF("MAIN", "Hardware detect: %s", gpio.deviceIsX3() ? "X3" : "X4");
  LOG_INF("BOOT", "Post-GPIO diagnostic: device=%s usb=%d silentReboot=%d silentTarget=%lu",
          gpio.deviceIsX3() ? "X3" : "X4", gpio.isUsbConnected() ? 1 : 0, isSilentReboot ? 1 : 0,
          static_cast<unsigned long>(snapshotTarget));

  // SD Card Initialization
  // We need 6 open files concurrently when parsing a new chapter
  if (!Storage.begin()) {
    LOG_ERR("MAIN", "SD card initialization failed");
    setupDisplayAndFonts(isSilentReboot);
    activityManager.goToFullScreenMessage("SD card error", EpdFontFamily::BOLD);
    return;
  }

  HalSystem::checkPanic();

  SETTINGS.loadFromFile();
  APP_STATE.loadFromFile();
  RECENT_BOOKS.loadFromFile();
  I18N.setLanguage(static_cast<Language>(SETTINGS.language));
  KOREADER_STORE.loadFromFile();
  OPDS_STORE.loadFromFile();
  UITheme::getInstance().reload();
  ButtonNavigator::setMappedInputManager(mappedInputManager);

  const auto wakeupReason = gpio.getWakeupReason();
  LOG_INF("BOOT", "Wake route: %s", wakeupRouteName(wakeupReason));
  switch (wakeupReason) {
    case HalGPIO::WakeupReason::PowerButton:
      LOG_INF("BOOT", "Power-button wake: verifying duration required=%u shortAllowed=%d",
              SETTINGS.getPowerButtonWakeDuration(), SETTINGS.shortPwrBtn == CrossPointSettings::SHORT_PWRBTN::SLEEP);
      gpio.verifyPowerButtonWakeup(SETTINGS.getPowerButtonWakeDuration(),
                                   SETTINGS.shortPwrBtn == CrossPointSettings::SHORT_PWRBTN::SLEEP);
      break;
    case HalGPIO::WakeupReason::AfterUSBPower:
      // TEMP: continue booting while diagnosing post-flash/reset behavior.
      // Normal behavior is to go back to sleep when USB power causes a cold boot.
      LOG_INF("BOOT", "AfterUSBPower route: TEMP continuing boot instead of deep sleep");
      break;
    case HalGPIO::WakeupReason::AfterFlash:
      // After flashing, just proceed to boot
      LOG_INF("BOOT", "AfterFlash route: continuing boot");
      break;
    case HalGPIO::WakeupReason::Other:
    default:
      LOG_INF("BOOT", "Other wake route: continuing boot");
      break;
  }

  // Recovery firmware mode: hold left side button (BTN_UP) together with the power button at
  // boot to skip directly to the SD-card firmware update screen. Useful on devices where USB
  // flashing has been locked down (e.g. recent X3 firmware).
  bool recoveryFirmwareMode = false;
  if (wakeupReason == HalGPIO::WakeupReason::PowerButton) {
    // Refresh the cached button state a few times — isPressed() needs ~half a second to settle
    // after boot per the HalGPIO contract. Use a millis-based deadline so we always wait the full
    // settle window even if the loop body takes longer than expected on slow boots.
    const unsigned long settleStart = millis();
    while (millis() - settleStart < 500) {
      gpio.update();
      delay(10);
    }
    if (gpio.isPressed(HalGPIO::BTN_UP)) {
      recoveryFirmwareMode = true;
      LOG_INF("MAIN", "Recovery firmware mode (UP + POWER held at boot)");
    }
  }

  // First serial output only here to avoid timing inconsistencies for power button press duration verification
  LOG_DBG("MAIN", "Starting CrossPoint version " CROSSPOINT_VERSION);

  // Resolve the single boot-presentation decision. Skipping the splash also
  // skips the panel-clearing pass and the X3 initial-full-sync arming (see
  // HalDisplay::begin), so the first paint is FAST_REFRESH (~500ms) over the
  // retained frame and input dispatches against a visible UI.
  const BootResume resume = isSilentReboot              ? BootResume::Silent
                            : !APP_STATE.showBootScreen ? BootResume::QuickResume
                                                        : BootResume::Splash;

  setupDisplayAndFonts(resume != BootResume::Splash);

  switch (resume) {
    case BootResume::Silent:
      // Splash skipped: the routing block below picks the target activity; the
      // panel keeps showing the pre-reboot popup until that first paint lands.
      break;
    case BootResume::QuickResume:
      // One-shot flag: re-arm the splash for the next non-quick-resume boot. Save
      // before any painting so a hang in the blocking paint path can't strand
      // us in a quick-resume-with-no-frame loop on the next boot.
      APP_STATE.showBootScreen = true;
      APP_STATE.saveToFile();
      if (loadSleepFrameBuffer()) {
        // Frame restored: swap the sleep moon for the loading icon.
        const auto pageHeight = renderer.getScreenHeight();
        renderer.drawImage(LoadingIcon, 0, pageHeight - LOADINGICON_HEIGHT, LOADINGICON_WIDTH, LOADINGICON_HEIGHT);
        renderer.displayBuffer(HalDisplay::HALF_REFRESH);
      } else {
        activityManager.goToBoot();  // frame file missing, fall back to the splash
      }
      break;
    case BootResume::Splash:
      activityManager.goToBoot();
      break;
  }

  if (recoveryFirmwareMode) {
    // Skip normal home/reader routing: jump straight into the SD firmware picker.
    activityManager.replaceActivity(
        std::make_unique<SdFirmwareUpdateActivity>(renderer, mappedInputManager, /*recoveryMode=*/true));
  } else if (HalSystem::isRebootFromPanic()) {
    // If we rebooted from a panic, go to crash report screen to show the panic info
    activityManager.goToCrashReport();
  } else if (resume == BootResume::Silent && snapshotTarget == SILENT_REBOOT_TARGET_READER &&
             !APP_STATE.openEpubPath.empty()) {
    activityManager.goToReader(APP_STATE.openEpubPath);
  } else if (resume == BootResume::Silent) {
    // target == home (or reader with no open book): land on home — don't fall
    // through to the sleep-wake "resume reader" logic, which fires on stale
    // openEpubPath + lastSleepFromReader from a prior session.
    activityManager.goHome();
  } else if (APP_STATE.openEpubPath.empty() || !APP_STATE.lastSleepFromReader ||
             mappedInputManager.isPressed(MappedInputManager::Button::Back) || APP_STATE.readerActivityLoadCount > 0) {
    // Boot to home screen if no book is open, last sleep was not from reader, back button is held, or reader activity
    // crashed (indicated by readerActivityLoadCount > 0)
    activityManager.goHome();
  } else {
    // Clear app state to avoid getting into a boot loop if the epub doesn't load
    const auto path = APP_STATE.openEpubPath;
    APP_STATE.openEpubPath = "";
    APP_STATE.readerActivityLoadCount++;
    APP_STATE.saveToFile();
    activityManager.goToReader(path);
  }

  if (resume == BootResume::Silent) {
    // Block until the first paint physically completes. refreshDisplay()
    // waits on the panel BUSY pin so when this returns the user can see the
    // new activity. Without the wait, an edge captured by gpio.update()
    // during boot dispatches against an invisible Home and the default
    // selectorIndex=0 opens the most-recent book.
    activityManager.requestUpdateAndWait();
    // Absorb any button held at this point into currentState as a non-edge:
    // two gpio.update() calls separated by > InputManager's 5ms debounce
    // transition the held bit through lastDebounceTime into currentState
    // without setting pressedEvents, so the first loop()'s own gpio.update()
    // sees state == currentState and emits nothing.
    gpio.update();
    delay(10);
    gpio.update();
  }

  // Ensure we're not still holding the power button before leaving setup
  waitForPowerRelease();
  allowSleepAt = millis() + 2000;
}

void loop() {
  static unsigned long maxLoopDuration = 0;
  const unsigned long loopStartTime = millis();
  static unsigned long lastMemPrint = 0;

  gpio.update();
  halTiltSensor.update(SETTINGS.tiltPageTurn, SETTINGS.orientation, activityManager.isReaderActivity());

  renderer.setFadingFix(SETTINGS.fadingFix);

  if (Serial && millis() - lastMemPrint >= 10000) {
    LOG_INF("MEM", "Free: %d bytes, Total: %d bytes, Min Free: %d bytes, MaxAlloc: %d bytes", ESP.getFreeHeap(),
            ESP.getHeapSize(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    lastMemPrint = millis();
  }

  // Handle incoming serial commands,
  // nb: we use logSerial from logging to avoid deprecation warnings
  if (logSerial.available() > 0) {
    String line = logSerial.readStringUntil('\n');
    if (line.startsWith("CMD:")) {
      String cmd = line.substring(4);
      cmd.trim();
      if (cmd == "SCREENSHOT") {
        const uint32_t bufferSize = display.getBufferSize();
        logSerial.printf("SCREENSHOT_START:%d\n", bufferSize);
        uint8_t* buf = display.getFrameBuffer();
        logSerial.write(buf, bufferSize);
        logSerial.printf("SCREENSHOT_END\n");
      }
    }
  }

  // Check for any user activity (button press or release) or active background work
  static unsigned long lastActivityTime = millis();
  if (gpio.wasAnyPressed() || gpio.wasAnyReleased() || halTiltSensor.hadActivity() ||
      activityManager.preventAutoSleep()) {
    lastActivityTime = millis();         // Reset inactivity timer
    powerManager.setPowerSaving(false);  // Restore normal CPU frequency on user activity
  }

  static bool screenshotButtonsReleased = true;
  static bool screenshotComboActive = false;
  if (gpio.isPressed(HalGPIO::BTN_POWER) && gpio.isPressed(HalGPIO::BTN_DOWN)) {
    screenshotComboActive = true;
    if (screenshotButtonsReleased) {
      screenshotButtonsReleased = false;
      screenshotComboHandled = true;
      mappedInputManager.suppressNextPowerConfirmRelease();
      {
        RenderLock lock;
        ScreenshotUtil::takeScreenshot(renderer);
      }
    }
    return;
  }
  if (screenshotComboActive) {
    if (gpio.isPressed(HalGPIO::BTN_POWER)) return;
    if (gpio.wasReleased(HalGPIO::BTN_POWER)) {
      screenshotButtonsReleased = true;
      screenshotComboActive = false;
      return;
    }
    screenshotButtonsReleased = true;
    screenshotComboActive = false;
  }

#ifdef SIMULATOR
  if (gpio.consumeSimulatorSleepRequest()) {
    enterDeepSleep();
    lastActivityTime = millis();
    return;
  }
#endif

  const unsigned long sleepTimeoutMs = SETTINGS.getSleepTimeoutMs();
  if (millis() - lastActivityTime >= sleepTimeoutMs) {
    LOG_DBG("SLP", "Auto-sleep triggered after %lu ms of inactivity", sleepTimeoutMs);
    enterDeepSleep(true);
    // This should never be hit as `enterDeepSleep` calls esp_deep_sleep_start
    // In the simulator, deep sleep is a no-op and returns — reset the timer so
    // the main loop does not immediately re-trigger auto-sleep.
    lastActivityTime = millis();
    return;
  }

  if (millis() >= allowSleepAt && handleGlobalPowerButtonAction(getPowerButtonAction())) {
    lastActivityTime = millis();
    return;
  }

  // Refresh the battery icon when USB is plugged or unplugged.
  // Placed after sleep guards so we never queue a render that won't be processed.
  if (gpio.wasUsbStateChanged()) {
    activityManager.requestUpdate();
  }

  const unsigned long activityStartTime = millis();
  activityManager.loop();
  const unsigned long activityDuration = millis() - activityStartTime;

#ifdef SIMULATOR
  runSimulatorSmokeTestTick();
#endif

  const unsigned long loopDuration = millis() - loopStartTime;
  if (loopDuration > maxLoopDuration) {
    maxLoopDuration = loopDuration;
    if (maxLoopDuration > 50) {
      LOG_DBG("LOOP", "New max loop duration: %lu ms (activity: %lu ms)", maxLoopDuration, activityDuration);
      (void)activityDuration;
    }
  }

  // Add delay at the end of the loop to prevent tight spinning
  // When an activity requests skip loop delay (e.g., webserver running), use yield() for faster response
  // Otherwise, use longer delay to save power
  if (activityManager.skipLoopDelay()) {
    powerManager.setPowerSaving(false);  // Make sure we're at full performance when skipLoopDelay is requested
    yield();                             // Give FreeRTOS a chance to run tasks, but return immediately
  } else {
    if (millis() - lastActivityTime >= HalPowerManager::IDLE_POWER_SAVING_MS) {
      // If we've been inactive for a while, increase the delay to save power
      powerManager.setPowerSaving(true);  // Lower CPU frequency after extended inactivity
      delay(50);
    } else {
      // Short delay to prevent tight loop while still being responsive
      delay(10);
    }
  }
}

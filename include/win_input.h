#ifndef TFF_WIN_INPUT_H
#define TFF_WIN_INPUT_H

/**
 * @file win_input.h
 * @brief Platform-neutral part of the Windows port.
 *
 * Translates between the engine's evdev key codes and Windows keyboard data (set-1 scan codes,
 * virtual-key codes, WH_KEYBOARD_LL hook flags), decides which hook events TFF swallows, and
 * batches engine output into abstract SendInput records.
 *
 * Nothing in here includes <windows.h>, so the logic is unit tested on every platform
 * (tests/test_win_input.cpp). The Win32 glue lives in src/platform/windows/windows_platform.cpp.
 */

#include "tff_types.h"
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tff {
namespace win {

/// dwExtraInfo tag on every event TFF injects ("TFF2"); the hook passes these through.
constexpr uintptr_t kInjectedMagic = 0x54464632;

/// Subset of the KBDLLHOOKSTRUCT::flags bits (LLKHF_*), duplicated to stay <windows.h>-free.
constexpr uint32_t kLlkhfExtended = 0x01;
constexpr uint32_t kLlkhfLowerIlInjected = 0x02;
constexpr uint32_t kLlkhfInjected = 0x10;
constexpr uint32_t kLlkhfUp = 0x80;

/// Scan code Windows reports for the fake LCtrl it synthesizes in front of AltGr.
constexpr uint32_t kAltGrFakeCtrlScan = 0x21D;

/// Virtual-key codes needed by the translation logic.
constexpr uint16_t kVkCancel = 0x03;
constexpr uint16_t kVkPause = 0x13;
constexpr uint16_t kVkNumLock = 0x90;
constexpr uint16_t kVkPacket = 0xE7;

struct KeyMapping {
    KeyCode code;    ///< evdev key code (engine code space)
    uint16_t scan;   ///< set-1 scan code (without the E0 prefix)
    bool extended;   ///< E0-prefixed key (KEYEVENTF_EXTENDEDKEY / LLKHF_EXTENDED)
    uint16_t vk;     ///< Windows virtual-key code
    bool output_vk;  ///< Inject by virtual-key instead of scan code (Pause, media keys)
};

/// The full evdev <-> Windows table.
const std::vector<KeyMapping>& keyMappings();

/// Looks up how to inject an evdev key. Returns false for keys Windows cannot produce.
bool keyCodeToMapping(KeyCode code, KeyMapping& out);

/// Maps a low-level hook event (vk, scan, extended flag) to an evdev key code.
bool hookToKeyCode(uint32_t vk, uint32_t scan, bool extended, KeyCode& out);

/// One WH_KEYBOARD_LL callback, reduced to the fields TFF needs.
struct HookEvent {
    uint32_t vk = 0;
    uint32_t scan = 0;
    uint32_t flags = 0;  ///< LLKHF_* bits
    uintptr_t extra_info = 0;
};

enum class HookDecision {
    PassThrough,  ///< Let Windows handle it (our own output, unmapped keys, ...)
    Process,      ///< Swallow it and feed the produced events to the engine
    Drop          ///< Swallow it without telling the engine
};

/**
 * @brief Turns hook events into engine events.
 *
 * The engine buffers every key and re-emits the ones it doesn't remap itself, so every key that
 * maps to an evdev code is swallowed (HookDecision::Process); only unmapped keys pass through.
 */
class InputTranslator {
public:
    /// Ignore events injected by other software (AutoHotkey, on-screen keyboard, ...).
    void setIgnoreInjected(bool ignore) { ignore_injected_ = ignore; }
    bool isIgnoreInjected() const { return ignore_injected_; }

    /**
     * @brief A key the hook reports as still down after this long without any (auto-repeat)
     * event is treated as stale: its key-up was lost (secure desktop, Win+L, UAC, an elevated
     * window had focus). The next press then synthesizes the missing key-up first.
     */
    void setStaleAfterMs(int64_t ms) { stale_after_us_ = ms * 1000; }

    /**
     * @brief Classifies a hook event.
     * @param out Receives zero or more engine events (key-up, key-down or repeat) when the
     * decision is HookDecision::Process.
     */
    HookDecision translate(const HookEvent& in, TimeVal now, std::vector<Event>& out);

    /**
     * @brief Bookkeeping for an event Windows handles while TFF is paused.
     *
     * Keys pressed while paused went to Windows; after resuming, their auto-repeat and key-up
     * must go to Windows too, or they would stay stuck.
     */
    void observePassThrough(const HookEvent& in);

    /// A key Windows already sees as held (pressed before the hook was installed): its
    /// repeats and key-up pass through.
    void markForeignDown(KeyCode code) { foreign_.insert(code); }

    /// Forget the keys the engine was tracking (pause, session lock). Foreign keys stay
    /// known until Windows reports their key-up.
    void reset() { down_.clear(); }

    bool isDown(KeyCode code) const { return down_.count(code) != 0; }
    bool isForeign(KeyCode code) const { return foreign_.count(code) != 0; }
    size_t downCount() const { return down_.size(); }

private:
    bool ignore_injected_ = false;
    int64_t stale_after_us_ = 1500 * 1000;
    std::unordered_map<KeyCode, TimeVal> down_;  ///< key -> time of last down/repeat
    std::unordered_set<KeyCode> foreign_;        ///< held keys Windows saw go down
    bool altgr_ctrl_dropped_ = false;            ///< fake LCtrl key-down was dropped
};

/// One abstract SendInput record. The Win32 layer converts it into an INPUT struct.
struct OutputInput {
    enum class Type { Key, MouseMove, MouseButton, Wheel, HWheel, Unicode };
    Type type = Type::Key;
    // Type::Key
    KeyMapping key{};
    bool up = false;
    // Type::MouseButton: evdev BTN_* code and up/down in `up`
    KeyCode button = 0;
    // Type::MouseMove: relative motion; Type::Wheel / HWheel: notches in dx
    int32_t dx = 0;
    int32_t dy = 0;
    // Type::Unicode: one UTF-16 code unit (KEYEVENTF_UNICODE), up/down in `up`
    uint16_t unit = 0;
};

/// Converts UTF-8 to UTF-16 code units; invalid bytes become U+FFFD.
std::vector<uint16_t> utf8ToUtf16(const std::string& text);

/**
 * @brief EventWriter that collects engine output and hands it to a sender in batches.
 *
 * A batch is flushed on every EV_SYN and whenever the platform calls flush(). The builder also
 * remembers which keys it holds down, so it can release them all (pause, reset, quit) and
 * re-send the last held key on auto-repeat: the engine drops repeat events and SendInput keys
 * never auto-repeat, so a physical repeat is turned into a repeat of the last pressed output.
 */
class OutputBuilder : public EventWriter {
public:
    /// Injects a batch; returns false if Windows rejected it (secure desktop, UIPI).
    using Sender = std::function<bool(const std::vector<OutputInput>&)>;
    using Observer = std::function<void(const Event&)>;

    explicit OutputBuilder(Sender sender = nullptr) : sender_(std::move(sender)) {}

    void setSender(Sender sender) { sender_ = std::move(sender); }
    /// Called for every engine event written (logging, debug buffer).
    void setObserver(Observer observer) { observer_ = std::move(observer); }

    void writeOne(const Event& ev) override;

    /**
     * @brief Types text as Unicode characters (KEYEVENTF_UNICODE), one batch per character.
     * Unlike Shift + key strokes this works with every keyboard layout, including characters
     * that need AltGr. One batch per character, because Windows 11 Notepad garbles Unicode
     * characters that arrive in a single SendInput call. Enter and Tab are sent as keys,
     * because programs handle them as keys, not as characters.
     */
    bool writeText(const std::string& text) override;

    /// Send everything collected so far. Returns false if the sender rejected the batch.
    bool flush();

    /// A physical key auto-repeated: re-send the last held output key (if any).
    void onPhysicalRepeat();

    /// A physical key went down. Like a real keyboard, only output caused by the most recent
    /// physical key press repeats: holding `a` and then a layer key must not repeat `a`.
    void onPhysicalDown() { repeat_key_ = 0; }

    /**
     * @brief Release every key and mouse button this builder holds down, then flush.
     * If Windows rejects the key-ups (e.g. the secure desktop is active), the keys stay
     * recorded as held, so a later call can release them.
     */
    void releaseAll();

    bool isHeld(KeyCode code) const;
    size_t heldCount() const { return held_.size(); }
    KeyCode repeatKey() const { return repeat_key_; }

private:
    void addKey(KeyCode code, bool up);

    Sender sender_;
    Observer observer_;
    std::vector<OutputInput> pending_;
    std::vector<KeyCode> held_;  ///< keys and mouse buttons currently down, in press order
    KeyCode repeat_key_ = 0;     ///< last pressed non-modifier key still held (0 = none)
};

}  // namespace win
}  // namespace tff

#endif  // TFF_WIN_INPUT_H

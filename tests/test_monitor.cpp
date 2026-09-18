#include "tff_monitor.h"
#include "tff_engine.h"
#include "tff_parser.h"
#include "tff_key_codes.h"
#include "linux_platform.h"
#include <iostream>
#include <cassert>
#include <sstream>

using namespace tff;

namespace {

class DummyWriter : public tff::EventWriter {
public:
    void writeOne(const tff::Event& ev) override { written.push_back(ev); }
    std::vector<tff::Event> written;
};

void test_format_timestamp() {
    std::cout << "Test 1: Format timestamp... " << std::flush;
    tff::TimeVal rel_tv{0, 120000};  // 120ms
    std::string rel_ts = tff::EventMonitor::formatTimestamp(rel_tv);
    assert(rel_ts == "[00:00:00.120]");

    tff::TimeVal sec_tv{1700000000LL, 456000};
    std::string sec_ts = tff::EventMonitor::formatTimestamp(sec_tv);
    assert(sec_ts.front() == '[' && sec_ts.back() == ']');
    assert(sec_ts.size() == 14);
    std::cout << "PASSED\n";
}

void test_format_key() {
    std::cout << "Test 2: Format key... " << std::flush;
    assert(tff::EventMonitor::formatKey(tff::Keys::KEY_D) == "'d' (code: 32)");
    assert(tff::EventMonitor::formatKey(tff::Keys::KEY_LEFTCTRL) == "'leftctrl' (code: 29)");
    assert(tff::EventMonitor::formatKey(tff::Keys::KEY_ESC) == "'esc' (code: 1)");
    assert(tff::EventMonitor::formatKey(9999) == "(code: 9999)");
    std::cout << "PASSED\n";
}

void test_format_delta() {
    std::cout << "Test 3: Format delta... " << std::flush;
    assert(tff::EventMonitor::formatDelta(-1).empty());
    assert(tff::EventMonitor::formatDelta(0) == "(+0ms)");
    assert(tff::EventMonitor::formatDelta(25) == "(+25ms)");
    assert(tff::EventMonitor::formatDelta(1500) == "(+1500ms)");
    assert(tff::EventMonitor::formatDelta(15000) == "(>9.9s)");
    std::cout << "PASSED\n";
}

void test_issue_36_simulated_chord_stream() {
    std::cout << "Test 4: Issue #36 simulated chord stream... " << std::flush;
    DummyWriter writer;
    tff::TFFEngine engine(&writer);

    std::string yaml = R"(
combos:
  - in: [d, f, j]
    out: esc
)";
    tff::Config config;
    std::string err_msg;
    bool ok = tff::loadYamlConfig(yaml, config, err_msg);
    assert(ok);
    engine.setConfig(config);

    tff::MonitorOptions opts;
    opts.color = false;
    opts.show_deltas = true;
    opts.show_emitted = true;

    tff::EventMonitor monitor(opts);
    monitor.attachToEngine(engine);

    // 1. [14:23:01.120]  DOWN  'd' (code: 32)
    tff::Event ev1{tff::TimeVal{0, 120000}, EV_KEY, tff::Keys::KEY_D, tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(ev1);
    std::string l1 = monitor.formatEvent(ev1);
    assert(l1.find("DOWN") != std::string::npos);
    assert(l1.find("'d' (code: 32)") != std::string::npos);

    // 2. [14:23:01.145]  DOWN  'f' (code: 33)   (+25ms)  -> CHORD CANDIDATE: d + f
    tff::Event ev2{tff::TimeVal{0, 145000}, EV_KEY, tff::Keys::KEY_F, tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(ev2);
    std::string l2 = monitor.formatEvent(ev2);
    assert(l2.find("DOWN") != std::string::npos);
    assert(l2.find("'f' (code: 33)") != std::string::npos);
    assert(l2.find("(+25ms)") != std::string::npos);
    assert(l2.find("CHORD CANDIDATE: d + f") != std::string::npos);

    // 3. [14:23:01.170]  DOWN  'j' (code: 36)   (+25ms)  -> CHORD CANDIDATE: d + f + j
    tff::Event ev3{tff::TimeVal{0, 170000}, EV_KEY, tff::Keys::KEY_J, tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(ev3);
    std::string l3 = monitor.formatEvent(ev3);
    assert(l3.find("DOWN") != std::string::npos);
    assert(l3.find("'j' (code: 36)") != std::string::npos);
    assert(l3.find("(+25ms)") != std::string::npos);
    assert(l3.find("CHORD CANDIDATE: d + f + j") != std::string::npos);

    // 4. [14:23:01.210]  UP    'd' (code: 32)   (+40ms)  (swallowed) -> TRIGGER COMBO: d + f + j ->
    // esc
    tff::Event ev4{tff::TimeVal{0, 210000}, EV_KEY, tff::Keys::KEY_D, tff::KEY_VAL_UP};
    monitor.clearTrace();
    engine.processEvent(ev4);
    std::string l4 = monitor.formatEvent(ev4);
    assert(l4.find("UP") != std::string::npos);
    assert(l4.find("'d' (code: 32)") != std::string::npos);
    assert(l4.find("(swallowed)") != std::string::npos);
    assert(l4.find("TRIGGER COMBO: d + f + j -> esc") != std::string::npos);
    assert(l4.find("EMIT: esc") != std::string::npos);

    std::cout << "PASSED\n";
}

void test_color_output() {
    std::cout << "Test 5: Color formatting output... " << std::flush;
    tff::MonitorOptions opts;
    opts.color = true;

    tff::EventMonitor monitor(opts);
    tff::Event ev{tff::TimeVal{0, 100000}, EV_KEY, tff::Keys::KEY_A, tff::KEY_VAL_DOWN};
    std::string out = monitor.formatEvent(ev);
    assert(out.find("\033[") != std::string::npos);
    std::cout << "PASSED\n";
}

void test_tap_hold_monitor() {
    std::cout << "Test 6: Tap-hold monitoring... " << std::flush;
    DummyWriter writer;
    tff::TFFEngine engine(&writer);

    std::string yaml = R"(
tap_hold:
  - key: capslock
    tap: esc
    hold: leftmeta
    timeout: 200
)";
    tff::Config config;
    std::string err_msg;
    bool ok = tff::loadYamlConfig(yaml, config, err_msg);
    assert(ok);
    engine.setConfig(config);

    tff::MonitorOptions opts;
    opts.color = false;
    opts.show_deltas = true;
    opts.show_emitted = true;

    tff::EventMonitor monitor(opts);
    monitor.attachToEngine(engine);

    // Tap sequence: DOWN capslock, then UP after 50ms
    tff::Event ev_down{tff::TimeVal{0, 100000}, EV_KEY, tff::Keys::KEY_CAPSLOCK, tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(ev_down);
    std::string l_down = monitor.formatEvent(ev_down);
    assert(l_down.find("TAP-HOLD: waiting (capslock, timeout: 200ms)") != std::string::npos);

    tff::Event ev_up{tff::TimeVal{0, 150000}, EV_KEY, tff::Keys::KEY_CAPSLOCK, tff::KEY_VAL_UP};
    monitor.clearTrace();
    engine.processEvent(ev_up);
    std::string l_up = monitor.formatEvent(ev_up);
    assert(l_up.find("TAP-HOLD: tap 'esc'") != std::string::npos);
    assert(l_up.find("EMIT: esc (code: 1, DOWN)") != std::string::npos);

    // Hold sequence: DOWN capslock, timeout fires
    tff::Event ev2_down{tff::TimeVal{0, 300000}, EV_KEY, tff::Keys::KEY_CAPSLOCK,
                        tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(ev2_down);
    monitor.formatEvent(ev2_down);

    monitor.clearTrace();
    engine.onTimer(tff::TimeVal{0, 500000});
    std::string t_out = monitor.formatTimer(tff::TimeVal{0, 500000});
    assert(t_out.find("TIMER EXPIRED") != std::string::npos);
    assert(t_out.find("TAP-HOLD: hold 'leftmeta'") != std::string::npos);
    assert(t_out.find("EMIT: leftmeta") != std::string::npos);

    std::cout << "PASSED\n";
}

void test_modal_layers_monitor() {
    std::cout << "Test 7: Modal layers monitoring... " << std::flush;
    DummyWriter writer;
    tff::TFFEngine engine(&writer);

    std::string yaml = R"(
layers:
  nav:
    j: down
    k: up
combos:
  - in: [d, f]
    toggle_layer: nav
)";
    tff::Config config;
    std::string err_msg;
    bool ok = tff::loadYamlConfig(yaml, config, err_msg);
    assert(ok);
    engine.setConfig(config);

    tff::MonitorOptions opts;
    opts.color = false;
    opts.show_deltas = true;
    opts.show_emitted = true;

    tff::EventMonitor monitor(opts);
    monitor.attachToEngine(engine);

    // Press d + f to toggle nav
    tff::Event d_down{tff::TimeVal{0, 100000}, EV_KEY, tff::Keys::KEY_D, tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(d_down);
    monitor.formatEvent(d_down);

    tff::Event f_down{tff::TimeVal{0, 120000}, EV_KEY, tff::Keys::KEY_F, tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(f_down);
    std::string l_cand = monitor.formatEvent(f_down);
    assert(l_cand.find("CHORD CANDIDATE: d + f") != std::string::npos);

    tff::Event d_up{tff::TimeVal{0, 165000}, EV_KEY, tff::Keys::KEY_D, tff::KEY_VAL_UP};
    monitor.clearTrace();
    engine.processEvent(d_up);
    std::string l_trig = monitor.formatEvent(d_up);
    assert(l_trig.find("TRIGGER COMBO: d + f -> toggle_layer(nav)") != std::string::npos);
    assert(l_trig.find("LAYER TOGGLE: nav (on)") != std::string::npos);

    std::cout << "PASSED\n";
}

void test_leader_key_monitor() {
    std::cout << "Test 8: Leader key monitoring... " << std::flush;
    DummyWriter writer;
    tff::TFFEngine engine(&writer);

    std::string yaml = R"(
leader:
  key: capslock
  timeout: 1000
  sequences:
    "g c": "git commit"
)";
    tff::Config config;
    std::string err_msg;
    bool ok = tff::loadYamlConfig(yaml, config, err_msg);
    assert(ok);
    engine.setConfig(config);

    tff::MonitorOptions opts;
    opts.color = false;

    tff::EventMonitor monitor(opts);
    monitor.attachToEngine(engine);

    // Tap capslock to activate leader
    tff::Event cap_down{tff::TimeVal{0, 100000}, EV_KEY, tff::Keys::KEY_CAPSLOCK,
                        tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(cap_down);
    monitor.formatEvent(cap_down);

    tff::Event cap_up{tff::TimeVal{0, 120000}, EV_KEY, tff::Keys::KEY_CAPSLOCK, tff::KEY_VAL_UP};
    monitor.clearTrace();
    engine.processEvent(cap_up);
    std::string l_active = monitor.formatEvent(cap_up);
    assert(l_active.find("LEADER: active") != std::string::npos);

    // Press 'g'
    tff::Event g_down{tff::TimeVal{0, 150000}, EV_KEY, tff::Keys::KEY_G, tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(g_down);
    std::string l_cand = monitor.formatEvent(g_down);
    assert(l_cand.find("LEADER CANDIDATE: g") != std::string::npos);

    // Press 'c' -> trigger sequence
    tff::Event c_down{tff::TimeVal{0, 200000}, EV_KEY, tff::Keys::KEY_C, tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(c_down);
    std::string l_trig = monitor.formatEvent(c_down);
    assert(l_trig.find("LEADER TRIGGER: g + c -> \"git commit\"") != std::string::npos);

    std::cout << "PASSED\n";
}

void test_auto_shift_monitor() {
    std::cout << "Test 9: Auto-shift monitoring... " << std::flush;
    DummyWriter writer;
    tff::TFFEngine engine(&writer);

    std::string yaml = R"(
auto_shift:
  enabled: true
  timeout_ms: 150
  keys: [a]
)";
    tff::Config config;
    std::string err_msg;
    bool ok = tff::loadYamlConfig(yaml, config, err_msg);
    assert(ok);
    engine.setConfig(config);

    tff::MonitorOptions opts;
    opts.color = false;

    tff::EventMonitor monitor(opts);
    monitor.attachToEngine(engine);

    // Hold 'a' past 150ms timeout
    tff::Event a_down{tff::TimeVal{0, 100000}, EV_KEY, tff::Keys::KEY_A, tff::KEY_VAL_DOWN};
    monitor.clearTrace();
    engine.processEvent(a_down);
    monitor.formatEvent(a_down);

    monitor.clearTrace();
    engine.onTimer(tff::TimeVal{0, 260000});
    std::string t_out = monitor.formatTimer(tff::TimeVal{0, 260000});
    assert(t_out.find("AUTO-SHIFT: hold 'a' (capitalized)") != std::string::npos);
    assert(t_out.find("EMIT: leftshift") != std::string::npos);
    assert(t_out.find("EMIT: a") != std::string::npos);

    std::cout << "PASSED\n";
}

void test_device_label() {
    std::cout << "Test 10: Device label formatting... " << std::flush;
    tff::MonitorOptions opts;
    opts.color = false;
    tff::EventMonitor monitor(opts);

    tff::Event ev{tff::TimeVal{0, 100000}, EV_KEY, tff::Keys::KEY_D, tff::KEY_VAL_DOWN};
    std::string out = monitor.formatEvent(ev, "event8");
    assert(out.find("[event8]") != std::string::npos);
    std::cout << "PASSED\n";
}

void test_platform_monitor_api() {
    std::cout << "Test 11: LinuxPlatform monitor API... " << std::flush;
    LinuxPlatform platform;
    platform.setGrab(false);
    platform.setEmitToUinput(false);
    assert(!platform.isGrabbed());
    assert(!platform.isEmitToUinput());

    bool init_ok = platform.initialize();
    assert(init_ok);

    const tff::TFFEngine& engine = platform.getEngine();
    assert(engine.getCombos().empty());
    std::cout << "PASSED\n";
}

void test_options_toggles() {
    std::cout << "Test 12: Monitor options toggles (--no-deltas, --no-emitted, --plain)... "
              << std::flush;
    DummyWriter writer;
    tff::TFFEngine engine(&writer);

    tff::MonitorOptions opts;
    opts.color = false;
    opts.show_deltas = false;
    opts.show_emitted = false;

    tff::EventMonitor monitor(opts);
    monitor.attachToEngine(engine);

    tff::Event ev1{tff::TimeVal{0, 100000}, EV_KEY, tff::Keys::KEY_A, tff::KEY_VAL_DOWN};
    engine.processEvent(ev1);
    std::string l1 = monitor.formatEvent(ev1);

    // No ANSI color escapes
    assert(l1.find("\033[") == std::string::npos);
    // No delta ms
    assert(l1.find("ms)") == std::string::npos);
    // No virtual emitted keys
    assert(l1.find("EMIT:") == std::string::npos);

    // Timer with only EmitKey and show_emitted = false returns empty string
    engine.trace(tff::TraceEvent::Kind::EmitKey, "EMIT: a (code: 30, DOWN)");
    std::string t_out = monitor.formatTimer(tff::TimeVal{0, 200000});
    assert(t_out.empty());

    std::cout << "PASSED\n";
}

void test_column_alignment_long_key_names() {
    std::cout << "Test 13: Column alignment with long key names... " << std::flush;
    tff::MonitorOptions opts;
    opts.color = false;
    opts.show_deltas = true;
    opts.show_emitted = true;

    tff::EventMonitor monitor(opts);
    DummyWriter writer;
    tff::TFFEngine engine(&writer);
    monitor.attachToEngine(engine);

    // CapsLock key has long string: "'capslock' (code: 58)" (21 characters)
    tff::Event ev{tff::TimeVal{0, 100000}, EV_KEY, tff::Keys::KEY_CAPSLOCK, tff::KEY_VAL_DOWN};
    engine.trace(tff::TraceEvent::Kind::TapHoldHold, "TAP-HOLD: hold 'layer(nav)'");
    engine.trace(tff::TraceEvent::Kind::EmitKey, "EMIT: esc (code: 1, DOWN)");

    std::string out = monitor.formatEvent(ev);
    std::istringstream iss(out);
    std::string line1, line2;
    std::getline(iss, line1);
    std::getline(iss, line2);

    size_t pos1 = line1.find("-> TAP-HOLD");
    size_t pos2 = line2.find("-> EMIT");
    assert(pos1 != std::string::npos);
    assert(pos2 != std::string::npos);
    // Exact column alignment
    assert(pos1 == pos2);
    assert(pos1 == 60);

    // Timer column alignment matches column 60
    engine.trace(tff::TraceEvent::Kind::LayerActive, "LAYER TOGGLE: nav (on)");
    std::string timer_out = monitor.formatTimer(tff::TimeVal{0, 250000});
    size_t timer_pos = timer_out.find("-> LAYER TOGGLE");
    assert(timer_pos != std::string::npos);
    assert(timer_pos == 60);

    std::cout << "PASSED\n";
}

}  // anonymous namespace

int main() {
    std::cout << "Running TFF Monitor Unit Tests:\n";
    std::cout << "===============================\n";
    test_format_timestamp();
    test_format_key();
    test_format_delta();
    test_issue_36_simulated_chord_stream();
    test_color_output();
    test_tap_hold_monitor();
    test_modal_layers_monitor();
    test_leader_key_monitor();
    test_auto_shift_monitor();
    test_device_label();
    test_platform_monitor_api();
    test_options_toggles();
    test_column_alignment_long_key_names();
    std::cout << "===============================\n";
    std::cout << "All 13 monitor tests PASSED!\n";
    return 0;
}

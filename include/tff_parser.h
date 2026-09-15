#ifndef TFF_PARSER_H
#define TFF_PARSER_H

#include "tff_types.h"
#include <string>
#include <vector>
#include <istream>

namespace tff {

// Parses single CSV line into an Event
bool csvLineToEvent(const std::string& line, Event& ev, std::string& err_msg);

// Parses multi-line CSV string into vector of Events
bool csvToEvents(const std::string& csv_str, std::vector<Event>& events, std::string& err_msg);

// Parses state string (e.g. "capslock_ (259.006ms) j_ (105.844ms) j/ (721.7ms) capslock/")
bool stateStringToEvents(const std::string& state_str, std::vector<Event>& events, std::string& err_msg);

// Parses combo log with |>> prefix lines into vector of Events
bool parseComboLog(std::istream& in, std::vector<Event>& events, std::string& err_msg);
bool parseComboLog(const std::string& log_str, std::vector<Event>& events, std::string& err_msg);

// Converts an Event to a CSV line
std::string eventToCsvLine(const Event& ev);

// Converts a vector of Events to a CSV string (skipping SYN and MSC)
std::string eventsToCsv(const std::vector<Event>& events);

// Converts events to short CSV lines (e.g. "F-down\nF-up\n...")
std::string eventsToShortCsv(const std::vector<Event>& events);

// Normalizes and trims expected short CSV lines for exact comparison
std::string normalizeShortCsv(const std::string& input);

// Parses duration string like "259.006ms", "1.000008s", "721.7ms" into microseconds
bool parseDurationMicros(const std::string& str, int64_t& out_us);

// Loads YAML combos matching Go load_yaml.go
bool loadYamlCombos(const std::string& yaml_str, std::vector<Combo>& combos, std::string& err_msg);

// Loads complete YAML configuration including combos and tap_hold settings
bool loadYamlConfig(const std::string& yaml_str, Config& config, std::string& err_msg);

} // namespace tff

#endif // TFF_PARSER_H

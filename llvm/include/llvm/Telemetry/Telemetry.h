//===- llvm/Telemetry/Telemetry.h - Telemetry -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TELEMETRY_TELEMETRY_H
#define LLVM_TELEMETRY_TELEMETRY_H

#include <chrono>
#include <ctime>
#include <memory>
#include <optional>
#include <string>

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/JSON.h"

namespace llvm {
namespace telemetry {

using SteadyTimePoint = std::chrono::time_point<std::chrono::steady_clock>;

struct TelemetryConfig {
  // If true, telemetry will be enabled.
  bool enable_telemetry;

  // Additional destinations to send the logged entries.
  // Could be stdout, stderr, or some local paths.
  // Note: these are destinations are __in addition to__ whatever the default
  // destination(s) are, as implemented by vendors.
  std::vector<std::string> additional_destinations;
};

struct TelemetryEventStats {
  // REQUIRED: Start time of event
  SteadyTimePoint m_start;
  // OPTIONAL: End time of event - may be empty if not meaningful.
  std::optional<SteadyTimePoint> m_end;
  // TBD: could add some memory stats here too?

  TelemetryEventStats() = default;
  TelemetryEventStats(SteadyTimePoint start) : m_start(start) {}
  TelemetryEventStats(SteadyTimePoint start, SteadyTimePoint end)
      : m_start(start), m_end(end) {}

  std::string ToString() const;
};

struct ExitDescription {
  int exit_code;
  std::string description;

  std::string ToString() const;
};

// The base class contains the basic set of data.
// Downstream implementations can add more fields as needed.
struct TelemetryInfo {
  // A "session" corresponds to every time the tool starts.
  // All entries emitted for the same session will have
  // the same session_uuid
  std::string session_uuid;

  TelemetryEventStats stats;

  std::optional<ExitDescription> exit_description;

  // Counting number of entries.
  // (For each set of entries with the same session_uuid, this value should
  // be unique for each entry)
  size_t counter;

  TelemetryInfo() = default;
  ~TelemetryInfo() = default;
  virtual std::string ToString() const;
};

// Where/how to send the telemetry entries.
class TelemetryDestination {
public:
  virtual ~TelemetryDestination() = default;
  virtual Error EmitEntry(const TelemetryInfo *entry) = 0;
  virtual std::string name() const = 0;
};

class Telemeter {
public:
  virtual ~Telemeter() = default;

  // Invoked upon tool startup
  virtual void LogStartup(llvm::StringRef tool_path, TelemetryInfo *entry) = 0;

  // Invoked upon tool exit.
  virtual void LogExit(llvm::StringRef tool_path, TelemetryInfo *entry) = 0;

  virtual void AddDestination(TelemetryDestination *destination) = 0;
};

} // namespace telemetry
} // namespace llvm

#endif // LLVM_TELEMETRY_TELEMETRY_H

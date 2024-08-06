
//===-- Telemetry.cpp -----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "lldb/Core/Telemetry.h"

#include <stdbool.h>
#include <sys/auxv.h>

#include <memory>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "lldb/API/SBDebugger.h"
#include "lldb/API/SBProcess.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/Module.h"
#include "lldb/Host/FileSystem.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/Interpreter/CommandInterpreter.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Statistics.h"
#include "lldb/Utility/ConstString.h"
#include "lldb/Utility/FileSpec.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/UUID.h"
#include "lldb/Version/Version.h"
#include "lldb/lldb-enumerations.h"
#include "lldb/lldb-forward.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Telemetry/Telemetry.h"

#ifdef HAS_VENDOR_TELEMETRY_PLUGINS
// TODO: could make this path a build-variable rather than hard-coded
#include "lldb/Core/VendorTelemetryPlugin.h"

// This must include definitions for these functions
extern void ApplyVendorSpecificConfigs(
    llvm::telemetry::TelemetryConfig *config) /* __attribute__((weak))*/;
extern std::shared_ptr<llvm::telemetry::TelemetryInfo> SanitizeSensitiveFields(
    const llvm::telemetry::TelemetryInfo *entry) /*__attribute__((weak))*/;
extern std::shared_ptr<lldb_private::LldbTelemeter>
CreateVendorSpecificTelemeter(
    llvm::telemetry::TelemetryConfig *config) /*__attribute__((weak))*/;
#endif

using namespace llvm::telemetry;

namespace lldb_private {

static std::string GetDuration(const TelemetryEventStats &stats) {
  if (stats.m_end.has_value())
    return std::to_string((stats.m_end.value() - stats.m_start).count()) +
           "(nanosec)";
  return "<NONE>";
}

std::string DebuggerTelemetryInfo::ToString() const {
  std::string duration_desc =
      (exit_description.has_value() ? "  lldb session duration: "
                                    : "  lldb startup duration: ") +
      std::to_string((stats.m_end.value() - stats.m_start).count()) +
      "(nanosec)\n";

  return TelemetryInfo::ToString() + "\n" + ("[DebuggerTelemetryInfo]\n") +
         ("  username: " + username + "\n") +
         ("  lldb_git_sha: " + lldb_git_sha + "\n") +
         ("  lldb_path: " + lldb_path + "\n") + ("  cwd: " + cwd + "\n") +
         duration_desc + "\n";
}

std::string ClientTelemetryInfo::ToString() const {
  return TelemetryInfo::ToString() + "\n" + ("[DapRequestInfoEntry]\n") +
         ("  request_name: " + request_name + "\n") +
         ("  request_duration: " + GetDuration(stats) + "(nanosec)\n") +
         ("  error_msg: " + error_msg + "\n");
}

std::string TargetTelemetryInfo::ToString() const {
  std::string exit_or_load_desc;
  if (exit_description.has_value()) {
    // If this entry was emitted for an exit
    exit_or_load_desc = "  process_duration: " + GetDuration(stats) +
                        "  exit: " + exit_description->ToString() + "\n";
  } else {
    // This was emitted for a load event.
    // See if it was the start-load or end-load entry
    if (stats.m_end.has_value()) {
      exit_or_load_desc =
          "  startup_init_duration: " + GetDuration(stats) + "\n";
    } else {
      exit_or_load_desc = " startup_init_start\n";
    }
  }
  return TelemetryInfo::ToString() + "\n" + ("[TargetTelemetryInfo]\n") +
         ("  target_uuid: " + target_uuid + "\n") +
         ("  file_format: " + file_format + "\n") +
         ("  binary_path: " + binary_path + "\n") +
         ("  binary_size: " + std::to_string(binary_size) + "\n") +
         exit_or_load_desc;
}

static std::string StatusToString(CommandReturnObject *result) {
  std::string msg;
  switch (result->GetStatus()) {
  case lldb::eReturnStatusInvalid:
    msg = "invalid";
    break;
  case lldb::eReturnStatusSuccessFinishNoResult:
    msg = "success_finish_no_result";
    break;
  case lldb::eReturnStatusSuccessFinishResult:
    msg = "success_finish_result";
    break;
  case lldb::eReturnStatusSuccessContinuingNoResult:
    msg = "success_continuing_no_result";
    break;
  case lldb::eReturnStatusSuccessContinuingResult:
    msg = "success_continuing_result";
    break;
  case lldb::eReturnStatusStarted:
    msg = "started";
    break;
  case lldb::eReturnStatusFailed:
    msg = "failed";
    break;
  case lldb::eReturnStatusQuit:
    msg = "quit";
    break;
  }
  if (llvm::StringRef error_data = result->GetErrorData();
      !error_data.empty()) {
    msg += " Error msg: " + error_data.str();
  }
  return msg;
}

std::string CommandTelemetryInfo::ToString() const {
  // Whether this entry was emitted at the start or at the end of the
  // command-execution.
  if (stats.m_end.has_value()) {
    return TelemetryInfo::ToString() + "\n" +
           ("[CommandTelemetryInfo] - END\n") +
           ("  target_uuid: " + target_uuid + "\n") +
           ("  command_uuid: " + command_uuid + "\n") +
           ("  command_name: " + command_name + "\n") +
           ("  args: " + args + "\n") +
           ("  command_runtime: " + GetDuration(stats) + "\n") +
           (exit_description.has_value() ? exit_description->ToString()
                                         : "no exit-description") +
           "\n";
  } else {
    return TelemetryInfo::ToString() + "\n" +
           ("[CommandTelemetryInfo] - START\n") +
           ("  target_uuid: " + target_uuid + "\n") +
           ("  command_uuid: " + command_uuid + "\n") +
           ("  original_command: " + original_command + "\n");
  }
}

std::string MiscTelemetryInfo::ToString() const {
  std::string ret;
  llvm::raw_string_ostream ret_strm(ret);
  ret_strm << TelemetryInfo::ToString() << "\n[MiscTelemetryInfo]\n"
           << "  target_uuid: " << target_uuid + "\n"
           << "  meta_data:\n";
  for (const auto &kv : meta_data) {
    ret_strm << "    " << kv.first << ": " << kv.second << "\n";
  }
  return ret;
}

class StreamTelemetryDestination : public TelemetryDestination {
public:
  StreamTelemetryDestination(llvm::raw_ostream &os, std::string desc)
      : os(os), desc(desc) {}
  llvm::Error EmitEntry(const TelemetryInfo *entry) override {
    // Unless there exists a custom (vendor-defined) data-cleanup
    // for printing, upstream Telemetry should not leak anything other than the
    // basic.
#ifdef HAS_TELEMETRY_FIELDS_PRINTER
    os << SanitizeSensitiveFields(entry)->ToString() << "\n";
#else
    os << "session_uuid: " << entry->session_uuid
       << "<the rest is omitted due to PII risk>\n";
#endif
    os.flush();
    return llvm::ErrorSuccess();
  }

  std::string name() const override { return desc; }

private:
  llvm::raw_ostream &os;
  const std::string desc;
};

// No-op logger to use when users disable telemetry
class NoOpTelemeter : public LldbTelemeter {
public:
  static std::shared_ptr<LldbTelemeter> CreateInstance(Debugger *debugger) {
    static std::shared_ptr<LldbTelemeter> ins(new NoOpTelemeter(debugger));
    return ins;
  }

  NoOpTelemeter(Debugger *debugger) {}
  void LogStartup(llvm::StringRef tool_path, TelemetryInfo *entry) override {}
  void LogExit(llvm::StringRef tool_path, TelemetryInfo *entry) override {}
  void LogProcessExit(int status, llvm::StringRef exit_string,
                      TelemetryEventStats stats, Target *target_ptr) override {}
  void LogMainExecutableLoadStart(lldb::ModuleSP exec_mod,
                                  TelemetryEventStats stats) override {}
  void LogMainExecutableLoadEnd(lldb::ModuleSP exec_mod,
                                TelemetryEventStats stats) override {}

  void LogCommandStart(llvm::StringRef uuid, llvm::StringRef original_command,
                       TelemetryEventStats stats, Target *target_ptr) override {
  }
  void LogCommandEnd(llvm::StringRef uuid, llvm::StringRef command_name,
                     llvm::StringRef command_args, TelemetryEventStats stats,
                     Target *target_ptr, CommandReturnObject *result) override {
  }

  void
  LogClientTelemetry(lldb_private::StructuredData::Object *entry) override {}

  void AddDestination(TelemetryDestination *destination) override {}
  std::string GetNextUUID() override { return ""; }
};

class BasicTelemeter : public LldbTelemeter {
public:
  static std::shared_ptr<BasicTelemeter> CreateInstance(Debugger *);

  virtual ~BasicTelemeter() = default;

  void LogStartup(llvm::StringRef lldb_path, TelemetryInfo *entry) override;
  void LogExit(llvm::StringRef lldb_path, TelemetryInfo *entry) override;

  void LogProcessExit(int status, llvm::StringRef exit_string,
                      TelemetryEventStats stats, Target *target_ptr) override;
  void LogMainExecutableLoadStart(lldb::ModuleSP exec_mod,
                                  TelemetryEventStats stats) override;
  void LogMainExecutableLoadEnd(lldb::ModuleSP exec_mod,
                                TelemetryEventStats stats) override;

  void LogCommandStart(llvm::StringRef uuid, llvm::StringRef original_command,
                       TelemetryEventStats stats, Target *target_ptr) override;
  void LogCommandEnd(llvm::StringRef uuid, llvm::StringRef command_name,
                     llvm::StringRef command_args, TelemetryEventStats stats,
                     Target *target_ptr, CommandReturnObject *result) override;

  void LogClientTelemetry(lldb_private::StructuredData::Object *entry) override;

  void AddDestination(TelemetryDestination *destination) override {
    m_destinations.push_back(destination);
  }

  std::string GetNextUUID() override {
    return std::to_string(uuid_seed.fetch_add(1));
  }

protected:
  BasicTelemeter(Debugger *debugger);

  void CollectMiscBuildInfo();

private:
  template <typename EntrySubType> EntrySubType MakeBaseEntry() {
    EntrySubType entry;
    entry.session_uuid = m_session_uuid;
    entry.counter = counter.fetch_add(1);
    return entry;
  }

  void EmitToDestinations(const TelemetryInfo *entry);

  Debugger *m_debugger;
  const std::string m_session_uuid;
  std::string startup_lldb_path;

  // counting number of entries.
  std::atomic<size_t> counter = 0;

  std::vector<TelemetryDestination *> m_destinations;

  std::atomic<size_t> uuid_seed = 0;
};

static std::string MakeUUID(lldb_private::Debugger *debugger) {
  std::string ret;
  uint8_t random_bytes[16];
  if (auto ec = llvm::getRandomBytes(random_bytes, 16)) {
    LLDB_LOG(GetLog(LLDBLog::Object),
             "Failed to generate random bytes for UUID: {0}", ec.message());
    // fallback to using timestamp + debugger ID.
    ret = std::to_string(
              std::chrono::steady_clock::now().time_since_epoch().count()) +
          "_" + std::to_string(debugger->GetID());
  } else {
    ret = lldb_private::UUID(random_bytes).GetAsString();
  }

  return ret;
}

BasicTelemeter::BasicTelemeter(lldb_private::Debugger *debugger)
    : m_debugger(debugger), m_session_uuid(MakeUUID(debugger)) {}

std::shared_ptr<BasicTelemeter>
BasicTelemeter::CreateInstance(lldb_private::Debugger *debugger) {
  auto *config = GetTelemetryConfig();

  BasicTelemeter *ins = new BasicTelemeter(debugger);
  for (const std ::string &dest : config->additional_destinations) {
    if (dest == "stdout") {
      ins->AddDestination(
          new StreamTelemetryDestination(llvm::outs(), "stdout"));
    } else if (dest == "stderr") {
      ins->AddDestination(
          new StreamTelemetryDestination(llvm::errs(), "stderr"));
    } else {
      // TODO: handle file paths
    }
  }

  return std::shared_ptr<BasicTelemeter>(ins);
}

void BasicTelemeter::EmitToDestinations(const TelemetryInfo *entry) {
  // TODO: can do this in a separate thread (need to own the ptrs!).
  for (TelemetryDestination *destination : m_destinations) {
    llvm::Error err = destination->EmitEntry(entry);
    if (err) {
      LLDB_LOG(GetLog(LLDBLog::Object),
               "Error emitting to destination(name = {0})",
               destination->name());
    }
  }
}

void BasicTelemeter::LogStartup(llvm::StringRef lldb_path,
                                TelemetryInfo *entry) {
  startup_lldb_path = lldb_path.str();
  lldb_private::DebuggerTelemetryInfo startup_info =
      MakeBaseEntry<lldb_private::DebuggerTelemetryInfo>();

  UserIDResolver &resolver = lldb_private::HostInfo::GetUserIDResolver();
  std::optional<llvm::StringRef> opt_username =
      resolver.GetUserName(lldb_private::HostInfo::GetUserID());
  if (opt_username)
    startup_info.username = *opt_username;

  startup_info.lldb_git_sha = lldb_private::GetVersion(); // TODO: fix this
  startup_info.lldb_path = startup_lldb_path;
  startup_info.stats = entry->stats;

  llvm::SmallString<64> cwd;
  if (!llvm::sys::fs::current_path(cwd)) {
    startup_info.cwd = cwd.c_str();
  } else {
    MiscTelemetryInfo misc_info = MakeBaseEntry<MiscTelemetryInfo>();
    misc_info.meta_data["internal_errors"] = "Cannot determine CWD";
    EmitToDestinations(&misc_info);
  }

  EmitToDestinations(&startup_info);

  // Optional part
  CollectMiscBuildInfo();
}

void BasicTelemeter::LogExit(llvm::StringRef lldb_path, TelemetryInfo *entry) {
  // we should be shutting down the same instance that we started?!
  // llvm::Assert(startup_lldb_path == lldb_path.str());

  lldb_private::DebuggerTelemetryInfo exit_info =
      MakeBaseEntry<lldb_private::DebuggerTelemetryInfo>();
  exit_info.stats = entry->stats;
  exit_info.lldb_path = startup_lldb_path;
  if (auto *selected_target =
          m_debugger->GetSelectedExecutionContext().GetTargetPtr()) {
    if (!selected_target->IsDummyTarget()) {
      const lldb::ProcessSP proc = selected_target->GetProcessSP();
      if (proc == nullptr) {
        // no process has been launched yet.
        exit_info.exit_description = {-1, "no process launched."};
      } else {
        exit_info.exit_description = {proc->GetExitStatus(), ""};
        if (const char *description = proc->GetExitDescription())
          exit_info.exit_description->description = std::string(description);
      }
    }
  }
  EmitToDestinations(&exit_info);
}

void BasicTelemeter::LogProcessExit(int status, llvm::StringRef exit_string,
                                    TelemetryEventStats stats,
                                    Target *target_ptr) {
  lldb_private::TargetTelemetryInfo exit_info =
      MakeBaseEntry<lldb_private::TargetTelemetryInfo>();
  exit_info.stats = stats;
  exit_info.target_uuid =
      target_ptr && !target_ptr->IsDummyTarget()
          ? target_ptr->GetExecutableModule()->GetUUID().GetAsString()
          : "";
  exit_info.exit_description = {status, exit_string.str()};

  EmitToDestinations(&exit_info);
}

void BasicTelemeter::CollectMiscBuildInfo() {
  // collecting use-case specific data
}

void BasicTelemeter::LogMainExecutableLoadStart(lldb::ModuleSP exec_mod,
                                                TelemetryEventStats stats) {
  TargetTelemetryInfo target_info = MakeBaseEntry<TargetTelemetryInfo>();
  target_info.stats = std::move(stats);
  target_info.binary_path =
      exec_mod->GetFileSpec().GetPathAsConstString().GetCString();
  target_info.file_format = exec_mod->GetArchitecture().GetArchitectureName();
  target_info.target_uuid = exec_mod->GetUUID().GetAsString();
  if (auto err = llvm::sys::fs::file_size(exec_mod->GetFileSpec().GetPath(),
                                          target_info.binary_size)) {
    // If there was error obtaining it, just reset the size to 0.
    // Maybe log the error too?
    target_info.binary_size = 0;
  }
  EmitToDestinations(&target_info);
}

void BasicTelemeter::LogMainExecutableLoadEnd(lldb::ModuleSP exec_mod,
                                              TelemetryEventStats stats) {
  TargetTelemetryInfo target_info = MakeBaseEntry<TargetTelemetryInfo>();
  target_info.stats = std::move(stats);
  target_info.binary_path =
      exec_mod->GetFileSpec().GetPathAsConstString().GetCString();
  target_info.file_format = exec_mod->GetArchitecture().GetArchitectureName();
  target_info.target_uuid = exec_mod->GetUUID().GetAsString();
  target_info.binary_size = exec_mod->GetObjectFile()->GetByteSize();

  EmitToDestinations(&target_info);

  // Collect some more info,  might be useful?
  MiscTelemetryInfo misc_info = MakeBaseEntry<MiscTelemetryInfo>();
  misc_info.target_uuid = exec_mod->GetUUID().GetAsString();
  misc_info.meta_data["symtab_index_time"] =
      std::to_string(exec_mod->GetSymtabIndexTime().get().count());
  misc_info.meta_data["symtab_parse_time"] =
      std::to_string(exec_mod->GetSymtabParseTime().get().count());
  EmitToDestinations(&misc_info);
}

void BasicTelemeter::LogClientTelemetry(
    lldb_private::StructuredData::Object *entry) {
  ClientTelemetryInfo client_info = MakeBaseEntry<ClientTelemetryInfo>();
  auto *dictionary = entry->GetAsDictionary();
  llvm::StringRef request_name;
  if (!dictionary->GetValueForKeyAsString("request_name", request_name, "")) {
    MiscTelemetryInfo misc_info = MakeBaseEntry<MiscTelemetryInfo>();
    misc_info.meta_data["internal_errors"] =
        "Cannot determine request name from client entry";
    // TODO: Dump the errornous entry to stderr too?
    EmitToDestinations(&misc_info);
    return;
  }
  client_info.request_name = request_name.str();

  size_t start_time;
  size_t end_time;
  if (!dictionary->GetValueForKeyAsInteger("start_time", start_time) ||
      !dictionary->GetValueForKeyAsInteger("end_time", end_time)) {
    MiscTelemetryInfo misc_info = MakeBaseEntry<MiscTelemetryInfo>();
    misc_info.meta_data["internal_errors"] =
        "Cannot determine start/end time from client entry";
    EmitToDestinations(&misc_info);
    return;
  }

  EmitToDestinations(&client_info);
}

void BasicTelemeter::LogCommandStart(llvm::StringRef uuid,
                                     llvm::StringRef original_command,
                                     TelemetryEventStats stats,
                                     Target *target_ptr) {

  lldb_private::CommandTelemetryInfo command_info =
      MakeBaseEntry<lldb_private::CommandTelemetryInfo>();

  // If we have a target attached to this command, then get the UUID.
  command_info.target_uuid = "";
  if (target_ptr && target_ptr->GetExecutableModule() != nullptr) {
    command_info.target_uuid =
        target_ptr->GetExecutableModule()->GetUUID().GetAsString();
  }
  command_info.command_uuid = uuid.str();
  command_info.original_command = original_command.str();
  command_info.stats = std::move(stats);

  EmitToDestinations(&command_info);
}

void BasicTelemeter::LogCommandEnd(llvm::StringRef uuid,
                                   llvm::StringRef command_name,
                                   llvm::StringRef command_args,
                                   TelemetryEventStats stats,
                                   Target *target_ptr,
                                   CommandReturnObject *result) {

  lldb_private::CommandTelemetryInfo command_info =
      MakeBaseEntry<lldb_private::CommandTelemetryInfo>();

  // If we have a target attached to this command, then get the UUID.
  command_info.target_uuid = "";
  if (target_ptr && target_ptr->GetExecutableModule() != nullptr) {
    command_info.target_uuid =
        target_ptr->GetExecutableModule()->GetUUID().GetAsString();
  }
  command_info.command_uuid = uuid.str();
  command_info.command_name = command_name.str();
  command_info.args = command_args.str();
  command_info.stats = std::move(stats);
  command_info.exit_description = {result->Succeeded() ? 0 : -1,
                                   StatusToString(result)};
  EmitToDestinations(&command_info);
}

llvm::StringRef parse_value(llvm::StringRef str, llvm::StringRef label) {
  return str.substr(label.size()).trim();
}

bool parse_field(llvm::StringRef str, llvm::StringRef label) {
  if (parse_value(str, label) == "true")
    return true;
  return false;
}

llvm::telemetry::TelemetryConfig *MakeTelemetryConfig() {
  bool enable_telemetry = false;
  std::vector<std::string> additional_destinations;

  // Look in the $HOME/.lldb_telemetry_config file to populate the struct
  llvm::SmallString<64> init_file;
  FileSystem::Instance().GetHomeDirectory(init_file);
  llvm::sys::path::append(init_file, ".lldb_telemetry_config");
  FileSystem::Instance().Resolve(init_file);
  if (llvm::sys::fs::exists(init_file)) {
    auto contents = llvm::MemoryBuffer::getFile(init_file, /*IsText*/ true);
    if (contents) {
      llvm::line_iterator iter =
          llvm::line_iterator(contents->get()->getMemBufferRef());
      for (; !iter.is_at_eof(); ++iter) {
        if (iter->starts_with("enable_telemetry:")) {
          enable_telemetry = parse_field(*iter, "enable_telemetry:");
        } else if (iter->starts_with("destination:")) {
          llvm::StringRef dest = parse_value(*iter, "destination:");
          if (dest == "stdout") {
            additional_destinations.push_back("stdout");
          } else if (dest == "stderr") {
            additional_destinations.push_back("stderr");
          } else {
            additional_destinations.push_back(dest.str());
          }
        }
      }
    } else {
      LLDB_LOG(GetLog(LLDBLog::Object), "Error reading config file at {0}",
               init_file.c_str());
    }
  }

  auto *ret = new llvm::telemetry::TelemetryConfig{enable_telemetry,
                                                   additional_destinations};
#ifdef HAS_VENDOR_TELEMETRY_CONFIG
  ApplyVendorSpecificConfigs(ret);
#endif
  return ret;
}

llvm::telemetry::TelemetryConfig *GetTelemetryConfig() {
  static llvm::telemetry::TelemetryConfig *config = MakeTelemetryConfig();
  return config;
}

std::shared_ptr<LldbTelemeter>
LldbTelemeter::CreateInstance(lldb_private::Debugger *debugger) {
  auto *config = GetTelemetryConfig();
  if (!config->enable_telemetry) {
    return NoOpTelemeter::CreateInstance(debugger);
  }

#ifdef HAS_VENDOR_TELEMETER
  return CreateVendorSpecificTelemeter(config);
#else
  return BasicTelemeter::CreateInstance(debugger);
#endif
}
} // namespace lldb_private

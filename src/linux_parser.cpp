#include "linux_parser.h"

#include <dirent.h>
#include <unistd.h>

#include <cstdio>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

using std::stof;
using std::string;
using std::to_string;
using std::vector;

//**************** System ***********************//
float LinuxParser::MemoryUtilization() {
  string mem_total_str, mem_total_val, mem_free_str, mem_free_val, line;
  std::ifstream stream(kProcDirectory + kMeminfoFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> mem_total_str >> mem_total_val;
    std::getline(stream, line);
    std::istringstream linestream2(line);
    linestream2 >> mem_free_str >> mem_free_val;
  }
  float total_mem = std::stof(mem_total_val);
  return (total_mem - std::stof(mem_free_val)) / total_mem;
}

long LinuxParser::UpTime() {
  string uptime_suspend, idle, line;
  std::ifstream stream(LinuxParser::kProcDirectory +
                       LinuxParser::kUptimeFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> uptime_suspend >> idle;
  }
  return std::stol(uptime_suspend);
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

int LinuxParser::TotalProcesses() {
  return stoi(LinuxParser::SystemStatValue("processes"));
}

int LinuxParser::RunningProcesses() {
  return stoi(LinuxParser::SystemStatValue("procs_running"));
}

string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

string LinuxParser::Kernel() {
  string os, version, kernel;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

//**************** CPU ***********************//
vector<long> LinuxParser::CpuUtilization() {
  string line, key, token;
  vector<long> CPUStatesUtil;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> key;
    for (int CPUStateIdx = 0; CPUStateIdx < kNumCPUStates_; ++CPUStateIdx) {
      linestream >> token;
      CPUStatesUtil.push_back(std::stol(token));
    }
  }
  return CPUStatesUtil;
}

long LinuxParser::Jiffies() {
  vector<long> utils = LinuxParser::CpuUtilization();
  long total_util = 0;
  for (auto util : utils) {
    total_util += util;
  }
  return total_util;
}

long LinuxParser::ActiveJiffies() {
  vector<long> utils = LinuxParser::CpuUtilization();
  return utils[kUser_] + utils[kNice_] + utils[kSystem_] + utils[kIRQ_] +
         utils[kSoftIRQ_] + utils[kSteal_];
}

long LinuxParser::ActiveJiffies(int pid) {
  long utime = std::stol(LinuxParser::ProcessStatValue(pid, 14));
  long stime = std::stol(LinuxParser::ProcessStatValue(pid, 15));
  long cutime = std::stol(LinuxParser::ProcessStatValue(pid, 16));
  long cstime = std::stol(LinuxParser::ProcessStatValue(pid, 17));
  return utime + stime + cutime + cstime;
}

long LinuxParser::IdleJiffies() {
  vector<long> utils = LinuxParser::CpuUtilization();
  return utils[kIdle_] + utils[kIOwait_];
}

//**************** Process Functions ***********************//
string LinuxParser::Command(int pid) {
  std::ifstream stream(kProcDirectory + std::to_string(pid) + kCmdlineFilename);
  string line;
  std::getline(stream, line);
  return line;
}

string LinuxParser::Ram(int pid) {
  return LinuxParser::ProcessStatusValue(pid, "VmSize");
}

string LinuxParser::Uid(int pid) {
  return LinuxParser::ProcessStatusValue(pid, "Uid");
}

string LinuxParser::User(int pid) {
  string uid = Uid(pid), line;
  std::ifstream stream(kPasswordPath);
  string user, permission, uid_maybe;
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      linestream >> user >> permission >> uid_maybe;
      if (uid_maybe == uid) {
        break;
      }
    }
  }
  return user;
}

long int LinuxParser::UpTime(int pid) {
  string proc_start_time_str = ProcessStatValue(pid, 22);
  long proc_start_time = std::stol(proc_start_time_str);
  long proc_start_time_s = proc_start_time / sysconf(_SC_CLK_TCK);
  long sys_uptime_s = UpTime(); 
  long proc_uptime_s = sys_uptime_s - proc_start_time_s;
  if (proc_uptime_s == 0) {
    return sys_uptime_s;
  }
  return proc_uptime_s;
}

//******************* Helper Functions ***********************//

// returns the first value for a given key in the PID status file
string LinuxParser::ProcessStatusValue(int pid, string key) {
  string line;
  string key_maybe;
  string value;
  std::ifstream stream(LinuxParser::kProcDirectory + std::to_string(pid) +
                       kStatusFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      linestream >> key_maybe;
      if (key_maybe.find(key) != string::npos) {
        linestream >> value;
        break;
      }
    }
  }
  if (value == "") {
    value = "0";
  }
  return value;
}

// returns the first value for a given key in the proc/stat file
string LinuxParser::SystemStatValue(string key) {
  string line, key_maybe, value;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      linestream >> key_maybe;
      if (key_maybe == key) {
        linestream >> value;
      }
    }
  }
  if (value == "") {
    value = "0";
  }
  return value;
}

// returns the value at token_position from the proc/[pid]/stat file
string LinuxParser::ProcessStatValue(int pid, int token_position) {
  string line, value;
  std::ifstream stream(kProcDirectory + std::to_string(pid) + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    int i = 0;
    while (i < token_position) {
      linestream >> value;
      ++i;
    }
  }
  if (value == "") {
    value = "0";
  }
  return value;
}

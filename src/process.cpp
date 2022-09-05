#include "process.h"

#include <unistd.h>

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "linux_parser.h"

using std::string;
using std::to_string;
using std::vector;

int Process::Pid() { return pid_; }

float Process::CpuUtilization() {
  long proc_uptime_s = LinuxParser::UpTime(pid_);
  long active_jiffies = LinuxParser::ActiveJiffies(Pid());
  return active_jiffies / sysconf(_SC_CLK_TCK) / proc_uptime_s * 100;
}

string Process::Command() { return LinuxParser::Command(pid_); }

string Process::Ram() {
  int pid = pid_;
  string ram_kb_str = LinuxParser::Ram(pid);
  long ram_kb = std::stol(ram_kb_str);
  string ram_mb_str = std::to_string(ram_kb / 1000);
  return ram_mb_str;
}

string Process::User() { return LinuxParser::User(pid_); }

long int Process::UpTime() { return LinuxParser::UpTime(pid_); }

bool Process::operator<(Process const& a) const {
  return LinuxParser::Ram(a.pid_) < LinuxParser::Ram(pid_);
}
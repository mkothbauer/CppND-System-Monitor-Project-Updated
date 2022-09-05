#include "processor.h"

#include "linux_parser.h"

float Processor::Utilization() {
  float total_jiffies = (float)LinuxParser::Jiffies();
  float idle_jiffies = (float)LinuxParser::IdleJiffies();
  return (total_jiffies - idle_jiffies) / total_jiffies;
}
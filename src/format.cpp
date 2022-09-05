#include "format.h"

#include <cstring>
#include <string>

using std::string;

static const long SEC_PER_HOUR = 3600;
static const long SEC_PER_MIN = 60;

string Format::ElapsedTime(long seconds_) {
  long hours = seconds_ / SEC_PER_HOUR;
  long minutes = (seconds_ - hours * SEC_PER_HOUR) / SEC_PER_MIN;
  long seconds = (seconds_ - hours * SEC_PER_HOUR - minutes * SEC_PER_MIN);

  string minutes_str = std::to_string(minutes);
  if (minutes < 10) {
    minutes_str = "0" + std::to_string(minutes);
  }

  string seconds_str = std::to_string(seconds);
  if (seconds < 10) {
    seconds_str = "0" + std::to_string(seconds);
  }

  return std::to_string(hours) + ":" + minutes_str + ":" + seconds_str;
}
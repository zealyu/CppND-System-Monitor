#include <pstl/pstl_config.h>
#include <unistd.h>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>
//#include <fstream>

#include "process.h"

using std::string;
using std::to_string;
using std::vector;

int Process::Pid() { return pid_; }

float Process::CpuUtilization() const { return LinuxParser::CpuUtilization(pid_); }

string Process::Command() { return LinuxParser::Command(pid_); }

string Process::Ram() { return LinuxParser::Ram(pid_); }

string Process::User() { return LinuxParser::User(pid_); }

long int Process::UpTime()
{
    long int uptime = LinuxParser::UpTime(pid_);
    return uptime;
}

bool Process::operator<(Process const& a) const 
{
    return a.CpuUtilization() < CpuUtilization();
}

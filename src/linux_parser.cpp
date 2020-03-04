#include <dirent.h>
#include <endian.h>
#include <unistd.h>
#include <exception>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "linux_parser.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;

// DONE: An example of how to read data from the filesystem
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

// DONE: An example of how to read data from the filesystem
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

float LinuxParser::MemoryUtilization() { 
    string key, line;
    double memTotal = 0, memFree = 0, value;
    std::ifstream filestream(kProcDirectory + kMeminfoFilename);
    if(filestream.is_open()) {
        while (!(memTotal && memFree) && std::getline(filestream, line)) {
            std::istringstream linestream(line);
            linestream >> key >> value;
            if(key == "MemTotal:")
                memTotal = value;
            if(key == "MemFree:")
                memFree = value;
        }
    }

    return (memTotal - memFree) / memTotal; 
}

long LinuxParser::UpTime() {
    long uptime = 0;
    string line;
    std::ifstream filestream(kProcDirectory + kUptimeFilename);
    if (filestream.is_open()) {
        std::getline(filestream, line);
        std::istringstream linestream(line);
        linestream >> uptime;
    }
    return uptime;
}

long LinuxParser::Jiffies() { 
    long user, nice, system, idle, iowait, irq, softirq, steal, guest, guestnice; 
    long usertime, nicetime, idlealltime, systemalltime, virtalltime, totaltime;
    string line, cpu;
    std::ifstream stream(kProcDirectory + kStatFilename);
    if (stream.is_open()) {
        std::getline(stream, line);
        std::istringstream linestream(line);
        linestream >> cpu >> user >> nice >> system >> idle >> iowait \
            >> irq >> softirq >> steal >>guest >> guestnice;
        usertime = user - guest;
        nicetime = nice - guestnice;
        idlealltime = idle + iowait;
        systemalltime = system + irq + softirq;
        virtalltime = guest + guestnice;
        totaltime = usertime + nicetime + systemalltime + idlealltime \
                    + steal + virtalltime;
    }
    return totaltime;
}

long LinuxParser::ActiveJiffies(int pid) {
    string line, ignore;
    long utime, stime, cutime, cstime;
    std::ifstream stream(kProcDirectory + to_string(pid) + kStatFilename);
    if(stream.is_open()) {
        std::getline(stream, line);
        std::istringstream linestream(line);
        for(int i = 0; i < 13; ++i)
            linestream >> ignore;
        linestream >> utime >> stime >> cutime >> cstime;
        for(int i = 0; i < 4; ++i)
            linestream >> ignore;
        return utime + stime + cutime + cstime; 
    }
    return 0;
}

long LinuxParser::ActiveJiffies() {
    long userJiffies, niceJiffies, systemJiffies;
    string line, lineHead;
    std::ifstream stream(kProcDirectory + kStatFilename);
    if (stream.is_open()) {
        std::getline(stream, line);
        std::istringstream linestream(line);
        linestream >> lineHead >> userJiffies >> niceJiffies >> systemJiffies;
    }
    return userJiffies + niceJiffies + systemJiffies;
}

long LinuxParser::IdleJiffies() {
    long user, nice, system, idle, iowait, irq, softirq, steal, guest, guestnice; 
    long idlealltime = 0;
    string line, cpu;
    std::ifstream stream(kProcDirectory + kStatFilename);
    if (stream.is_open()) {
        std::getline(stream, line);
        std::istringstream linestream(line);
        linestream >> cpu >> user >> nice >> system >> idle >> iowait \
            >> irq >> softirq >> steal >>guest >> guestnice;
        idlealltime = idle + iowait;
    }
    return idlealltime;
}

vector<string> LinuxParser::CpuUtilization() { 
    long totalJiffies = Jiffies();
    long idleJiffies = IdleJiffies();
    float ut = 1.0 * (totalJiffies - idleJiffies) / totalJiffies;
    return vector<string>{to_string(ut)}; 
}


float LinuxParser::CpuUtilization(int pid)
{
    string line, ignore;
    long utime, stime, cutime, cstime, starttime, totalCpuTime;
    std::ifstream stream(kProcDirectory + to_string(pid) + kStatFilename);
    if(stream.is_open()) {
        std::getline(stream, line);
        std::istringstream linestream(line);
        for(int i = 0; i < 13; ++i)
            linestream >> ignore;
        linestream >> utime >> stime >> cutime >> cstime;
        for(int i = 0; i < 4; ++i)
            linestream >> ignore;
        linestream >> starttime;
        totalCpuTime = (utime + stime + cutime + cstime)/sysconf(_SC_CLK_TCK);
        return 1.0*totalCpuTime / (LinuxParser::UpTime() - starttime/sysconf(_SC_CLK_TCK));
    }
    return 0;
}

int LinuxParser::TotalProcesses() {
    string line, key;
    int numProcess = 0;
    std::ifstream filestream(kProcDirectory + kStatFilename);
    if (filestream.is_open()) 
        while(!numProcess && std::getline(filestream, line)) {
            std::istringstream linestream(line);
            linestream >> key;
            if (key == "processes")
                linestream >> numProcess;
        }

    return numProcess; 
}

int LinuxParser::RunningProcesses() {
    string line, key;
    int numProcessRunning = 0;
    std::ifstream filestream(kProcDirectory + kStatFilename);
    if (filestream.is_open()) 
        while(!numProcessRunning && std::getline(filestream, line)) {
            std::istringstream linestream(line);
            linestream >> key;
            if (key == "procs_running")
                linestream >> numProcessRunning;
        }

    return numProcessRunning; 
}

string LinuxParser::Command(int pid) {
    string cmdLine;
    std::ifstream filestream(kProcDirectory + to_string(pid) + kCmdlineFilename);
    if (filestream.is_open())
        std::getline(filestream, cmdLine);

    return cmdLine; 
}

string LinuxParser::Ram(int pid) {
    string line, key;
    long ram = 0;
    std::ifstream filestream(kProcDirectory + to_string(pid) + kStatusFilename);
    if (filestream.is_open()) 
        while(!ram && std::getline(filestream, line)) {
            std::istringstream linestream(line);
            linestream >> key;
            if (key == "VmSize:")
                linestream >> ram;
        }
    return to_string(ram/1024);
}

string LinuxParser::Uid(int pid) {
    string line, key, uid;
    std::ifstream filestream(kProcDirectory + to_string(pid) + kStatusFilename);
    if (filestream.is_open()) 
        while(uid.empty() && std::getline(filestream, line)) {
            std::istringstream linestream(line);
            linestream >> key;
            if (key == "Uid:")
                linestream >> uid;
        }

    return uid;
}

string LinuxParser::User(int pid) {
    string usrName, ignore, line, currentUid;
    string uid = LinuxParser::Uid(pid);
    std::ifstream filestream(kPasswordPath);
    if (filestream.is_open())
        while( currentUid != uid && std::getline(filestream, line)) {
            std::replace(line.begin(), line.end(), ':', ' ');
            std::istringstream linestream(line);
            linestream >> usrName >> ignore >> currentUid;
        }

    return usrName; 
}

long LinuxParser::UpTime(int pid) {
    string line, ignore;
    long int uptime = 0;
    std::ifstream stream(kProcDirectory + to_string(pid) + kStatFilename);
    if(stream.is_open()) {
        std::getline(stream, line);
        std::istringstream linestream(line);
        for(int i = 0; i < 21; ++i)
            linestream >> ignore;
        linestream >> uptime;
    }
    return uptime; 
}

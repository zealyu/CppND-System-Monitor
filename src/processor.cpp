#include "processor.h"
#include <string>


float Processor::Utilization() 
{ 
    return std::stof(LinuxParser::CpuUtilization()[0]); 
}

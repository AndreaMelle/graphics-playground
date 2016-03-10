#include <iostream>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include "../include/cl.hpp"
#else
#include <CL/cl.h>
#include <CL/cl.hpp>
#endif

int main(int argc, const char * argv[])
{
    /*
     * Query platforms and platform info
     */
    
    cl_int clStatus = 0;
    std::vector<cl::Platform> platforms;
    clStatus = cl::Platform::get(&platforms);
    
    std::string name, vendor, version, profile, extenstions;
    std::vector<cl::Device> devices;
    std::vector<cl::Device>::iterator it_device;
    
    std::vector<cl::Platform>::iterator it;
    for(it = platforms.begin(); it != platforms.end(); ++it)
    {
        name = it->getInfo<CL_PLATFORM_NAME>();
        vendor = it->getInfo<CL_PLATFORM_VENDOR>();
        version = it->getInfo<CL_PLATFORM_VERSION>();
        profile = it->getInfo<CL_PLATFORM_PROFILE>();
        extenstions = it->getInfo<CL_PLATFORM_EXTENSIONS>();
        
        devices.clear();
        clStatus = it->getDevices(CL_DEVICE_TYPE_ALL, &devices);
        
        std::cout <<
            "Platform report:\n" <<
            "   name: "         << name << "\n"
            "   vendor: "       << vendor << "\n"
            "   version: "      << version << "\n"
            "   profile: "      << profile << "\n"
            "   extensions: "   << extenstions << "\n";
        
        for(it_device = devices.begin(); it_device != devices.end(); ++it_device)
        {
            std::string deviceAvailable = it_device->getInfo<CL_DEVICE_AVAILABLE>() ? "true" : "false";
            std::string compilerAvailable = it_device->getInfo<CL_DEVICE_COMPILER_AVAILABLE>() ? "true" : "false";
            
            std::cout <<
            "   Devices:\n" <<
            "       name: "                         << it_device->getInfo<CL_DEVICE_NAME>() << "\n"
            "       vendor: "                       << it_device->getInfo<CL_DEVICE_VENDOR>() << "\n"
            "       extensions: "                   << it_device->getInfo<CL_DEVICE_EXTENSIONS>() << "\n"
            "       global mem size: "              << it_device->getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << "\n"
            "       address bits: "                 << it_device->getInfo<CL_DEVICE_ADDRESS_BITS>() << "\n"
            "       device available: "             << deviceAvailable << "\n"
            "       device compiler available: "    << compilerAvailable << "\n";
        }
        
    }
    
    
    return 0;
}

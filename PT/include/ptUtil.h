#ifndef ptUtil_h
#define ptUtil_h

#include <iostream>
#include <ostream>
#include <exception>
#include "glm/glm.hpp"
#include <fstream>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include "../include/cl.hpp"
#else
#include <CL/cl.h>
#include <CL/cl.hpp>
#endif

#define PT_PRECISION glm::highp
#define PT_EPSILON 1e-4



//void PTCL_ASSERT(const cl_int& status, const std::string& errorMsg = "")
//{
//    if (status != CL_SUCCESS)
//    {
//        std::cerr << errorMsg << "\n";
//        exit(EXIT_FAILURE);
//    }
//}

namespace pt
{
    template <typename T> using ptvec = glm::tvec3<T, PT_PRECISION>;
    
    /* Clamps a value between [0, 1] */
    template <typename T>
    T clamp(T x) { return x < 0 ? 0 : x > 1 ? 1 : x; }
    
    /* scale value to [0, 255] with clamp and gamma correction */
    template <typename T>
    int to255Gamma(T x) { return int(pow(clamp<T>(x), 1./2.2) * 255. + 0.5); }
    
    /* scale value to [0, 255] with clamp */
    template <typename T>
    int to255Linear(T x) { return int(clamp<T>(x) * 255. + 0.5); }
    
    template <typename T>
    int to255(T x) { return int(x * 255. + 0.5); }
    
    //TODO: make it with masks
    typedef enum BufferTransformOptions
    {
        BUFFER_TRANSFORM_NONE, // does not modify the input
        BUFFER_TRANSFORM_255_GAMMA, // clamps the input to [0,1] range, performs gamma correction and scale to [0,255]
        BUFFER_TRANSFORM_255_CLAMP, // clamps the input to [0,1] range and scale to [0,255]
        BUFFER_TRANSFORM_255, // scale to [0,255] with no clamping
    } BufferTransformOptions;
    
    /*
     * Write image buffer to .ppm file with optional rescaling to [0, 255] and ignoring channels after 3rd
     */
    template <typename T>
    void write_ppm(const T*                 buffer,
                   unsigned int             width,
                   unsigned int             height,
                   unsigned int             channels,
                   BufferTransformOptions   bufferTransform,
                   const std::string&       filename)
    {
        if(!(channels == 1 || channels >= 3)) throw "Invalid number of channels.";
        
        std::function<int(T)> scaler;
        
        switch (bufferTransform)
        {
            case BUFFER_TRANSFORM_NONE:
                scaler = std::function<int(T)>([](T arg){ return arg; });
                break;
            case BUFFER_TRANSFORM_255:
                scaler = to255<T>;
                break;
            case BUFFER_TRANSFORM_255_CLAMP:
                scaler = to255Linear<T>;
                break;
            case BUFFER_TRANSFORM_255_GAMMA:
                scaler = to255Gamma<T>;
                break;
            default:
                break;
        }
        
        std::ofstream file;
        file.open(filename);
        
        unsigned int size = channels * width * height;
        
        file << "P3\n" << width << " " << height << "\n" << 255 << "\n";
        
        for (unsigned int i = 0; i < size; i+=channels)
        {
            if (channels == 1)
            {
                int val = (int)(scaler(buffer[i]));
                file << val << " " << val << " " << val << " ";
            }
            else
            {
                file << (int)(scaler(buffer[i + 0])) << " "
                << (int)(scaler(buffer[i + 1])) << " "
                << (int)(scaler(buffer[i + 2])) << " ";
            }
        }
        
        file.close();
    }
    
    template <typename T>
    void write_ppm(const T*             buffer,
                   unsigned int         width,
                   unsigned int         height,
                   unsigned int         channels,
                   bool                 normalize255,
                   bool                 gamma,
                   const std::string&   filename)
    {
        if(!(channels == 1 || channels >= 3)) throw "Invalid number of channels.";
        
        std::function<int(T)> scaler;
        
        if (normalize255)
        {
            if (gamma)
                scaler = to255Gamma<T>;
            else
                scaler = to255Linear<T>;
        }
        else
            scaler = std::function<int(T)>([](T arg){
                return arg;
            });
        
        std::ofstream file;
        file.open(filename);
        
        unsigned int size = channels * width * height;
        
        file << "P3\n" << width << " " << height << "\n" << 255 << "\n";
        
        for (unsigned int i = 0; i < size; i+=channels)
        {
            if (channels == 1)
            {
                int val = (int)(scaler(buffer[i]));
                file << val << " " << val << " " << val << " ";
            }
            else
            {
                file << (int)(scaler(buffer[i + 0])) << " "
                     << (int)(scaler(buffer[i + 1])) << " "
                     << (int)(scaler(buffer[i + 2])) << " ";
            }
        }
        
        file.close();
    }
    
    template<typename T>
    ptvec<T> Color_RGB8_to_RGB(unsigned char r, unsigned char g, unsigned char b)
    {
        return ptvec<T>((T)r/255.0, (T)g/255.0, (T)b/255.0);
    }
    
    template<typename T>
    ptvec<T> ColorHex_to_RGBfloat(const std::string& hex_color)
    {
        unsigned int x = static_cast<unsigned int>(std::stoul(hex_color, nullptr, 16));
        
        return Color_RGB8_to_RGB<T>((unsigned char)(x >> 16 & 0xFF),
                                         (unsigned char)(x >> 8 & 0xFF),
                                         (unsigned char)(x  & 0xFF));
    }
    
}

#endif /* ptUtil_h */

#ifndef ptUtil_h
#define ptUtil_h

#include <iostream>
#include <ostream>
#include <exception>
#include "glm/glm.hpp"
#include <fstream>

#define PT_PRECISION glm::highp
#define PT_EPSILON 1e-4

namespace pt
{
    template <typename T> using ptvec = glm::tvec3<T, PT_PRECISION>;
    
    /* Clamps a value between [0, 1] */
    template <typename T>
    T clamp(T x) { return x < 0 ? 0 : x > 1 ? 1 : x; }
    
    /* scale value to [0, 255] with clamp and gamma correction */
    template <typename T>
    int to255(T x) { return int(pow(clamp<T>(x), 1./2.2) * 255. + 0.5); }
    
    /* scale value to [0, 255] with clamp */
    template <typename T>
    int to255Linear(T x) { return int(clamp<T>(x) * 255. + 0.5); }
    
    /*
     * Write image buffer to .ppm file with optional rescaling to [0, 255] and ignoring channels after 3rd
     */
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
                scaler = to255<T>;
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
}

#endif /* ptUtil_h */

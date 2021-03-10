#ifndef __OURGL_H__
#define __OURGL_H__

#include <array>

#include "geometry.h"
#include "model.h"


/**
 * match unit square onto the image with width and height.
 * you can read this get more: https://github.com/ssloy/tinyrenderer/wiki/Lesson-5-Moving-the-camera 
*/
void viewport(const int x, const int y, const int w, const int h);


/**
 * generate perpective projection, the coaff is the -1/c
 * reference: https://github.com/ssloy/tinyrenderer/wiki/Lesson-4-Perspective-projection 
*/
void projection(const float coeff = 0.0);


/**
 * transform model which is in world coordinates into camera(eye) coordinats
 * @param eye the camera(eye) location in world
 * @param center the lens centric position
 * @param up the camera up directory
 * you can get more from this: https://github.com/ssloy/tinyrenderer/wiki/Lesson-5-Moving-the-camera  
*/
void lookat(const vec3f& eye, const vec3f& center, const vec3f& up);


class Shader
{
public:

    /**
     * Vertex shader, it will return the coordinates appeared in screnn before perspective.
     * @param iface nth face in mesh
     * @param nthvert the nth vertex in face
     * @return the coordinates appeared in screnn before perspective
    */
    virtual vec4f vertex(const int iface, const int nthvert) = 0;

    /**
     * Calculating pixel color with barycentric coordinates
     * @param bar the barycentric coordinates with triangle, you'd better let it keep original
     * @param color the color of pixel will be returned if we don't it
     * @return if we discard this pixle, true: Yes, false: No
    */
    virtual bool fragment(const vec3f& bar, TGAColor& color) = 0;

};

/**
 * rasterize triangle 
 * @param clipVerts the vertex of triangle without perspective 
 * @param shader shader the vertex and pixel
 * @param image the image will be output
 * @param zBuffer zBuffer about removeable pixel
*/
void triangle(const std::array<vec4f, 3>& clipVerts, Shader& shader, TGAImage& image, std::vector<float>& zBuffer);

#endif
#ifndef __Model_H__
#define __Model_H__

#include<vector>
#include<string>

#include "geometry.h"
#include "tgaimage.h"

class Model
{
public:
    std::vector<vec3f> verts_; // array of vertices
    std::vector<vec2f> uv_; // array of texture coordinates
    std::vector<vec3f> norms_; // array of normal vectors
    
    std::vector<int> facet_vrt_; // indices in abover arrays of per triangle
    std::vector<int> facet_tex_; 
    std::vector<int> facet_nrm_;

    TGAImage diffusemap_; // diffuse color texture
    TGAImage normalmap_; // normal map texture
    TGAImage specularmap_; // specular map texture

    void load_texture(const std::string& filename, const std::string& suffix, TGAImage& image);
public:
    Model(const std::string filename);
    ~Model();

    int nverts() const;
    int nfaces() const;
    vec3f normal(const int iface, const int nthvert) const; // per trangle vertex normal
    vec3f normal(const vec2f& uv) const; // fetch the normal vector from normal texture map
    vec3f vert(const int i) const;
    vec3f vert(const int iface, const int nthvert) const;
    vec2f uv(const int iface, const int nthvert) const;
    TGAColor diffuse(const vec2f& uv) const;
    double specular(const vec2f& uv) const;
};



#endif
#include "model.h"

#include<iostream>
#include<fstream>
#include<sstream>

Model::Model(const std::string filename)
    :verts_(), uv_(), norms_(), facet_vrt_(), facet_nrm_(), facet_tex_(), 
    diffusemap_(), normalmap_(), specularmap_() 
{   
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if(in.fail()) return;
    std::string line;
    while(!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if(!line.compare(0, 2, "v ")) {
            iss >> trash;
            vec3f v;
            for(int i = 0; i < 3; i++) iss >> v[i];
            verts_.push_back(v);
        } else if(!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            vec3f n;
            for(int i = 0; i < 3; i++) iss >> n[i];
            norms_.push_back(n.normalize());
        } else if(!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            vec2f uv;
            for(int i = 0; i < 2; i++) iss >> uv[i];
            uv_.push_back(uv);
        } else if(!line.compare(0, 2, "f ")) {
            iss >> trash;
            int v, t, n;
            int cnt = 0;
            while(iss >> v >> trash >> t >> trash >> n) {
                facet_vrt_.push_back(--v);
                facet_tex_.push_back(--t);
                facet_nrm_.push_back(--n);
                cnt++;
            }
            if(cnt != 3) {
                std::cerr << "Error: the obj file is supposed be triangled" << std::endl;
                in.close();
                return;
            }

        }
    }

    in.close();
    std::cerr << "# v# " << nverts() << " f# "  << nfaces() << " vt# " << uv_.size() << " vn# " << norms_.size() << std::endl;
    load_texture(filename, "_diffuse.tga", diffusemap_);
    load_texture(filename, "_nm.tga", normalmap_);
    load_texture(filename, "_spec.tga", specularmap_);
}
Model::~Model()
{
}


void Model::load_texture(const std::string& filename, const std::string& suffix, TGAImage& image) {
    std::size_t dot = filename.find_last_of(".");
    if(dot == std::string::npos) return;
    std::string tex_file = filename.substr(0, dot) + suffix;
    std::cerr << "texture file " << tex_file << "is loading... " << (image.read_tga_file(tex_file)? "OK" : "Error") << std::endl;
    image.flip_vertically();
}

int Model::nverts() const {
    return verts_.size();
}

int Model::nfaces() const {
    return facet_vrt_.size() / 3;
}

vec3f Model::normal(const int iface, const int nthvert) const {
    return norms_[facet_nrm_[iface * 3 + nthvert]];
}

vec3f Model::normal(const vec2f& uv) const {
    TGAColor color = normalmap_.get(uv[0] * normalmap_.get_width(), uv[1] * normalmap_.get_height());
    vec3f ret;
    for(int i = 0; i < 3; i++) {
        ret[2 - i] = color[i] / 255.0 * 2 - 1;
    }
    return ret;
}

vec3f Model::vert(const int i) const {
    return verts_[i];
}

vec3f Model::vert(const int iface, const int nthvert) const {
    return verts_[facet_vrt_[iface * 3 + nthvert]];
}

vec2f Model::uv(const int iface, const int nthvert) const {
    return uv_[facet_vrt_[iface * 3 + nthvert]];
}

TGAColor Model::diffuse(const vec2f& uv) const {
    return diffusemap_.get(uv[0] * diffusemap_.get_width(), uv[1] * diffusemap_.get_height());
}

double Model::specular(const vec2f& uv) const {
    return specularmap_.get(uv[0] * specularmap_.get_width(), uv[1] * specularmap_.get_height())[0];
}
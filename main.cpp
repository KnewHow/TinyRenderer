#include <iostream>
#include<vector>
#include<limits>

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "ourGL.h"

constexpr int width = 1024;
constexpr int height = 1024;

extern mat4f ModelView;
extern mat4f Viewport;
extern mat4f Projection;


const vec3f lightDir(1.0f, 1.0f, 1.0f);
const vec3f eye(1.0f, 1.0f, 3.0f);
const vec3f center(0.0f, 0.0f, 0.0f);
const vec3f up(0.0f, 1.0f, 0.0f);

class IShader: public Shader {
    const Model& model;
    vec3f light; // light directory normalized in camera coordinates
    mat<float, 2, 3> varying_uv; //  triangle uv coordinates, written by vertex shader, read by fragment shader
    mat3f varying_nrm; // normal of per vertex of triangle
    mat3f ndc_tri; // vertex with homogenous coordinates in triangle

public:    
    IShader(const Model& m): model(m){
        light = (proj<float, 3>(Projection * ModelView * embed<float, 4>(lightDir, 0.0f))).normalize(); // tramsform lightDir into camera coordinates
    }

    
    virtual vec4f vertex(const int iface, const int nthvert) override {
        varying_uv.set_col(nthvert, model.uv(iface, nthvert));
        varying_nrm.set_col(nthvert, proj<float, 3>((Projection * ModelView).invert_transpose() * embed<float, 4>(model.normal(iface, nthvert), 0.0f))); // transform normal, reference: https://github.com/ssloy/tinyrenderer/wiki/Lesson-5-Moving-the-camera
        vec4f glVertex = Projection * ModelView * embed<float, 4>(model.vert(iface, nthvert));
        ndc_tri.set_col(nthvert, proj<float, 3>(glVertex/glVertex[3]));
        return glVertex;
    }

    virtual bool fragment(const vec3f& bar, TGAColor& color) override {
        vec3f bn = (varying_nrm * bar).normalize();
        vec2f uv = varying_uv * bar;
        // use tangent space normal texture, you can read by: https://github.com/ssloy/tinyrenderer/wiki/Lesson-6bis-tangent-space-normal-mapping
        mat3f AI = mat3f{
            {
                ndc_tri.col(1) - ndc_tri.col(0),
                ndc_tri.col(2) - ndc_tri.col(0),
                bn
            }
        }.invert();
        vec3f i = AI * vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0.0f);
        vec3f j = AI * vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0.0f);
        
        mat3f B = mat3f{ {i.normalize(), j.normalize(), bn} }.transpose();
        vec3f n = (B * model.normal(uv)).normalize(); // get normal from tangent space
        vec3f r = (n * (n * light) * 2 - light).normalize(); // Phone shading， https://github.com/ssloy/tinyrenderer/wiki/Lesson-6-Shaders-for-the-software-renderer
        float specular = std::pow(std::max(r.z, 0.0f), 5 + model.specular(uv));
        float diffuse = std::max(0.0f, n * light);
        float ambient = 10;
        TGAColor c = model.diffuse(uv);
        color = c;
        for(int i = 0; i < 3; i++) {
            color[i] = std::min<int>((ambient + c[i] * (diffuse + specular)), 255);
        }
        return false;
    }
};

int main(int, char**) {

    std::vector<std::string> modelPaths = {
        "../obj/diablo3_pose/diablo3_pose.obj",
        "../obj/floor.obj"

    };
    std::vector<float> zBuffer(width * height, -std::numeric_limits<float>::max());
    TGAImage image(width, height, TGAImage::RGB);
    lookat(eye, center, up);
    viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    projection(-1.0f/(eye - center).norm());

    for(const auto& path: modelPaths) {
        Model m(path);
        IShader shader(m);
        for(int i = 0; i < m.nfaces(); i++) {
            std::array<vec4f, 3> clipVerts = {};
            for(int j = 0; j < 3; j++) {
                clipVerts[j] = shader.vertex(i, j);
            }
            triangle(clipVerts, shader, image, zBuffer);
        }
    }
    
    image.write_tga_file("result.tga");
}


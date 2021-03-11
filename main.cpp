#include <iostream>
#include<vector>
#include<limits>
#include <ctime>

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "ourGL.h"

constexpr int width = 1024;
constexpr int height = 1024;

extern mat4f ModelView;
extern mat4f Viewport;
extern mat4f Projection;


const vec3f lightDir = vec3f(1.0f, 1.0f, 0.0f).normalize();
const vec3f eye(1.0f, 1.0f, 4.0f);
const vec3f center(0.0f, 0.0f, 0.0f);
const vec3f up(0.0f, 1.0f, 0.0f);

class IShader: public Shader {
private:
    const Model& model;
    const std::vector<float>& shadowBuffer;
    vec3f light; // light directory normalized in camera coordinates
    mat<float, 2, 3> varying_uv; //  triangle uv coordinates, written by vertex shader, read by fragment shader
    mat3f varying_nrm; // normal of per vertex of triangle
    mat3f ndc_tri; // vertex with homogenous coordinates in triangle

public:    
    IShader(const Model& m, const std::vector<float>& shadow): model(m), shadowBuffer(shadow){
        light = (proj<float, 3>(Projection * ModelView * embed<float, 4>(lightDir, 0.0f))).normalize(); // tramsform lightDir into camera coordinates
    }

    
    virtual vec4f vertex(const int iface, const int nthvert) override {
        varying_uv.set_col(nthvert, model.uv(iface, nthvert));
        varying_nrm.set_col(nthvert, proj<float, 3>((Projection * ModelView).invert_transpose() * embed<float, 4>(model.normal(iface, nthvert), 0.0f))); // transform normal, reference: https://github.com/ssloy/tinyrenderer/wiki/Lesson-5-Moving-the-camera
        vec4f glVertex = Projection * ModelView * embed<float, 4>(model.vert(iface, nthvert));
        ndc_tri.set_col(nthvert, proj<float, 3>(glVertex/glVertex[3]));
        return glVertex;
    }

    virtual bool fragment(const vec3f& bar, TGAColor& color, float x, float y) override {
        vec3f bn = (varying_nrm * bar).normalize();
        vec2f uv = varying_uv * bar;
        int idx = x + y * width;
        float shadow =  0.45 + shadowBuffer[idx] * 2.2;
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
        float ambient = 20;
        float Kd = 1.6;
        float Ks = 0.6;
        TGAColor c = model.diffuse(uv);
        color = c;
        for(int i = 0; i < 3; i++) {
            color[i] = std::min<int>((ambient + c[i] * shadow * (Kd * diffuse + Ks * specular)), 255);
        }
        return false;
    }
};

class DepthShader: public Shader {
    const Model& model;
    mat3f varying_tri;

public:
    DepthShader(const Model& m)
        : varying_tri(), model(m) {}

    virtual vec4f vertex(const int iface, const int nthvert) override {
        vec4f glVertex = embed<float, 4>(model.vert(iface, nthvert));
        glVertex = Projection * ModelView * glVertex;
        varying_tri.set_col(nthvert, proj<float,3>(glVertex/ glVertex[3]));
        return glVertex;
    }

    virtual bool fragment(const vec3f& bar, TGAColor& color, float x, float y) override {
        vec3f p = varying_tri * bar;
        color = TGAColor(255, 255, 255) * (p.z);
        return false;
    }
};


int main(int, char**) {

    std::vector<std::string> modelPaths = {
        "../obj/diablo3_pose/diablo3_pose.obj",
        "../obj/floor.obj"
    };
    std::vector<float> zBuffer(width * height, -std::numeric_limits<float>::max());
    std::vector<float> depthShadow(width * height, -std::numeric_limits<float>::max());
    TGAImage depthImage(width, height, TGAImage::RGB);
    TGAImage image(width, height, TGAImage::RGB);
    
    time_t beginRender = std::time(nullptr);
    // render shadow
    {
        lookat(lightDir, center, up);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(0);
        for(const auto& path: modelPaths) {
            Model m(path);
            DepthShader depthShader(m);
            for(int i = 0; i < m.nfaces(); i++) {
                std::array<vec4f, 3> clipVerts = {};
                for(int j = 0; j < 3; j++) {
                    clipVerts[j] = depthShader.vertex(i, j);
                }
                triangle(clipVerts, depthShader, depthImage, depthShadow);
            }
        }
        for(int i =0; i < width; i++) {
            for(int j = 0; j < height; j++) {
                depthShadow[i + j * width] = depthImage.get(i, j).bgra[0]/255.0f;
            }
        }
    }

    // rendering object
    {
        lookat(lightDir, center, up);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(-1.0f/(eye - center).norm());

        for(const auto& path: modelPaths) {
            Model m(path);
            IShader shader(m, depthShadow);
            for(int i = 0; i < m.nfaces(); i++) {
                std::array<vec4f, 3> clipVerts = {};
                for(int j = 0; j < 3; j++) {
                    clipVerts[j] = shader.vertex(i, j);
                }
                triangle(clipVerts, shader, image, zBuffer);
            }
        }

       
    }
    time_t endRender = std::time(nullptr);
    std::cout << "Render took: " << (endRender - beginRender) * 1000 << " millseconds" << std::endl;
    image.write_tga_file("result.tga");
    depthImage.write_tga_file("depth_image.tag");
}


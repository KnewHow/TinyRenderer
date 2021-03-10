#include <iostream>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

#include<vector>
#include<limits>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0, 0,   255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue  = TGAColor(0, 0, 255, 255);
const int width = 1024;
const int height = 1024;
TGAImage image(width, height, TGAImage::RGB);
float* z_buffer = new float[width * height];

std::vector<vec2f> MSAA_4x4 = {
    vec2f(0.25, 0.25),
    vec2f(0.75, 0.25),
    vec2f(0.25, 0.75),
    vec2f(0.75, 0.75),
};



void line(int x0, int y0, int x1, int y1, TGAImage& image, const TGAColor& color) {
    bool steep = false; 
    if (std::abs(x0-x1)<std::abs(y0-y1)) { 
        std::swap(x0, y0); 
        std::swap(x1, y1); 
        steep = true; 
    } 
    if (x0>x1) { 
        std::swap(x0, x1); 
        std::swap(y0, y1); 
    } 
    int dx = x1-x0; 
    int dy = y1-y0; 
    int derror2 = std::abs(dy)*2; 
    int error2 = 0; 
    int y = y0; 
    for (int x=x0; x<=x1; x++) { 
        if (steep) { 
            image.set(y, x, color); 
        } else { 
            image.set(x, y, color); 
        } 
        error2 += derror2; 
        if (error2 > dx) { 
            y += (y1>y0?1:-1); 
            error2 -= dx*2; 
        } 
    }
}

void line(const vec2i& v1, const vec2i& v2, TGAImage& image, const TGAColor& color) {
    line(v1.x, v1.y, v2.x, v2.y, image, color);
}

void triangle_old(vec2i v0, vec2i v1, vec2i v2, TGAImage& image, const TGAColor& color) {
    if (v0.y==v1.y && v0.y==v2.y) return; // I dont care about degenerate triangles 
    // sort the vertices, t0, t1, t2 lower−to−upper (bubblesort yay!) 
    if (v0.y>v1.y) std::swap(v0, v1); 
    if (v0.y>v2.y) std::swap(v0, v2); 
    if (v1.y>v2.y) std::swap(v1, v2); 
    int total_height = v2.y-v0.y; 
    for (int i=0; i<total_height; i++) { 
        bool second_half = i>v1.y-v0.y || v1.y==v0.y; 
        int segment_height = second_half ? v2.y-v1.y : v1.y-v0.y; 
        float alpha = (float)i/total_height; 
        float beta  = (float)(i-(second_half ? v1.y-v0.y : 0))/segment_height; // be careful: with above conditions no division by zero here 
        vec2i A =               v0 + (v2-v0)*alpha; 
        vec2i B = second_half ? v1 + (v2-v1)*beta : v0 + (v1-v0)*beta; 
        if (A.x>B.x) std::swap(A, B); 
        for (int j=A.x; j<=B.x; j++) { 
            image.set(j, v0.y+i, color); // attention, due to int casts t0.y+i != A.y 
        } 
    } 
}


vec3f barycentric(const vec3f& v0, const vec3f& v1, const vec3f& v2, const vec3f& p) {
    float T = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);
    float alpha = ((float)(v1.y - v2.y) * (p.x - v2.x) + (v2.x - v1.x) * (p.y - v2.y)) / T;
    float beta = ((float)(v2.y - v0.y) * (p.x - v2.x) + (v0.x - v2.x) * (p.y - v2.y)) / T;
    float gamma = 1 - alpha - beta;
    return vec3f(alpha, beta, gamma);
}

bool isInnerTriangle(const vec3f& v0, const vec3f& v1, const vec3f& v2, const vec3f& p) {
    vec2f AB = (v1 - v0).to2d();
    vec2f AP = (p - v0).to2d();
    vec2f BC =  (v2 - v1).to2d();
    vec2f BP = (p - v1).to2d();
    vec2f CA = (v0 - v2).to2d();
    vec2f CP =  (p - v2).to2d();
    float r1 = AB.x * AP.y - AP.x * AB.y;
    float r2 = BC.x * BP.y - BP.x * BC.y;
    float r3 = CA.x * CP.y - CP.x * CA.y;
    if(r1 > 0 && r2 > 0 && r3 > 0) {
        return true;
    }
    return false;
}

vec3f world2Screen(const vec3f& world) {
    return vec3f((world.x + 1) * width / 2, (world.y + 1) * height / 2, world.z);
}

void triangle(const std::vector<vec3f>& vs, const std::vector<vec2f>& uvs, const Model* model, const float intensity, bool withMASS = false) {
    int minX = std::max(0.0f, std::min(vs[2].x, std::min(vs[0].x, vs[1].x)));
    int maxX = std::min(width * 1.0f, std::max(vs[2].x, std::max(vs[0].x, vs[1].x)));
    int minY = std::max(0.0f, std::min(vs[2].y, std::min(vs[1].y, vs[0].y)));
    int maxY = std::min(height * 1.0f, std::max(vs[2].y, std::max(vs[1].y, vs[0].y)));


    for(int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            vec3f p(x, y, 0);
            vec3f b_coordinate = barycentric(vs[0], vs[1], vs[2], p);
            if(b_coordinate.x < 0 || b_coordinate.y < 0 || b_coordinate.z < 0)
                continue;
            
            vec2f uv(0, 0);
            for(int i = 0; i < 3; i++) {
                p.z += vs[i].z * b_coordinate[i];
                uv = uv + uvs[i] * b_coordinate[i];
            }
            TGAColor color = model->diffuse(uv) * intensity;
            if(z_buffer[x + width * y] < p.z) {
                z_buffer[x + width * y] = p.z;
                if(withMASS) {
                    int cnt = 0;
                    for(int i = 0; i < MSAA_4x4.size(); i++) {
                        float m = x + MSAA_4x4[i].x;
                        float n = y + MSAA_4x4[i].y;
                        bool r = isInnerTriangle(vs[0], vs[1], vs[2], vec3f(m, n, 0));
                        if(r) {
                            cnt++;
                        }
                    }
                    int total = MSAA_4x4.size();
                    float sample = (float)cnt / total;
                    TGAColor c = color * sample;
                    image.set(x, y, c);
                } else {
                    image.set(x, y, color);
                }
            }
        }
    }
}
void loadModel() {
    std::string objFile = "/home/knewhow/dev/TinyRenderer/obj/african_head/african_head.obj";
    Model* m = new Model(objFile);
    long long int face = m->nfaces();
    std::vector<vec3f> screen_coordinate(3);
    std::vector<vec3f> world_coordinate(3);
    std::vector<vec2f> uv(3);
    vec3f light_dir(0, 0, 1);
    for(int i = 0; i < face; i++) {
        for(int j = 0; j < 3; j++) {
            vec3f v = m->vert(i, j);    
            world_coordinate[j] = v;
            screen_coordinate[j] = world2Screen(v);
            uv[j] = m->uv(i, j);
        }
        vec3f AC = world_coordinate[2] - world_coordinate[0];
        vec3f AB = world_coordinate[1] - world_coordinate[0];
        vec3f n = cross(AB, AC);
        n.normalize();
        float intensity = n * light_dir;
        if(intensity > 0) {
            // TGAColor color = TGAColor(intensity * 255, intensity * 255, intensity * 255, 255);
            triangle(screen_coordinate, uv, m, intensity, false);
        }
       
    }
}

void rasterize(vec2i v0, vec2i v1, const TGAColor& color, int y_buffer[]) {
    if(v0.x > v1.x) {
        std::swap(v0, v1);
    }
    for(int x = v0.x; x <= v1.x; x++) {
        float t = (x - v0.x) / (float)(v1.x - v0.x);
        int y = v0.y * (1.0f - t) + v1.y * t;
        if(y_buffer[y] < y) {
            y_buffer[y] = y;
            image.set(x, 0, color);
        }
    }
}

int main(int, char**) {
    int y_buffer[width];
    for(int i = 0; i < width * height; i++) 
        z_buffer[i] = std::numeric_limits<float>::min();
    
    loadModel();

    // image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
}


#include "ourGL.h"
#include <limits>

mat4f ModelView;
mat4f Viewport;
mat4f Projection;

void viewport(const int x, const int y, const int w, const int h) {
    Viewport = {
        {
            {w / 2.0f, 0, 0, x + w / 2.0f}, 
            {0, h / 2.0f, 0, y + h / 2.0f}, 
            {0, 0, 1, 0}, 
            {0, 0, 0, 1}, 
        }
    };
}

void projection(const float coeff) {
    Projection = {
        {
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, coeff, 1}
        }
    };
}

void lookat(const vec3f& eye, const vec3f& center, const vec3f& up) {
    vec3f z = (eye - center).normalize();
    vec3f x = cross(up, z).normalize();
    vec3f y = cross(z, x).normalize();
    mat4f minv = {{{x.x, x.y, x.z, 0},   {y.x, y.y, y.z, 0},   {z.x, z.y, z.z, 0},   {0, 0, 0, 1}}};
    mat4f tr = {{{1,0,0,-center.x}, {0,1,0,-center.y}, {0,0,1,-center.z}, {0,0,0,1}}};
    ModelView = minv * tr;
}

vec3f barycentric(const vec2f* tri, const vec2f p) {
    mat3f ABC = {embed<float, 3>(tri[0]), embed<float, 3>(tri[1]), embed<float, 3>(tri[2])};
    if(ABC.det() < 1e-3) {
        return vec3f(-1, -1, -1);
    } else {
        return ABC.invert_transpose() * embed<float, 3>(p);
    }
}

void triangle(const std::array<vec4f, 3>& clipVerts, Shader& shader, TGAImage& image, std::vector<float>& zBuffer) {
    vec4f pts[3] = {Viewport * clipVerts[0], Viewport * clipVerts[1], Viewport * clipVerts[2]}; // add perspective
    vec2f pts2[3] = {proj<float, 2>(pts[0] / pts[0][3]), proj<float, 2>(pts[1] / pts[1][3]), proj<float, 2>(pts[2] / pts[2][3])}; // divide w
    vec2f bboxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    vec2f bboxMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 2; j++) {
            bboxMin[j] = std::max(0.0f, std::min(bboxMin[j], pts2[i][j]));
            bboxMax[j] = std::min(clamp[j], std::max(bboxMax[j], pts2[i][j]));
        }
    }

    for(int x = (int)bboxMin.x; x <= bboxMax.x; x++) {
        for(int y = (int)bboxMin.y; y <= bboxMax.y; y++) {
            vec3f bcScreen = barycentric(pts2, vec2f(x, y));
            vec3f bcClip = vec3f(bcScreen.x / pts[0][3], bcScreen.y / pts[1][3], bcScreen.z / pts[2][3]);
            bcClip = bcClip / (bcClip.x + bcClip.y + bcClip.z); // barycentric is non-liner, you can refer: https://github.com/ssloy/tinyrenderer/wiki/Technical-difficulties-linear-interpolation-with-perspective-deformations
            float fragDepth = vec3f(clipVerts[0][2], clipVerts[1][2], clipVerts[2][2]) * bcClip;
            int idx = x + y * image.get_width();
            if(bcScreen.x < 0 || bcScreen.y < 0 || bcScreen.z < 0 || fragDepth < zBuffer[idx])
                continue;
            TGAColor color;
            bool discard = shader.fragment(bcClip, color);
            if(discard)
                continue;
            zBuffer[idx] = fragDepth;
            image.set(x, y, color);
        }
    }

}
#include "ourGL.h"

mat4f ModelView;
mat4f Viewport;
mat4f Projection;

void view(const int x, const int y, const int w, const int h) {
    Viewport = {
        {
            {w / 2.0f, 0, 1, x + w / 2.0f}, 
            {0, h / 2.0f, 1, y + h / 2.0f}, 
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
    mat4f minv = mat4f::identity();
    mat4f tr = mat4f::identity();
    for(int i = 0; i < 3; i++) {
        minv[0][i] = x[i];
        minv[1][i] = y[i];
        minv[2][i] = z[i];
        tr[i][3] = -center[i];
    }
    ModelView = minv * tr;
}
#ifndef __TGAIMAGE_H__
#define __TGAIMAGE_H__

#include <fstream>
#include <string>
#include <vector>

#pragma pack(push,1)
struct TGA_Header {
    std::uint8_t  idlength{};
    std::uint8_t  colormaptype{};
    std::uint8_t  datatypecode{};
    std::uint16_t colormaporigin{};
    std::uint16_t colormaplength{};
    std::uint8_t  colormapdepth{};
    std::uint16_t x_origin{};
    std::uint16_t y_origin{};
    std::uint16_t width{};
    std::uint16_t height{};
    std::uint8_t  bitsperpixel{};
    std::uint8_t  imagedescriptor{};
};
#pragma pack(pop)


/**
 * 一个表示每个像素的颜色值的结构体
 * 
*/
struct TGAColor
{
    std::uint8_t bgra[4] = {0, 0, 0, 0};
    std::uint8_t bytespp = 0;
    TGAColor() = default;
    TGAColor(const std::uint8_t R, const std::uint8_t G, const std::uint8_t B, const std::uint8_t A = 255)
        :bgra{B, G, R, A}, bytespp(4){}
    TGAColor(const std::uint8_t v): bgra{v, 0, 0, 0}, bytespp(1){}
    TGAColor(const std::uint8_t* p, const std::uint8_t bpp): bgra{0, 0, 0, 0}, bytespp(bpp) {
        for(int i=0; i < bpp; i++) {
            bgra[i] = *(p + i);
        }
    }
    std::uint8_t& operator[](const int i) { return bgra[i]; }
    std::uint8_t operator[](const int i) const { return bgra[i]; }
    TGAColor operator*(const double intensity) const {
        TGAColor res = *this;
        double clamped = std::max(0., std::min(1., intensity));
        for(int i=0; i < 4; i++) {
            res[i] = res[i] * clamped;
        }
        return res;
    }
    TGAColor operator+(const TGAColor& c) {
        TGAColor ret = *this;
        for(int i=0; i < 4; i++) {
            ret[i] += c[i];
        }
        return ret;
    }
    TGAColor operator+=(const TGAColor&c) {
        for(int i=0; i < 4; i++) {
            (*this)[i] += c[i];
        }
        return *this;
    }
};



/**
 * 一个用于TGA图片构建、读取和写入的类
 *  
*/

class TGAImage {
protected:
    std::vector<std::uint8_t> data;
    int width;
    int height;
    int bytespp;
    bool load_rle_data(std::ifstream& in);
    bool unload_rle_data(std::ofstream& out) const;

public:
    enum Format {
        GRAYSCALE=1, RGB=3, RGBA=4
    };
    TGAImage();
    TGAImage(int width, int height, int bytespp);
    ~TGAImage();
    bool read_tga_file(const std::string filepath);
    bool write_tga_file(const std::string filepath, const bool vflip = true, const bool rle = true) const;
    void flip_horizontally(); // 水平翻转
    void flip_vertically(); // 竖直翻转
    void scale(const int w, const int h); // 缩放
    TGAColor get(const int x, const int y) const; // 获取图片在 x,y坐标处的颜色值
    void set(const int x, const int y, const TGAColor& color); // 设置图片在x,y坐标处的颜色值
    inline int get_width() const { return width; }
    inline int get_height() const { return height; }
    inline int get_bytespp() const { return bytespp; }
    inline std::uint8_t* buffer() { return data.data(); }
    void clear();
};
#endif
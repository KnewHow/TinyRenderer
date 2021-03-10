#include "tgaimage.h"
#include<iostream>
#include<cstring>

TGAImage::TGAImage()
    :width(0), height(0),data(), bytespp(0){}

TGAImage::TGAImage(int width, int height, int bytespp)
    :data(width * height *bytespp, 0), width(width), height(height), bytespp(bytespp) {}


TGAImage::~TGAImage() {
    // TODO
}

bool TGAImage::load_rle_data(std::ifstream& in) {
    size_t pixelcount = width*height;
    size_t currentpixel = 0;
    size_t currentbyte  = 0;
    TGAColor colorbuffer;
    do {
        std::uint8_t chunkheader = 0;
        chunkheader = in.get();
        if (!in.good()) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
        if (chunkheader<128) {
            chunkheader++;
            for (int i=0; i<chunkheader; i++) {
                in.read(reinterpret_cast<char *>(colorbuffer.bgra), bytespp);
                if (!in.good()) {
                    std::cerr << "an error occured while reading the header\n";
                    return false;
                }
                for (int t=0; t<bytespp; t++)
                    data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel>pixelcount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        } else {
            chunkheader -= 127;
            in.read(reinterpret_cast<char *>(colorbuffer.bgra), bytespp);
            if (!in.good()) {
                std::cerr << "an error occured while reading the header\n";
                return false;
            }
            for (int i=0; i<chunkheader; i++) {
                for (int t=0; t<bytespp; t++)
                    data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel>pixelcount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        }
    } while (currentpixel < pixelcount);
    return true;
}

bool TGAImage::unload_rle_data(std::ofstream& out) const{
        const std::uint8_t max_chunk_length = 128;
    size_t npixels = width*height;
    size_t curpix = 0;
    while (curpix<npixels) {
        size_t chunkstart = curpix*bytespp;
        size_t curbyte = curpix*bytespp;
        std::uint8_t run_length = 1;
        bool raw = true;
        while (curpix+run_length<npixels && run_length<max_chunk_length) {
            bool succ_eq = true;
            for (int t=0; succ_eq && t<bytespp; t++)
                succ_eq = (data[curbyte+t]==data[curbyte+t+bytespp]);
            curbyte += bytespp;
            if (1==run_length)
                raw = !succ_eq;
            if (raw && succ_eq) {
                run_length--;
                break;
            }
            if (!raw && !succ_eq)
                break;
            run_length++;
        }
        curpix += run_length;
        out.put(raw?run_length-1:run_length+127);
        if (!out.good()) {
            std::cerr << "can't dump the tga file\n";
            return false;
        }
        out.write(reinterpret_cast<const char *>(data.data()+chunkstart), (raw?run_length*bytespp:bytespp));
        if (!out.good()) {
            std::cerr << "can't dump the tga file\n";
            return false;
        }
    }
    return true;
}

bool TGAImage::read_tga_file(const std::string filepath) {
    std::ifstream in;
    in.open(filepath, std::ios::binary);
    if(!in.is_open()) {
        in.close();
        std::cerr << "can't open file: " << filepath << std::endl;
        return false;
    }
    TGA_Header header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    if(!in.good()) {
        in.close();
        std::cerr << "can't read header from: " << filepath << std::endl;
        return false;
    }
    this->width = header.width;
    this->height = header.height;
    this->bytespp = header.bitsperpixel>>3;
    if(this->width <= 0 || this->height <= 0 ||
        (this->bytespp != Format::GRAYSCALE && this->bytespp != Format::RGB && this->bytespp != Format::RGBA)) {
            in.close();
            std::cerr << "bad bytespp or width/height" << std::endl;
            return false;
    }
    std::size_t nbytes = this->width * this->height * this->bytespp;
    this->data = std::vector<std::uint8_t>(nbytes, 0);
    if(header.datatypecode == 2 || header.datatypecode == 3) {
        in.read(reinterpret_cast<char*>(data.data()), nbytes);
        if(!in.good()) {
            in.close();
            std::cerr << "an error occured when reading data" << std::endl;
            return false;
        }
    } else if(header.datatypecode == 10 || header.datatypecode == 11) {
        if(!load_rle_data(in)) {
            in.close();
            std::cerr << "an error occured when reading data" << std::endl;
            return false;
        }
    } else {
        in.close();
        std::cerr << "unknow file format" << std::endl;
        return false;
    }
    if(!(header.imagedescriptor & 0x20)) {
        flip_vertically();
    }
    if((header.imagedescriptor & 0x10)) {
        flip_horizontally();
    }
    in.close();
    std::cout << "read file successfully, width: " << this->width << "height: " << this->height <<
        "bytespp: " << this->bytespp << std::endl;
    return true;
}

bool TGAImage::write_tga_file(const std::string filepath, const bool vflip, const bool rle) const {
    std::uint8_t developer_area_ref[4] = {0, 0, 0, 0};
    std::uint8_t extension_area_ref[4] = {0, 0, 0, 0};
    std::uint8_t footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};
    std::ofstream out;
    out.open (filepath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "can't open file " << filepath << "\n";
        out.close();
        return false;
    }
    TGA_Header header;
    header.bitsperpixel = bytespp<<3;
    header.width  = width;
    header.height = height;
    header.datatypecode = (bytespp==GRAYSCALE?(rle?11:3):(rle?10:2));
    header.imagedescriptor = vflip ? 0x00 : 0x20; // top-left or bottom-left origin
    out.write(reinterpret_cast<const char *>(&header), sizeof(header));
    if (!out.good()) {
        out.close();
        std::cerr << "can't dump the tga file\n";
        return false;
    }
    if (!rle) {
        out.write(reinterpret_cast<const char *>(data.data()), width*height*bytespp);
        if (!out.good()) {
            std::cerr << "can't unload raw data\n";
            out.close();
            return false;
        }
    } else {
        if (!unload_rle_data(out)) {
            out.close();
            std::cerr << "can't unload rle data\n";
            return false;
        }
    }
    out.write(reinterpret_cast<const char *>(developer_area_ref), sizeof(developer_area_ref));
    if (!out.good()) {
        std::cerr << "can't dump the tga file\n";
        out.close();
        return false;
    }
    out.write(reinterpret_cast<const char *>(extension_area_ref), sizeof(extension_area_ref));
    if (!out.good()) {
        std::cerr << "can't dump the tga file\n";
        out.close();
        return false;
    }
    out.write(reinterpret_cast<const char *>(footer), sizeof(footer));
    if (!out.good()) {
        std::cerr << "can't dump the tga file\n";
        out.close();
        return false;
    }
    out.close();
    return true;
}

void TGAImage::flip_horizontally() {
    if(!data.size()) return;
    int half = width >> 1;
    for(int i=0; i < half; i++) {
        for(int j = 0; j < height; j++) {
            TGAColor c1 = get(i, j);
            TGAColor c2 = get(width - i - 1, j);
            set(i, j, c2);
            set(width - i - 1, j, c1);
        }
    }
}

void TGAImage::flip_vertically() {
    if(!data.size()) return;
    std::size_t byte_per_line = width * bytespp;
    std::vector<std::uint8_t> line(byte_per_line, 0);
    int half = height >> 1;
    for(int j=0; j < half; j++) {
        std::size_t l1 = j * byte_per_line;
        std::size_t l2 = (height - j - 1) * byte_per_line;
        std::copy(data.begin() + l1, data.begin() + l1 + byte_per_line, line.begin());
        std::copy(data.begin() + l2, data.begin() + l2 + byte_per_line, data.begin() + l1);
        std::copy(line.begin(), line.end(), data.begin() + l2);
    }
}

void TGAImage::scale(const int w, const int h) {
    if (w<=0 || h<=0 || !data.size()) return;
    std::vector<std::uint8_t> tdata(w*h*bytespp, 0);
    int nscanline = 0;
    int oscanline = 0;
    int erry = 0;
    size_t nlinebytes = w*bytespp;
    size_t olinebytes = width*bytespp;
    for (int j=0; j<height; j++) {
        int errx = width-w;
        int nx   = -bytespp;
        int ox   = -bytespp;
        for (int i=0; i<width; i++) {
            ox += bytespp;
            errx += w;
            while (errx>=(int)width) {
                errx -= width;
                nx += bytespp;
                memcpy(tdata.data()+nscanline+nx, data.data()+oscanline+ox, bytespp);
            }
        }
        erry += h;
        oscanline += olinebytes;
        while (erry>=(int)height) {
            if (erry>=(int)height<<1) // it means we jump over a scanline
                memcpy(tdata.data()+nscanline+nlinebytes, tdata.data()+nscanline, nlinebytes);
            erry -= height;
            nscanline += nlinebytes;
        }
    }
    data = tdata;
    width = w;
    height = h;
}

TGAColor TGAImage::get(const int x, const int y) const {
    if(data.empty() || x < 0 || y < 0 || x >= width || y >= height) {
        return {};
    } else {
        return TGAColor(data.data() + (x + y * width) * bytespp, bytespp);
    }
}

void TGAImage::set(const int x, const int y, const TGAColor& color) {
    if(data.empty() || x < 0 || y < 0 || x >= width || y >= height) {
        return;
    } else {
        memcpy(data.data() + (x + y * width) * bytespp, color.bgra, bytespp);
    }
}

void TGAImage::clear() {
    data = std::vector<std::uint8_t>(width * height * bytespp, 0);
}


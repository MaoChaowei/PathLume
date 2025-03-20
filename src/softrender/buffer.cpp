#include"buffer.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include"stb_image_write.h" // external head file


bool ColorBuffer::saveToImage(const std::string& filename) const {
    unsigned char* flippedData = new unsigned char[width_ * height_ * 4];

    // 垂直翻转（因为ColorBuffer从左下角开始，而stb_image_write从左上角开始）
    for (int y = 0; y < height_; ++y) {
        memcpy(flippedData + (height_ - 1 - y) * width_ * 4,
               addr_ + y * width_ * 4,
               width_ * 4);
    }

    // 使用stb_image_write保存
    bool success = stbi_write_png(filename.c_str(), width_, height_, 4, flippedData, width_ * 4);

    delete[] flippedData;

    if (!success) {
        std::cerr << "Failed to save image: " << filename << std::endl;
        return false;
    }

    std::cout << "Image saved: " << filename << std::endl;
    return true;
}

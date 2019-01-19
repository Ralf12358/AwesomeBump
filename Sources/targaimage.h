#ifndef TARGAIMAGE_H
#define TARGAIMAGE_H

#include <QImage>
#include <QString>

#define TARGA_HEADER_SIZE    0x12
#define TARGA_UNCOMP_RGB_IMG 0x02
#define TARGA_UNCOMP_BW_IMG  0x03

enum TargaColorFormat
{
    TARGA_BGR=0,
    TARGA_BGRA,
    TARGA_LUMINANCE
};

class TargaImage
{
public:
    // Write QImage to tga file.
    void write(QImage image, QString fileName);
    // Return QImage from readed tga file.
    QImage read(QString fileName);

private:
    /**
     * Load tga image to data.
     * @param filename  Path to the image
     * @param width     (output) Width of the image
     * @param height    (output) Height of the image
     * @param format    (output) Format of the image (RGB, RGBA, LUMINANCE)
     * @param pixels    (output) Pointer to the pixel data
     * @return          True if the image was loaded.
     */
    bool load_targa (const char *filename, int &width, int &height,
                     TargaColorFormat &format, unsigned char *&pixels);

    /**
     * Save tga image from data.
     * @param filename  Path to the image
     * @param width     Width of the image
     * @param height    Height of the image
     * @param format    Format of the image (RGB, RGBA, LUMINANCE)
     * @param pixels    Pointer to the pixel data
     * @return          True if the image was saved.
     */
    bool save_targa (const char *filename, int width, int height,
                     TargaColorFormat format, unsigned char *pixels);
};

#endif // TARGAIMAGE_H

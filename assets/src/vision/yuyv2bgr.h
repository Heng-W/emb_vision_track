#ifndef EVT_YUYV2BGR_H
#define EVT_YUYV2BGR_H

namespace evt
{

// 查表法将YUYV图像转换成BGR图像
void yuyv2bgr(unsigned char* bufferIn, unsigned char* bufferOut, int width, int height);

} // namespace evt

#endif // EVT_YUYV2BGR_H

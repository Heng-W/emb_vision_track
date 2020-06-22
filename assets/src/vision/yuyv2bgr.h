#ifndef EVT_YUYV2RGB_H
#define EVT_YUYV2RGB_H


namespace EVTrack
{

//生成查表法所需的表格
void initColorTables();

//查表法将YUYV图像转换成BGR图像
void yuyv2bgr(int width, int height, unsigned char* bufferIn, unsigned char* bufferOut);


}


#endif

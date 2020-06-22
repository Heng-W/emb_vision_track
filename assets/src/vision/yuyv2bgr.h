#ifndef EVT_YUYV2RGB_H
#define EVT_YUYV2RGB_H


namespace EVTrack
{


void initColorTables();
void yuyv2bgr(int width, int height, unsigned char* bufferIn, unsigned char* bufferOut);


}


#endif
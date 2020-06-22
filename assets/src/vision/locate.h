#ifndef EVT_LOCATE_H
#define EVT_LOCATE_H

#include "video_device.h"

namespace EVTrack
{

namespace Locate
{
extern volatile int xpos;//目标中心x坐标
extern volatile int ypos;//目标中心y坐标
extern volatile int width;//目标宽度
extern volatile int height;//目标高度

//目标与图像中心的x轴偏移量
inline int xposError()
{
    return VideoDevice::IMAGE_W / 2 - xpos;
}

//目标与图像中心的y轴偏移量
inline int yposError()
{
    return VideoDevice::IMAGE_H / 2 - ypos;
}

}


}


#endif  //EVT_LOCATE_H

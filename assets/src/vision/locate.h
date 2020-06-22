#ifndef EVT_LOCATE_H
#define EVT_LOCATE_H

#include "video_device.h"

namespace EVTrack
{

namespace Locate
{
extern volatile int xpos;
extern volatile int ypos;
extern volatile int width;
extern volatile int height;

inline int xposError()
{
    return VideoDevice::IMAGE_W / 2 - xpos;
}

inline int yposError()
{
    return VideoDevice::IMAGE_H / 2 - ypos;
}

}


}


#endif  //EVT_LOCATE_H

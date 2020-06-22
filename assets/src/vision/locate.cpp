
#include "locate.h"

#include "video_device.h"


namespace EVTrack
{

namespace Locate
{

volatile int xpos = VideoDevice::IMAGE_W / 2;
volatile int ypos = VideoDevice::IMAGE_H / 2;
volatile int width;
volatile int height;

}
}

#ifndef EVT_FFT_TOOLS_HPP
#define EVT_FFT_TOOLS_HPP

#include "opencv2/opencv.hpp"

namespace EVTrack
{

namespace kcf
{


namespace FFTTools
{

cv::Mat fftd(cv::Mat img, bool backwards = false);
cv::Mat real(cv::Mat img);
cv::Mat imag(cv::Mat img);
cv::Mat magnitude(cv::Mat img);
cv::Mat complexMultiplication(cv::Mat a, cv::Mat b);
cv::Mat complexDivision(cv::Mat a, cv::Mat b);
void rearrange(cv::Mat& img);
void normalizedLogTransform(cv::Mat& img);


}

}

}

#endif


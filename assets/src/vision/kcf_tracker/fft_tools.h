#ifndef EVT_FFT_TOOLS_H
#define EVT_FFT_TOOLS_H

#include "opencv2/core.hpp"

namespace evt
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

} // namespace FFTTools

} // namespace evt

#endif // EVT_FFT_TOOLS_H


#include "kcf_tracker.h"
#include "fft_tools.h"
#include "rect_tools.hpp"


namespace EVTrack
{

namespace kcf
{

static cv::Mat sub;
static cv::Mat FeaturesMap;
static float peak_value[3];
static cv::Point2f result[3];


inline int findMaxIndex(float a, float b, float c)
{

    int idx = 0;
    float max = a;
    if (b > max)
    {
        idx = 1;
        max = b;
    }
    if (c > max)
    {
        idx = 2;
    }

    return idx;
}

KCFTracker::KCFTracker(bool useMultiScale):
    _useMultiScale(useMultiScale)
{
    initVar();
}

void KCFTracker::initVar()
{

    lambda = 0.0001;//正则化参数
    padding = 2.5;//(第一帧)
    //output_sigma_factor = 0.1;
    output_sigma_factor = 0.125;//(第一帧)


    // RAW
    interp_factor = 0.075;//插值系数(训练)
    sigma = 0.2;//(高斯核函数)


    setTrackMode(_useMultiScale);

}


void KCFTracker::setTrackMode(bool useMultiScale)
{
    _useMultiScale = useMultiScale;

    if (_useMultiScale)
    {
        template_size = 96;
        //template_size = 100;
        scale_step = 1.05;
        scale_weight = 0.95;
    }
    else
    {

        template_size = 96;
        //template_size = 100;
        scale_step = 1;
    }

}


void KCFTracker::reset()
{
    initVar();
}

void KCFTracker::reset(const cv::Rect& roi, cv::Mat& image)
{
    initVar();
    init(roi, image);
}

// Initialize tracker
void KCFTracker::init(const cv::Rect& roi, cv::Mat& image)
{
    _roi = roi;
    assert(roi.width >= 0 && roi.height >= 0);



    _tmpl = getFeatures(image, 1, true);
    _prob = createGaussianPeak(size_patch[0], size_patch[1]);
    _alphaf = cv::Mat(size_patch[0], size_patch[1], CV_32FC2, float(0));
    //_num = cv::Mat(size_patch[0], size_patch[1], CV_32FC2, float(0));
    //_den = cv::Mat(size_patch[0], size_patch[1], CV_32FC2, float(0));
    train(_tmpl, 1.0); // train with initial frame
}
// Update position based on the new frame
cv::Rect KCFTracker::update(cv::Mat& image)
{


    if (_roi.x + _roi.width <= 0) _roi.x = -_roi.width + 1;
    if (_roi.y + _roi.height <= 0) _roi.y = -_roi.height + 1;
    if (_roi.x >= image.cols - 1) _roi.x = image.cols - 2;
    if (_roi.y >= image.rows - 1) _roi.y = image.rows - 2;

    float cx = _roi.x + _roi.width / 2.0f;
    float cy = _roi.y + _roi.height / 2.0f;


    int idx = 0;
    result[0] = detect(_tmpl, getFeatures(image, 1.0f), peak_value[0]);


    if (_useMultiScale)
    {
        // Test at a smaller _scale
        result[1] = detect(_tmpl, getFeatures(image, 1.0f / scale_step), peak_value[1]);

        // Test at a bigger _scale
        result[2] = detect(_tmpl, getFeatures(image, scale_step), peak_value[2]);

        idx = findMaxIndex(peak_value[0], scale_weight * peak_value[1], scale_weight * peak_value[2]);
        switch (idx)
        {
            case 0:
                break;
            case 1:

                _scale /= scale_step;
                _roi.width /= scale_step;
                _roi.height /= scale_step;
                break;
            case 2:
                _scale *= scale_step;
                _roi.width *= scale_step;
                _roi.height *= scale_step;
                break;
            default:
                assert(0);
        }
    }

    // Adjust by cell size and _scale
    _roi.x = cx - _roi.width / 2.0f + ((float) result[idx].x * _scale);
    _roi.y = cy - _roi.height / 2.0f + ((float) result[idx].y * _scale);

    if (_roi.x >= image.cols - 1) _roi.x = image.cols - 1;
    if (_roi.y >= image.rows - 1) _roi.y = image.rows - 1;
    if (_roi.x + _roi.width <= 0) _roi.x = -_roi.width + 2;
    if (_roi.y + _roi.height <= 0) _roi.y = -_roi.height + 2;

    assert(_roi.width >= 0 && _roi.height >= 0);

    train(getFeatures(image), interp_factor);

    return _roi;
}


// Detect object in the current frame.
cv::Point2f KCFTracker::detect(cv::Mat z, cv::Mat x, float& peak_value)
{
    using namespace FFTTools;

    cv::Mat k = gaussianCorrelation(x, z);
    cv::Mat res = (real(fftd(complexMultiplication(_alphaf, fftd(k)), true)));//复数乘法

    //minMaxLoc only accepts doubles for the peak, and integer points for the coordinates
    // 使用opencv的minMaxLoc来定位峰值坐标位置
    cv::Point2i pi;
    double pv;
    cv::minMaxLoc(res, NULL, &pv, NULL, &pi);
    peak_value = (float) pv;

    //subpixel peak estimation, coordinates will be non-integer
    //子像素峰值检测，坐标是非整形的
    cv::Point2f p((float)pi.x, (float)pi.y);

    if (pi.x > 0 && pi.x < res.cols - 1)
    {
        p.x += subPixelPeak(res.at<float>(pi.y, pi.x - 1), peak_value, res.at<float>(pi.y, pi.x + 1));
    }

    if (pi.y > 0 && pi.y < res.rows - 1)
    {
        p.y += subPixelPeak(res.at<float>(pi.y - 1, pi.x), peak_value, res.at<float>(pi.y + 1, pi.x));
    }

    p.x -= (res.cols) / 2;
    p.y -= (res.rows) / 2;

    return p;
}

// train tracker with a single image
void KCFTracker::train(cv::Mat x, float train_interp_factor)
{
    using namespace FFTTools;

    cv::Mat k = gaussianCorrelation(x, x);
    cv::Mat alphaf = complexDivision(_prob, (fftd(k) + lambda));//复数除法

    //线性插值，使参数训练更平滑
    _tmpl = (1 - train_interp_factor) * _tmpl + (train_interp_factor) * x;
    _alphaf = (1 - train_interp_factor) * _alphaf + (train_interp_factor) * alphaf;


    /*cv::Mat kf = fftd(gaussianCorrelation(x, x));
    cv::Mat num = complexMultiplication(kf, _prob);
    cv::Mat den = complexMultiplication(kf, kf + lambda);

    _tmpl = (1 - train_interp_factor) * _tmpl + (train_interp_factor) * x;
    _num = (1 - train_interp_factor) * _num + (train_interp_factor) * num;
    _den = (1 - train_interp_factor) * _den + (train_interp_factor) * den;

    _alphaf = complexDivision(_num, _den);*/

}

// Evaluates a Gaussian kernel with bandwidth SIGMA for all relative shifts between input images X and Y, which must both be MxN. They must
//also be periodic (ie., pre-processed with a cosine window).
cv::Mat KCFTracker::gaussianCorrelation(cv::Mat x1, cv::Mat x2)
{
    using namespace FFTTools;
    cv::Mat c = cv::Mat(cv::Size(size_patch[1], size_patch[0]), CV_32F, cv::Scalar(0));

    // Gray features

    cv::mulSpectrums(fftd(x1), fftd(x2), c, 0, true);//两个傅立叶频谱的每个元素的乘法
    c = fftd(c, true);
    rearrange(c);
    c = real(c);

    cv::Mat d;
    cv::max(((cv::sum(x1.mul(x1))[0] + cv::sum(x2.mul(x2))[0]) - 2. * c) / (size_patch[0]*size_patch[1]*size_patch[2]), 0, d);

    cv::Mat k;
    cv::exp((-d / (sigma * sigma)), k);
    return k;
}

// Create Gaussian Peak. Function called only in the first frame.
cv::Mat KCFTracker::createGaussianPeak(int sizey, int sizex)
{
    cv::Mat_<float> res(sizey, sizex);

    int syh = (sizey) / 2;
    int sxh = (sizex) / 2;

    float output_sigma = std::sqrt((float) sizex * sizey) / padding * output_sigma_factor;
    float mult = -0.5 / (output_sigma * output_sigma);

    for (int i = 0; i < sizey; i++)
        for (int j = 0; j < sizex; j++)
        {
            int ih = i - syh;
            int jh = j - sxh;
            res(i, j) = std::exp(mult * (float)(ih * ih + jh * jh));
        }
    return FFTTools::fftd(res);
}


cv::Mat KCFTracker::getFeatures(const cv::Mat& image,  float scale_adjust, bool inithann)
{
    cv::Rect extracted_roi;

    float cx = _roi.x + _roi.width / 2;
    float cy = _roi.y + _roi.height / 2;

    if (inithann)
    {
        int padded_w = _roi.width * padding;
        int padded_h = _roi.height * padding;

        if (template_size > 1)    // Fit largest dimension to the given template size
        {
            if (padded_w >= padded_h)  //fit to width
                _scale = padded_w / (float) template_size;
            else
                _scale = padded_h / (float) template_size;

            _tmpl_sz.width = padded_w / _scale;
            _tmpl_sz.height = padded_h / _scale;
        }


        //Make number of pixels even (helps with some logic involving half-dimensions)
        _tmpl_sz.width = (_tmpl_sz.width / 2) * 2;
        _tmpl_sz.height = (_tmpl_sz.height / 2) * 2;

    }

    extracted_roi.width = scale_adjust * _scale * _tmpl_sz.width;
    extracted_roi.height = scale_adjust * _scale * _tmpl_sz.height;

    // center roi with new size
    extracted_roi.x = cx - extracted_roi.width / 2;
    extracted_roi.y = cy - extracted_roi.height / 2;

    sub = RectTools::subwindow(image, extracted_roi, cv::BORDER_REPLICATE);

    if (sub.cols != _tmpl_sz.width || sub.rows != _tmpl_sz.height)
    {
        cv::resize(sub, sub, _tmpl_sz);
    }

    //chg1
    //FeaturesMap = RectTools::getGrayImage(sub);

    //   cv::cvtColor(sub, FeaturesMap, CV_BGR2GRAY);
    FeaturesMap = sub;

    //  FeaturesMap.convertTo(FeaturesMap, CV_32F, 1 / 255.f);



    //    FeaturesMap -= (float) 0.5; // In Paper;
    size_patch[0] = sub.rows;
    size_patch[1] = sub.cols;
    size_patch[2] = 1;


    if (inithann)
    {
        createHanningMats();
    }
    FeaturesMap = hann.mul(FeaturesMap);
    return FeaturesMap;
}

// Initialize Hanning window. Function called only in the first frame.
void KCFTracker::createHanningMats()
{
    cv::Mat hann1t = cv::Mat(cv::Size(size_patch[1], 1), CV_32F, cv::Scalar(0));
    cv::Mat hann2t = cv::Mat(cv::Size(1, size_patch[0]), CV_32F, cv::Scalar(0));

    for (int i = 0; i < hann1t.cols; i++)
        hann1t.at<float > (0, i) = 0.5 * (1 - std::cos(2 * 3.1415926535 * i / (hann1t.cols - 1)));
    for (int i = 0; i < hann2t.rows; i++)
        hann2t.at<float > (i, 0) = 0.5 * (1 - std::cos(2 * 3.1415926535 * i / (hann2t.rows - 1)));

    hann = hann2t * hann1t;// Gray features

}

// Calculate sub-pixel peak for one dimension
float KCFTracker::subPixelPeak(float left, float center, float right)
{
    float divisor = 2 * center - right - left;

    if (divisor == 0)
        return 0;

    return 0.5 * (right - left) / divisor;
}

}

}

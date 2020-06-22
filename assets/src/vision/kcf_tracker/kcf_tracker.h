#ifndef EVT_KCF_TRACKER_H
#define EVT_KCF_TRACKER_H


#include "opencv2/opencv.hpp"
#include <string>


namespace EVTrack
{

namespace kcf
{


class KCFTracker
{
public:

    KCFTracker(bool useMultiScale = false);



    void setTrackMode(bool useMultiScale);

    void reset();
    void reset(const cv::Rect& roi, cv::Mat& image);
    // 初始化追踪器
    void init(const cv::Rect& roi, cv::Mat& image);

    // 基于新输入的帧更新位置
    cv::Rect update(cv::Mat& image);

    float interp_factor; // 自适应线性插值系数
    float sigma; // 高斯核带宽
    float lambda; // 正则化参数

    float padding; // 目标扩展区域
    float output_sigma_factor; // 高斯目标带宽
    int template_size; // 模板大小
    float scale_step; // 多尺度估计的尺度调整步长
    float scale_weight;  // 调整追踪得分的权值以增加稳定性

protected:

    void initVar();

    //在当前帧追踪目标
    cv::Point2f detect(cv::Mat z, cv::Mat x, float& peak_value);

    // 用当前图像训练追踪器
    void train(cv::Mat x, float train_interp_factor);

    //使用带宽SIGMA计算输入图像X和Y之间的所有相对位移的高斯核。
    //它们必须是周期性的（即用余弦窗口进行预处理）。
    cv::Mat gaussianCorrelation(cv::Mat x1, cv::Mat x2);

    // 在第一帧创建高斯峰值
    cv::Mat createGaussianPeak(int sizey, int sizex);

    //从图像中获取子窗口，使用赋值填充和提取特征
    cv::Mat getFeatures(const cv::Mat& image, float scale_adjust = 1.0f, bool inithann = false);

    // 在第一帧初始化汉宁窗，用余弦窗口进行预处理，变成周期信号
    void createHanningMats();

    // 计算一维亚像素峰值
    float subPixelPeak(float left, float center, float right);

    cv::Mat _alphaf;//训练结果
    cv::Mat _prob;//高斯矩阵结果，用于训练
    cv::Mat _tmpl;//保存上一帧模板

private:
    bool _useMultiScale;
    int size_patch[3];//子图像大小
    cv::Mat hann;
    cv::Size _tmpl_sz;
    float _scale;
    int _gaussian_size;

    cv::Rect_<float> _roi;
};

}

}

#endif

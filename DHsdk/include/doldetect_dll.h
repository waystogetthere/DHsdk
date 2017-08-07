#ifndef DOLDETECT_DLL_H
#define DOLDETECT_DLL_H

#ifdef DOLDETECT_DLL_EXPORTS
#ifdef __linux__
#define DOLDETECT_API
#else
#define DOLDETECT_API __declspec(dllexport)
#endif
#else
#ifdef __linux__
#define DOLDETECT_API
#else
#define DOLDETECT_API __declspec(dllimport)
#endif
#endif

#include <vector>
#include "opencv2/opencv.hpp"

DOLDETECT_API void doldetect(cv::Mat& img, std::vector<cv::Rect>& bboxes,
	int minHessian = 400, float minDist = 0.5, float eps = 0.5, int minPts = 3, int maxPts = 6, bool use_svm = true);

DOLDETECT_API void genTemplates();

#endif

#include "common.hpp"

#include <chrono>
#include <cstdio>
#include <numeric>
#include <opencv2/imgproc.hpp>

int main(int argc, char** argv) {
    const char* path = inputPath(argc, argv);
    const std::vector<unsigned char> nv12Storage = readNv12File(path);

    cv::Mat nv12(kSrcHeight * 3 / 2, kSrcWidth, CV_8UC1,
                 const_cast<unsigned char*>(nv12Storage.data()));
    cv::Mat rgbFull;
    cv::Mat rgb640(kDstHeight, kDstWidth, CV_8UC3);
    std::vector<double> costsMs;
    costsMs.reserve(kIterations);

    for (int i = 0; i < kIterations; ++i) {
        const auto begin = std::chrono::steady_clock::now();
        cv::cvtColor(nv12, rgbFull, cv::COLOR_YUV2RGB_NV12);
        cv::resize(rgbFull, rgb640, cv::Size(kDstWidth, kDstHeight), 0.0, 0.0, cv::INTER_LINEAR);
        const auto end = std::chrono::steady_clock::now();
        costsMs.push_back(std::chrono::duration<double, std::milli>(end - begin).count());
    }

    const double total = std::accumulate(costsMs.begin(), costsMs.end(), 0.0);
    const double avg = total / static_cast<double>(costsMs.size());
    LOGI("OpenCV NV12 %dx%d -> RGB %dx%d", kSrcWidth, kSrcHeight, kDstWidth, kDstHeight);
    LOGI("Iterations: %d", kIterations);
    LOGI("Average conversion time: %.3f ms", avg);
    LOGI("Output RGB bytes: %zu", rgb640.total() * rgb640.elemSize());
    return 0;
}

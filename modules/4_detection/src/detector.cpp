#include "detector.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include <inference_engine.hpp>

using namespace cv;
using namespace InferenceEngine;

Detector::Detector() {
    Core ie;

    // Load deep learning network into memory
    auto net = ie.ReadNetwork(utils::fs::join(DATA_FOLDER, "face-detection-0104.xml"),
                              utils::fs::join(DATA_FOLDER, "face-detection-0104.bin"));

    InputInfo::Ptr inputInfo = net.getInputsInfo()["data"];
    inputInfo->getPreProcess().setResizeAlgorithm(ResizeAlgorithm::RESIZE_BILINEAR);
    inputInfo->setLayout(Layout::NHWC);
    inputInfo->setPrecision(Precision::U8);
    outputName = net.getOutputsInfo().begin()->first;

    // Initialize runnable object on CPU device
    ExecutableNetwork execNet = ie.LoadNetwork(net, "CPU");

    // Create a single processing thread
    req = execNet.CreateInferRequest();
}


Blob::Ptr wrapMatToBlob(const Mat& m) {
    CV_Assert(m.depth() == CV_8U);
    std::vector<size_t> dims = { 1, (size_t)m.channels(), (size_t)m.rows, (size_t)m.cols };
    return make_shared_blob<uint8_t>(TensorDesc(Precision::U8, dims, Layout::NHWC),
        m.data);
}


void Detector::detect(const cv::Mat& image,
                      float nmsThreshold,
                      float probThreshold,
                      std::vector<cv::Rect>& boxes,
                      std::vector<float>& probabilities,
                      std::vector<unsigned>& classes) {
    // Create 4D blob from BGR image
    Blob::Ptr input = wrapMatToBlob(image);

    req.SetBlob("image", input);

    // Launch network
    req.Infer();

    float* output = req.GetBlob(outputName)->buffer();

    float probability, imageIndex = 0;
    unsigned classIndex = 0;

    int height = image.rows;
    int width = image.cols;

    int detectedRects = req.GetBlob(outputName)->size() / 7;

    for (int i = 0; i < detectedRects; i++)
    {
        int tempRectIndex = i * 7;

        imageIndex = output[tempRectIndex];
        classIndex = output[tempRectIndex + 1];
        probability = output[tempRectIndex + 2];

        if (probability > probThreshold)
        {
            classes.push_back(classIndex);
            probabilities.push_back(probability);

            int xmin, ymin, xmax, ymax;

            xmin = output[tempRectIndex + 3] * width;
            ymin = output[tempRectIndex + 4] * height;
            xmax = output[tempRectIndex + 5] * width;
            ymax = output[tempRectIndex + 6] * height;

            Rect approvedRect(xmin, ymin, int(xmax - xmin) + 1, int(ymax - ymin) + 1);
            boxes.push_back(approvedRect);
        }
    }

    nms(boxes, probabilities, nmsThreshold, classes);
}


void nms(const std::vector<cv::Rect>& boxes, const std::vector<float>& probabilities,
         float threshold, std::vector<unsigned>& indices) {
    std::vector<Rect> boxesCopy = boxes;
    std::vector<float> probsCopy = probabilities;
    std::vector<Rect> approvedBoxes;

    while (boxesCopy.size() > 0)
    {
        float tempprob = *std::max_element(probsCopy.begin(), probsCopy.end());
        auto iteratorFinding = std::find(probsCopy.begin(), probsCopy.end(), tempprob);
        int indexFinding = iteratorFinding - probsCopy.begin();

        Rect currentRect = boxesCopy[indexFinding];

        boxesCopy.erase(boxesCopy.begin() + indexFinding);
        probsCopy.erase(probsCopy.begin() + indexFinding);

        approvedBoxes.push_back(currentRect);

        for (int i = 0; i < boxesCopy.size(); i++)
        {
            float iou_metric = iou(currentRect, boxesCopy[i]);
            if (iou_metric > threshold)
            {
                boxesCopy.erase(boxesCopy.begin() + i);
                probsCopy.erase(probsCopy.begin() + i);
            }
        }
    }

    for (int i = 0; i < approvedBoxes.size(); i++)
    {
        Rect tempRect = approvedBoxes[i];
        auto iteratorFinding = std::find(boxes.begin(), boxes.end(), tempRect);
        int indexFinding = std::distance(boxes.begin(), iteratorFinding);
        indices.push_back(indexFinding);
    }
}

float iou(const cv::Rect& a, const cv::Rect& b) {
    float intersectionRects = (a & b).area();
    float unionRects = a.area() + b.area();
    return intersectionRects / (unionRects - intersectionRects);
}

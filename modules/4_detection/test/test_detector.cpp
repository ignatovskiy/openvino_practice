#include <fstream>

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>

#include "detector.hpp"

using namespace cv;
using namespace cv::utils::fs;

// Simple NMS test
TEST(detection, nms) {
    std::vector<Rect> boxes = {Rect(24, 17, 75, 129), Rect(38, 38, 88, 134), Rect(30, 88, 150, 105)};
    std::vector<float> probs = {0.81f, 0.89f, 0.92f};
    std::vector<unsigned> indices;

    // Replace #if 0 to #if 1 for debug visualization.
#if 0
    Mat img(300, 300, CV_8UC1, Scalar(0));
    for (int i = 0; i < boxes.size(); ++i) {
        rectangle(img, boxes[i], 255);
        putText(img, format("%.2f", probs[i]), Point(boxes[i].x, boxes[i].y - 2),
                FONT_HERSHEY_SIMPLEX, 0.5, 255);
    }
    imshow("nms", img);
    waitKey();
#endif

    nms(boxes, probs, 0.4, indices);

    ASSERT_EQ(indices.size(), 2);
    ASSERT_EQ(indices[0], 2);
    ASSERT_EQ(indices[1], 1);
}

TEST(detection, iou) {
    Rect a(21, 27, 110, 150);
    Rect b(53, 56, 108, 152);
    ASSERT_NEAR(iou(a, b), 0.401993, 1e-5);
}

TEST(detection, faces) {
    const float nmsThreshold = 0.45f;
    const float probThreshold = 0.3f;

    Mat img = imread(join(DATA_FOLDER, "conference.png"));
    std::vector<Rect> refBoxes = {
        Rect(276, 267, 24, 31), Rect(529, 244, 24, 31), Rect(127, 268, 22, 29),
        Rect(53, 263, 23, 29), Rect(600, 248, 23, 30), Rect(453, 242, 22, 31),
        Rect(428, 196, 22, 30), Rect(199, 264, 22, 29), Rect(570, 267, 22, 29),
        Rect(605, 192, 21, 30), Rect(94, 250, 22, 28), Rect(486, 197, 20, 27),
        Rect(432, 263, 23, 30), Rect(370, 196, 20, 28), Rect(534, 194, 20, 26),
        Rect(328, 236, 20, 29), Rect(627, 276, 22, 28), Rect(108, 173, 21, 31),
        Rect(368, 261, 24, 31), Rect(571, 173, 20, 27), Rect(166, 241, 22, 30),
        Rect(252, 202, 21, 29), Rect(243, 240, 23, 30), Rect(387, 238, 21, 30),
        Rect(195, 205, 21, 30), Rect(162, 193, 17, 25), Rect(492, 262, 22, 29),
        Rect(319, 210, 19, 27), Rect(31, 238, 23, 30)
    };
    std::vector<float> refProbs = {
        0.988363, 0.987432, 0.983568, 0.981547, 0.980774, 0.980398,
        0.97681,  0.972978, 0.964593, 0.960844, 0.958782, 0.942152,
        0.913904, 0.903534, 0.888043, 0.879184, 0.875234, 0.862958,
        0.815154, 0.810955, 0.797403, 0.792146, 0.771417, 0.710467,
        0.696174, 0.619217, 0.577113, 0.418896, 0.306871
    };

    Detector model;
    std::vector<Rect> boxes;
    std::vector<float> probs;
    std::vector<unsigned> classes;
    model.detect(img, nmsThreshold, probThreshold, boxes, probs, classes);

    // Replace #if 0 to #if 1 for debug visualization.
#if 0
    for (int i = 0; i < boxes.size(); ++i) {
        rectangle(img, boxes[i], Scalar(0, 0, 255));
        rectangle(img, refBoxes[i], Scalar(0, 255, 0));
        putText(img, format("%.2f", probs[i]), Point(boxes[i].x, boxes[i].y - 2),
                FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0));
    }
    imshow("Detection", img);
    waitKey();
#endif

    ASSERT_GE(boxes.size(), refBoxes.size());
    ASSERT_GE(probs.size(), refProbs.size());
    ASSERT_GE(classes.size(), refBoxes.size());

    int i;
    for (i = 0; i < boxes.size(); ++i) {
        if (probs[i] < probThreshold)
            break;

        ASSERT_EQ(classes[i], 1);
        ASSERT_NEAR(probs[i], refProbs[i], 1e-5f);
        ASSERT_EQ(boxes[i], refBoxes[i]);
    }
    ASSERT_EQ(i, boxes.size());
}

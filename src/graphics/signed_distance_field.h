#pragma once
#include <Eigen/Dense>
#include <QImage>
#include <algorithm>
#include <cmath>

namespace impl {
constexpr float DISTANCE_INF_SENTINEL{1e20f};
}

// Helper function to convert QImage to Eigen::MatrixXf
Eigen::MatrixXf qImageToEigenMatrix(const QImage& image, bool invert = false) {
    int width = image.width();
    int height = image.height();
    Eigen::MatrixXf matrix(height, width);

    for (int y = 0; y < height; ++y) {
        const uchar* scanLine = image.constScanLine(y);
        for (int x = 0; x < width; ++x) {
            bool isSet = scanLine[x] < 128; // Threshold at 128
            if (invert) isSet = !isSet;
            matrix(y, x) = isSet ? 0 : impl::DISTANCE_INF_SENTINEL;
        }
    }

    return matrix;
}

// Helper function to convert Eigen::MatrixXf to QImage
QImage eigenMatrixToQImage(const Eigen::MatrixXf& matrix) {
    int height = matrix.rows();
    int width = matrix.cols();
    QImage result(width, height, QImage::Format_Grayscale8);
    result.fill(Qt::white);

    float minVal = matrix.minCoeff();
    float maxVal = matrix.maxCoeff();
    float range = std::max(std::abs(minVal), std::abs(maxVal)) * 2;

    for (int y = 0; y < height; ++y) {
        uchar* scanLine = result.scanLine(y);
        for (int x = 0; x < width; ++x) {
            float normalizedVal = (matrix(y, x) / range) + 0.5f;
            normalizedVal = std::max(0.0f, std::min(1.0f, normalizedVal));
            scanLine[x] = static_cast<uchar>(std::round(normalizedVal * 255));
        }
    }

    return result;
}

// 1D distance transform (helper function)
void distance_transform_1d(Eigen::VectorXf& f, Eigen::VectorXf& d, Eigen::VectorXi& v, Eigen::VectorXf& z) {
    int n = f.size();
    int k = 0;
    v(0) = 0;
    z(0) = -impl::DISTANCE_INF_SENTINEL;
    z(1) = impl::DISTANCE_INF_SENTINEL;

    auto square = [](auto x) { return x * x; };

    for (int index = 1; index < n; ++index) {
        float s = ((f(index) + square(index)) - (f(v(k)) + square(v(k)))) / (2 * index - 2 * v(k));
        while (s <= z(k)) {
            k--;
            s = ((f(index) + square(index)) - (f(v(k)) + square(v(k)))) / (2 * index - 2 * v(k));
        }
        k++;
        v(k) = index;
        z(k) = s;
        z(k + 1) = impl::DISTANCE_INF_SENTINEL;
    }

    k = 0;
    for (int q = 0; q < n; ++q) {
        while (z(k + 1) < q)
            k++;
        d(q) = square(q - v(k)) + f(v(k));
    }
}

// 2D Signed Distance Transform
QImage signed_distance_transform_2d(const QImage& image) {
    int width = image.width();
    int height = image.height();
    int n = std::max(width, height);

    Eigen::MatrixXf dtdata1 = qImageToEigenMatrix(image, false);
    Eigen::MatrixXf dtdata2 = qImageToEigenMatrix(image, true);

    Eigen::VectorXf f(n), d(n);
    Eigen::VectorXi v(n);
    Eigen::VectorXf z(n + 1);

    // Transform along columns
    for (int x = 0; x < width; ++x) {
        f = dtdata1.col(x);
        distance_transform_1d(f, d, v, z);
        dtdata1.col(x) = d.head(height);

        f = dtdata2.col(x);
        distance_transform_1d(f, d, v, z);
        dtdata2.col(x) = d.head(height);
    }

    // Transform along rows
    for (int y = 0; y < height; ++y) {
        f = dtdata1.row(y).transpose();
        distance_transform_1d(f, d, v, z);
        dtdata1.row(y) = d.head(width).transpose();

        f = dtdata2.row(y).transpose();
        distance_transform_1d(f, d, v, z);
        dtdata2.row(y) = d.head(width).transpose();
    }

    dtdata1 = dtdata1.array().sqrt();
    dtdata2 = dtdata2.array().sqrt();

    Eigen::MatrixXf result = dtdata1 - dtdata2;

    return eigenMatrixToQImage(result);
}

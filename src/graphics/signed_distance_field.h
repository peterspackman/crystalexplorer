#pragma once
#include <QImage>
#include <algorithm>
#include <cmath>
#include <vector>

#define DISTANCE_INF_SENTINEL 1e20f

inline void distance_transform_1d(const std::vector<float> &f,
                                  std::vector<float> &d, std::vector<int> &v,
                                  std::vector<float> &z, int n) {
  int k = 0;
  v[0] = 0;
  z[0] = -DISTANCE_INF_SENTINEL;
  z[1] = DISTANCE_INF_SENTINEL;
  auto square = [](auto x) { return x * x; };

  for (int index = 1; index <= n - 1; index++) {
    float s = ((f[index] + square(index)) - (f[v[k]] + square(v[k]))) /
              (2 * index - 2 * v[k]);
    while (s <= z[k]) {
      k--;
      s = ((f[index] + square(index)) - (f[v[k]] + square(v[k]))) /
          (2 * index - 2 * v[k]);
    }
    k++;
    v[k] = index;
    z[k] = s;
    z[k + 1] = DISTANCE_INF_SENTINEL;
  }

  k = 0;
  for (int q = 0; q <= n - 1; q++) {
    while (z[k + 1] < q)
      k++;
    d[q] = square(q - v[k]) + f[v[k]];
  }
}

inline std::vector<float>
grayscale_to_float(const QImage &im,
                   uchar on = std::numeric_limits<uchar>::max()) {
  int width = im.width();
  int height = im.height();

  std::vector<float> out(width * height);
  for (int y = 0; y < height; y++) {
    const uchar *bits = im.scanLine(y);
    for (int x = 0; x < width; x++) {
      if (bits[x] >= on)
        out[y * width + x] = 0;
      else
        out[y * width + x] = DISTANCE_INF_SENTINEL;
    }
  }
  return out;
}

template <typename T>
inline T bound(const T &x, const T &lower, const T &upper) {
  return (x < lower ? lower : (x > upper ? upper : x));
}

QImage to_8bit_grayscale(const std::vector<float> &data, int width, int height,
                         float l, float u) {
  QImage result = QImage(QSize(width, height), QImage::Format_Grayscale8);
  float scale = std::numeric_limits<uchar>::max() / (u - l);
  for (int y = 0; y < height; y++) {
    uchar *bits = result.scanLine(y);
    for (int x = 0; x < width; x++) {
      uchar val = static_cast<uchar>((data[y * width + x] - l) * scale);
      bits[x] = bound(val, uchar{0}, std::numeric_limits<uchar>::max());
    }
  }
  return result;
}

/* dt of 2d function using squared distance */
inline QImage
distance_transform_2d(const QImage &image,
                      uchar on = std::numeric_limits<uchar>::max()) {
  int width = image.width();
  int height = image.height();
  const int n = std::max(width, height);
  std::vector<float> f(n);
  std::vector<float> d(n);
  std::vector<int> v(n);
  std::vector<float> z(n + 1);
  std::vector<float> dtdata = grayscale_to_float(image, on);

  // transform along columns
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      f[y] = dtdata[y * width + x];
    }
    distance_transform_1d(f, d, v, z, height);
    for (int y = 0; y < height; y++) {
      dtdata[y * width + x] = d[y];
    }
  }

  // transform along rows
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      f[x] = dtdata[y * width + x];
    }
    distance_transform_1d(f, d, v, z, width);
    for (int x = 0; x < width; x++) {
      dtdata[y * width + x] = d[x];
    }
  }
  std::transform(dtdata.begin(), dtdata.end(), dtdata.begin(),
                 [](double x) { return std::sqrt(x); });
  auto minmax = std::minmax_element(dtdata.begin(), dtdata.end());
  float l = *minmax.first;
  float u = *minmax.second;
  return to_8bit_grayscale(dtdata, width, height, l, u);
}

/* dt of 2d function using squared distance */
inline QImage signed_distance_transform_2d(const QImage &image) {
  int width = image.width();
  int height = image.height();
  const int n = std::max(width, height);
  std::vector<float> f(n);
  std::vector<float> d(n);
  std::vector<int> v(n);
  std::vector<float> z(n + 1);
  std::vector<float> dtdata1 = grayscale_to_float(image);
  std::vector<float> dtdata2 = grayscale_to_float(image, 0);

  // transform along columns
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      f[y] = dtdata1[y * width + x];
    }
    distance_transform_1d(f, d, v, z, height);
    for (int y = 0; y < height; y++) {
      dtdata1[y * width + x] = d[y];
    }
  }

  // transform along rows
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      f[x] = dtdata1[y * width + x];
    }
    distance_transform_1d(f, d, v, z, width);
    for (int x = 0; x < width; x++) {
      dtdata1[y * width + x] = d[x];
    }
  }
  // transform along columns
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      f[y] = dtdata2[y * width + x];
    }
    distance_transform_1d(f, d, v, z, height);
    for (int y = 0; y < height; y++) {
      dtdata2[y * width + x] = d[y];
    }
  }

  // transform along rows
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      f[x] = dtdata2[y * width + x];
    }
    distance_transform_1d(f, d, v, z, width);
    for (int x = 0; x < width; x++) {
      dtdata2[y * width + x] = d[x];
    }
  }
  std::transform(dtdata1.begin(), dtdata1.end(), dtdata1.begin(),
                 [](double x) { return std::sqrt(x); });
  std::transform(dtdata2.begin(), dtdata2.end(), dtdata2.begin(),
                 [](double x) { return std::sqrt(x); });
  for (int i = 0; i < dtdata1.size(); i++) {
    dtdata1[i] = dtdata1[i] - dtdata2[i];
  }
  auto minmax = std::minmax_element(dtdata1.begin(), dtdata1.end());
  float l = *minmax.first;
  float u = *minmax.second;
  return to_8bit_grayscale(dtdata1, width, height, l, u);
}

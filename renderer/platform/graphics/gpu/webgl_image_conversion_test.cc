// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/gpu/webgl_image_conversion.h"

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class WebGLImageConversionTest : public testing::Test {
 protected:
  void UnpackPixels(const uint16_t* source_data,
                    WebGLImageConversion::DataFormat source_data_format,
                    unsigned pixels_per_row,
                    uint8_t* destination_data) {
    WebGLImageConversion::UnpackPixels(source_data, source_data_format,
                                       pixels_per_row, destination_data);
  }
  void PackPixels(const uint8_t* source_data,
                  WebGLImageConversion::DataFormat source_data_format,
                  unsigned pixels_per_row,
                  uint8_t* destination_data) {
    WebGLImageConversion::PackPixels(source_data, source_data_format,
                                     pixels_per_row, destination_data);
  }
};

TEST_F(WebGLImageConversionTest, ConvertRGBA4444toRGBA8) {
  uint16_t source_data[9] = {0x1234, 0x3456, 0x1234, 0x3456, 0x1234,
                             0x3456, 0x1234, 0x3456, 0x1234};
  uint8_t expected_data[36] = {
      0x11, 0x22, 0x33, 0x44, 0x33, 0x44, 0x55, 0x66, 0x11, 0x22, 0x33, 0x44,
      0x33, 0x44, 0x55, 0x66, 0x11, 0x22, 0x33, 0x44, 0x33, 0x44, 0x55, 0x66,
      0x11, 0x22, 0x33, 0x44, 0x33, 0x44, 0x55, 0x66, 0x11, 0x22, 0x33, 0x44};
  uint8_t destination_data[36];
  UnpackPixels(source_data, WebGLImageConversion::kDataFormatRGBA4444, 9,
               destination_data);
  EXPECT_EQ(0,
            memcmp(expected_data, destination_data, sizeof(destination_data)));
}

TEST_F(WebGLImageConversionTest, ConvertRGBA5551toRGBA8) {
  uint16_t source_data[9] = {0x1234, 0x3456, 0x1234, 0x3456, 0x1234,
                             0x3456, 0x1234, 0x3456, 0x1234};
  uint8_t expected_data[36] = {
      0x12, 0x40, 0xd2, 0x0, 0x36, 0x89, 0x5b, 0x0, 0x12, 0x40, 0xd2, 0x0,
      0x36, 0x89, 0x5b, 0x0, 0x12, 0x40, 0xd2, 0x0, 0x36, 0x89, 0x5b, 0x0,
      0x12, 0x40, 0xd2, 0x0, 0x36, 0x89, 0x5b, 0x0, 0x12, 0x40, 0xd2, 0x0};
  uint8_t destination_data[36];
  UnpackPixels(source_data, WebGLImageConversion::kDataFormatRGBA5551, 9,
               destination_data);
  EXPECT_EQ(0,
            memcmp(expected_data, destination_data, sizeof(destination_data)));
}

TEST_F(WebGLImageConversionTest, ConvertRGBA8toRA8) {
  uint8_t source_data[40] = {0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56};
  uint8_t expected_data[20] = {0x9a, 0x56, 0x9a, 0x56, 0x9a, 0x56, 0x9a,
                               0x56, 0x9a, 0x56, 0x9a, 0x56, 0x9a, 0x56,
                               0x9a, 0x56, 0x9a, 0x56, 0x9a, 0x56};
  uint8_t destination_data[20];
  PackPixels(source_data, WebGLImageConversion::kDataFormatRA8, 10,
             destination_data);
  EXPECT_EQ(0,
            memcmp(expected_data, destination_data, sizeof(destination_data)));
}

TEST_F(WebGLImageConversionTest, convertBGRA8toRGBA8) {
  uint32_t source_data[9] = {0x12345678, 0x34567888, 0x12345678,
                             0x34567888, 0x12345678, 0x34567888,
                             0x12345678, 0x34567888, 0x12345678};
#if defined(ARCH_CPU_BIG_ENDIAN)
  uint32_t expectedData[9] = {0x56341278, 0x78563488, 0x56341278,
                              0x78563488, 0x56341278, 0x78563488,
                              0x56341278, 0x78563488, 0x56341278};
#else
  uint32_t expected_data[9] = {0x12785634, 0x34887856, 0x12785634,
                               0x34887856, 0x12785634, 0x34887856,
                               0x12785634, 0x34887856, 0x12785634};
#endif
  uint32_t destination_data[9];
  UnpackPixels(reinterpret_cast<uint16_t*>(&source_data[0]),
               WebGLImageConversion::kDataFormatBGRA8, 9,
               reinterpret_cast<uint8_t*>(&destination_data[0]));
  EXPECT_EQ(0,
            memcmp(expected_data, destination_data, sizeof(destination_data)));
}

TEST_F(WebGLImageConversionTest, ConvertRGBA8toR8) {
  uint8_t source_data[40] = {0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56};
  uint8_t expected_data[10] = {0x9a, 0x9a, 0x9a, 0x9a, 0x9a,
                               0x9a, 0x9a, 0x9a, 0x9a, 0x9a};
  uint8_t destination_data[10];
  PackPixels(source_data, WebGLImageConversion::kDataFormatR8, 10,
             destination_data);
  EXPECT_EQ(0,
            memcmp(expected_data, destination_data, sizeof(destination_data)));
}

TEST_F(WebGLImageConversionTest, ConvertRGBA8toRGBA8) {
  uint8_t source_data[40] = {0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56};
  uint8_t expected_data[40] = {0x9a, 0xff, 0x9a, 0x56, 0x9a, 0xff, 0x9a, 0x56,
                               0x9a, 0xff, 0x9a, 0x56, 0x9a, 0xff, 0x9a, 0x56,
                               0x9a, 0xff, 0x9a, 0x56, 0x9a, 0xff, 0x9a, 0x56,
                               0x9a, 0xff, 0x9a, 0x56, 0x9a, 0xff, 0x9a, 0x56,
                               0x9a, 0xff, 0x9a, 0x56, 0x9a, 0xff, 0x9a, 0x56};
  uint8_t destination_data[40];
  PackPixels(source_data, WebGLImageConversion::kDataFormatRGBA8, 10,
             destination_data);
  EXPECT_EQ(0,
            memcmp(expected_data, destination_data, sizeof(destination_data)));
}

TEST_F(WebGLImageConversionTest, ConvertRGBA8ToUnsignedShort4444) {
  uint8_t source_data[40] = {0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56};
  uint16_t expected_data[10] = {0x3535, 0x3535, 0x3535, 0x3535, 0x3535,
                                0x3535, 0x3535, 0x3535, 0x3535, 0x3535};
  uint16_t destination_data[10];
  PackPixels(source_data, WebGLImageConversion::kDataFormatRGBA4444, 10,
             reinterpret_cast<uint8_t*>(&destination_data[0]));
  EXPECT_EQ(0,
            memcmp(expected_data, destination_data, sizeof(destination_data)));
}

TEST_F(WebGLImageConversionTest, ConvertRGBA8ToRGBA5551) {
  uint8_t source_data[40] = {0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56};
  uint16_t expected_data[10] = {0x328c, 0x328c, 0x328c, 0x328c, 0x328c,
                                0x328c, 0x328c, 0x328c, 0x328c, 0x328c};
  uint16_t destination_data[10];
  PackPixels(source_data, WebGLImageConversion::kDataFormatRGBA5551, 10,
             reinterpret_cast<uint8_t*>(&destination_data[0]));
  EXPECT_EQ(0,
            memcmp(expected_data, destination_data, sizeof(destination_data)));
}

TEST_F(WebGLImageConversionTest, ConvertRGBA8ToRGB565) {
  uint8_t source_data[40] = {0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56,
                             0x34, 0x56, 0x34, 0x56, 0x34, 0x56, 0x34, 0x56};
  uint16_t expected_data[10] = {0x32a6, 0x32a6, 0x32a6, 0x32a6, 0x32a6,
                                0x32a6, 0x32a6, 0x32a6, 0x32a6, 0x32a6};
  uint16_t destination_data[10];
  PackPixels(source_data, WebGLImageConversion::kDataFormatRGB565, 10,
             reinterpret_cast<uint8_t*>(&destination_data[0]));
  EXPECT_EQ(0,
            memcmp(expected_data, destination_data, sizeof(destination_data)));
}

}  // namespace blink

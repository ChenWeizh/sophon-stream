//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//
#include <sys/stat.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <unordered_map>

#include "common/clocker.h"
#include "common/error_code.h"
#include "common/object_metadata.h"
#include "common/logger.h"
#include "decode.h"
#include "engine.h"
#include "gtest/gtest.h"
#include "init_engine.h"

const std::vector<std::vector<int>> colors = {
    {255, 0, 0},    {255, 85, 0},    {255, 170, 0},   {255, 255, 0},
    {170, 255, 0},  {85, 255, 0},    {0, 255, 0},     {0, 255, 85},
    {0, 255, 170},  {0, 255, 255},   {0, 170, 255},   {0, 85, 255},
    {0, 0, 255},    {85, 0, 255},    {170, 0, 255},   {255, 0, 255},
    {255, 0, 170},  {255, 0, 85},    {255, 0, 0},     {255, 0, 255},
    {255, 85, 255}, {255, 170, 255}, {255, 255, 255}, {170, 255, 255},
    {85, 255, 255}};

void draw_tracker_bmcv(bm_handle_t& handle, int track_id, int left, int top,
                       int width, int height, bm_image& frame,
                       bool put_text_flag)  // Draw the predicted bounding box
{
  int colors_num = colors.size();
  // Draw a rectangle displaying the bounding box
  bmcv_rect_t rect;
  rect.start_x = left;
  rect.start_y = top;
  rect.crop_w = width;
  rect.crop_h = height;
  std::cout << rect.start_x << "," << rect.start_y << "," << rect.crop_w << ","
            << rect.crop_h << std::endl;
  bmcv_image_draw_rectangle(
      handle, frame, 1, &rect, 3, colors[track_id % colors_num][0],
      colors[track_id % colors_num][1], colors[track_id % colors_num][2]);
  if (put_text_flag) {
    std::string label = std::to_string(track_id);
    bmcv_point_t org = {left, top - 10};
    bmcv_color_t color = {colors[track_id % colors_num][0],
                          colors[track_id % colors_num][1],
                          colors[track_id % colors_num][2]};
    int thickness = 2;
    float fontScale = 1.5;
    if (BM_SUCCESS != bmcv_image_put_text(handle, frame, label.c_str(), org,
                                          color, fontScale, thickness)) {
      std::cout << "bmcv put text error !!!" << std::endl;
    }
  }
}

typedef struct usecase_config_ {
  int num_graphs;
  int num_channels_per_graph;
  nlohmann::json channel_config;
  bool download_image;
  std::string engine_config_file;
} usecase_config;

constexpr const char* JSON_CONFIG_NUM_CHANNELS_PER_GRAPH_FILED =
    "num_channels_per_graph";
constexpr const char* JSON_CONFIG_DOWNLOAD_IMAGE_FILED = "download_image";
constexpr const char* JSON_CONFIG_ENGINE_CONFIG_PATH_FILED =
    "engine_config_path";
constexpr const char* JSON_CONFIG_CHANNEL_CONFIG_FILED = "channel";

usecase_config parse_usecase_json(std::string& json_path) {
  std::ifstream istream;
  istream.open(json_path);
  assert(istream.is_open());
  nlohmann::json usecase_json;
  istream >> usecase_json;
  istream.close();

  usecase_config config;

  config.num_channels_per_graph =
      usecase_json.find(JSON_CONFIG_NUM_CHANNELS_PER_GRAPH_FILED)->get<int>();

  auto channel_config_it = usecase_json.find(JSON_CONFIG_CHANNEL_CONFIG_FILED);
  config.channel_config = *channel_config_it;

  config.download_image =
      usecase_json.find(JSON_CONFIG_DOWNLOAD_IMAGE_FILED)->get<bool>();
  config.engine_config_file =
      usecase_json.find(JSON_CONFIG_ENGINE_CONFIG_PATH_FILED)
          ->get<std::string>();

  if (config.download_image) {
    const char* dir_path = "./results";
    struct stat info;
    if (stat(dir_path, &info) == 0 && S_ISDIR(info.st_mode)) {
      std::cout << "Directory already exists." << std::endl;
    } else {
      if (mkdir(dir_path, 0777) == 0) {
        std::cout << "Directory created successfully." << std::endl;
      } else {
        std::cerr << "Error creating directory." << std::endl;
      }
    }
  }

  return config;
}

TEST(TestByteTrack, TestByteTrack) {
  ::logInit("debug", "");

  std::mutex mtx;
  std::condition_variable cv;

  sophon_stream::common::Clocker clocker;
  std::atomic_uint32_t frameCount(0);
  std::atomic_int32_t finishedChannelCount(0);

  auto& engine = sophon_stream::framework::SingletonEngine::getInstance();

  std::ifstream istream, elem_stream;
  nlohmann::json engine_json;
  std::string bytetrack_config_file = "../config/usecase_bytetrack.json";
  usecase_config bytetrack_json = parse_usecase_json(bytetrack_config_file);

  // 启动每个graph, graph之间没有联系，可以是完全不同的配置
  istream.open(bytetrack_json.engine_config_file);
  assert(istream.is_open());
  istream >> engine_json;
  istream.close();

  bytetrack_json.num_graphs = engine_json.size();
  int num_channels =
      bytetrack_json.num_channels_per_graph * bytetrack_json.num_graphs;

  auto stopHandler = [&](std::shared_ptr<void> data) {
    // write stop data handler here
    auto objectMetadata =
        std::static_pointer_cast<sophon_stream::common::ObjectMetadata>(data);
    if (objectMetadata == nullptr) return;
    frameCount++;
    if (objectMetadata->mFrame->mEndOfStream) {
      printf("meet a eof\n");
      finishedChannelCount++;
      if (finishedChannelCount == num_channels) {
        cv.notify_one();
      }
      return;
    }
    if (bytetrack_json.download_image) {
      int width = objectMetadata->mFrame->mWidth;
      int height = objectMetadata->mFrame->mHeight;
      bm_image image = *objectMetadata->mFrame->mSpData;
      bm_image imageStorage;
      bm_image_create(objectMetadata->mFrame->mHandle, height, width,
                      FORMAT_YUV420P, image.data_type, &imageStorage);
      bmcv_image_storage_convert(objectMetadata->mFrame->mHandle, 1, &image,
                                 &imageStorage);
      for (auto subObj : objectMetadata->mSubObjectMetadatas) {
        // draw image
        draw_tracker_bmcv(objectMetadata->mFrame->mHandle,
                          subObj->mTrackedObjectMetadata->mTrackId,
                          subObj->mDetectedObjectMetadata->mBox.mX,
                          subObj->mDetectedObjectMetadata->mBox.mY,
                          subObj->mDetectedObjectMetadata->mBox.mWidth,
                          subObj->mDetectedObjectMetadata->mBox.mHeight,
                          imageStorage, true);
      }
      // save image
      void* jpeg_data = NULL;
      size_t out_size = 0;
      int ret = bmcv_image_jpeg_enc(objectMetadata->mFrame->mHandle, 1,
                                    &imageStorage, &jpeg_data, &out_size);
      if (ret == BM_SUCCESS) {
        std::string img_file =
            "./results/" + std::to_string(objectMetadata->mFrame->mChannelId) +
            "_" + std::to_string(objectMetadata->mFrame->mFrameId) + ".jpg";
        FILE* fp = fopen(img_file.c_str(), "wb");
        fwrite(jpeg_data, out_size, 1, fp);
        fclose(fp);
      }
      free(jpeg_data);
      bm_image_destroy(imageStorage);
    }
  };

  std::map<int, std::pair<int, int>> graph_src_id_port_map;
  init_engine(engine, engine_json, stopHandler, graph_src_id_port_map);

  for (auto graph_id : engine.getGraphIds()) {
    for (int channel_id = 0; channel_id < bytetrack_json.num_channels_per_graph;
         ++channel_id) {
      nlohmann::json channel_config = bytetrack_json.channel_config;
      channel_config["channel_id"] = channel_id;
      auto channelTask =
          std::make_shared<sophon_stream::element::decode::ChannelTask>();
      channelTask->request.operation = sophon_stream::element::decode::
          ChannelOperateRequest::ChannelOperate::START;
      channelTask->request.channelId = channel_id;
      channelTask->request.json = channel_config.dump();
      std::pair<int, int> src_id_port = graph_src_id_port_map[graph_id];
      sophon_stream::common::ErrorCode errorCode =
          engine.pushInputData(graph_id, src_id_port.first, src_id_port.second,
                               std::static_pointer_cast<void>(channelTask));
    }
  }

  {
    std::unique_lock<std::mutex> uq(mtx);
    cv.wait(uq);
  }
  for (int i = 0; i < bytetrack_json.num_graphs; i++) {
    std::cout << "graph stop" << std::endl;
    engine.stop(i);
  }
  long totalCost = clocker.tell_us();
  std::cout << " total time cost " << totalCost << " us." << std::endl;
  double fps = static_cast<double>(frameCount) / totalCost;
  std::cout << "frame count is " << frameCount << " | fps is " << fps * 1000000
            << " fps." << std::endl;
}

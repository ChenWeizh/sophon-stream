//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#ifndef SOPHON_STREAM_ELEMENT_LPRNET_POST_PROCESS_H_
#define SOPHON_STREAM_ELEMENT_LPRNET_POST_PROCESS_H_

#include "algorithmApi/post_process.h"
#include "lprnet_context.h"

namespace sophon_stream {
namespace element {
namespace lprnet {

class LprnetPostProcess : public ::sophon_stream::element::PostProcess {
 public:
  void init(std::shared_ptr<LprnetContext> context);
  /**
   * @brief 对一个batch的数据做后处理
   * @param context context指针
   * @param objectMetadatas 一个batch的数据
   */
  void postProcess(std::shared_ptr<LprnetContext> context,
                   common::ObjectMetadatas& objectMetadatas);

 private:
  std::string get_res(int pred_num[], int len_char, int clas_char);
  int argmax(float* data, int num);
};

}  // namespace lprnet
}  // namespace element
}  // namespace sophon_stream

#endif  // SOPHON_STREAM_ELEMENT_LPRNET_POST_PROCESS_H_
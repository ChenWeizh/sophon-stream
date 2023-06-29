//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "converger.h"

#include <nlohmann/json.hpp>

#include "common/logger.h"
#include "element_factory.h"

namespace sophon_stream {
namespace element {
namespace converger {

Converger::Converger() {}
Converger::~Converger() {}

common::ErrorCode Converger::initInternal(const std::string& json) {
  common::ErrorCode errorCode = common::ErrorCode::SUCCESS;
  do {
    auto configure = nlohmann::json::parse(json, nullptr, false);
    if (!configure.is_object()) {
      errorCode = common::ErrorCode::PARSE_CONFIGURE_FAIL;
      break;
    }
    int _default_port =
        configure.find(CONFIG_INTERNAL_DEFAULT_PORT_FILED)->get<int>();
    default_port = _default_port;
  } while (false);
  return errorCode;
}

void Converger::uninitInternal() {}

common::ErrorCode Converger::doWork(int dataPipeId) {
  common::ErrorCode errorCode = common::ErrorCode::SUCCESS;
  std::vector<int> inputPorts = getInputPorts();
  std::vector<int> outputPorts = getOutputPorts();
  int outputPort = getSinkElementFlag() ? 0 : outputPorts[0];

  // 从所有inputPort中取出数据，并且做判断
  // default_port中取出的数据，放到map里
  auto data = popInputData(default_port, dataPipeId);
  while (!data && (getThreadStatus() == ThreadStatus::RUN)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    data = popInputData(default_port, dataPipeId);
  }
  if (data == nullptr) return common::ErrorCode::SUCCESS;

  auto objectMetadata = std::static_pointer_cast<common::ObjectMetadata>(data);
  int channel_id = objectMetadata->mFrame->mChannelIdInternal;
  int frame_id = objectMetadata->mFrame->mFrameId;
  candidates[channel_id][frame_id] = objectMetadata;
  // branches[channel_id][frame_id] = objectMetadata->numBranches;
  IVS_DEBUG("data recognized, channel_id = {0}, frame_id = {1}", channel_id, frame_id);

  // 非default_port，取出来之后更新分支数的记录
  for (int inputPort : inputPorts) {
    if (inputPort == default_port) continue;
    auto subdata = popInputData(inputPort, dataPipeId);
    // 这里不能在while里取，否则会堵住
    // while (!subdata && (getThreadStatus() == ThreadStatus::RUN)) {
    //   std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //   subdata = popInputData(inputPort, dataPipeId);
    // }
    if (subdata == nullptr) continue;
    auto subObj = std::static_pointer_cast<common::ObjectMetadata>(subdata);
    int sub_channel_id = subObj->mFrame->mChannelId;
    int sub_frame_id = subObj->mFrame->mFrameId;
    IVS_DEBUG("subData recognized, channel_id = {0}, frame_id = {0}", sub_channel_id, sub_frame_id);
    // 更新map时，判断一下是否存在
    if (branches.find(sub_channel_id) != branches.end() &&
        branches[sub_channel_id].find(sub_frame_id) !=
            branches[sub_channel_id].end()) {
      branches[sub_channel_id][sub_frame_id]++;
      IVS_DEBUG("data updated, channel_id = {0}, frame_id = {1}", channel_id, frame_id);
    }
  }

  // 遍历map，能弹出去的都弹出去
  for (auto channel_it = candidates.begin(); channel_it != candidates.end();
       ++channel_it) {
    // 第一层：遍历所有channel
    int channel_id_internal = channel_it->first;
    for (auto frame_it = candidates[channel_id_internal].begin();
         frame_it != candidates[channel_id_internal].end();) {
      // 第二层：遍历当前channel下的所有frame，有序
      int frame_id = frame_it->first;
      // 如果可以弹出，则弹出并循环至下一个
      if (branches[channel_id_internal][frame_id] == candidates[channel_id_internal][frame_id]->numBranches) {
        IVS_DEBUG("Data converged! Now pop... channel_id = {0}, frame_id = {1}", channel_id_internal, frame_id);
        auto obj = candidates[channel_id_internal][frame_id];
        int outDataPipeId = getSinkElementFlag()
                                ? 0
                                : (channel_id_internal %
                                   getOutputConnectorCapacity(outputPort));
        errorCode = pushOutputData(outputPort, outDataPipeId,
                                   std::static_pointer_cast<void>(obj));
        if (common::ErrorCode::SUCCESS != errorCode) {
          IVS_WARN(
              "Send data fail, element id: {0:d}, output port: {1:d}, data: "
              "{2:p}",
              getId(), outputPort, static_cast<void*>(obj.get()));
        }
        candidates[channel_id_internal].erase(frame_it++);
        // branches[channel_id_internal][frame_id] 应该也需要删除，否则内存越积越多？
      } else {
        // 当前帧不可以弹出，为了保证时序性，后续帧也不弹出
        break;
      }
    }
  }
  return errorCode;
}
REGISTER_WORKER("converger", Converger)

}  // namespace converger
}  // namespace element
}  // namespace sophon_stream
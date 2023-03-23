#pragma once

#include "../Context.h"

namespace sophon_stream {
namespace algorithm {
namespace context {

struct HostContext :public algorithm::Context {

    std::vector<float> result;

    /**
     * context初始化
     * @param[in] json: 初始化的json字符串
     * @return 错误码
     */
    common::ErrorCode init(const std::string& json) override;
};
} // namespace context
} // namespace algorithm
} // namespace sophon_stream

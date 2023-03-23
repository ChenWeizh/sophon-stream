#include "HostEncodeFactory.h"
#include "../AlgorithmFactorySelector.h"
#include "../pre_process/HostEncodePre.h"
#include "../inference/NoopInference.h"
#include "../post_process/NoopPostprocess.h"
#include "../context/HostContext.h"

namespace sophon_stream {
namespace algorithm {
namespace factory {


/**
 * 创建context
 * @return context对象
 */
std::shared_ptr<algorithm::Context> HostEncodeFactory::makeContext() {
    return std::static_pointer_cast<algorithm::Context>(std::make_shared<context::HostContext>());
}

/**
 * 创建预处理
 * @return 预处理对象
 */
std::shared_ptr<algorithm::PreProcess> HostEncodeFactory::makePreProcess() {
    return std::static_pointer_cast<algorithm::PreProcess>(std::make_shared<pre_process::HostEncodePre>());
}

/**
 * 创建推理
 * @return 推理对象
 */
std::shared_ptr<algorithm::Inference> HostEncodeFactory::makeInference() {
    return std::static_pointer_cast<algorithm::Inference>(std::make_shared<inference::InferenceNoop>());
}

/**
 * 创建后处理
 * @return 后处理对象
 */
std::shared_ptr<algorithm::PostProcess> HostEncodeFactory::makePostProcess() {
    return std::static_pointer_cast<algorithm::PostProcess>(std::make_shared<post_process::PostprocessNoop>());
}

//编码工厂注册
REGISTER_ALGORITHM_FACTORY("host", "encode_picture", HostEncodeFactory);

} // namespace factory
} // namespace algorithm
} // namespace sophon_stream

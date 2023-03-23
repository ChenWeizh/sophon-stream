#pragma once

#include "../Process.h"
#include "../../share/common/ff_decode.hpp"
#include "bmruntime_interface.h"
// #include "../process/decode/gpu/FFmpegFormatInput-new.h"
// #include "../process/decode/gpu/FFmpegDecoder-new.h"

namespace sophon_stream {
namespace multimedia {
namespace process {

/**
 * face transformations gpu process class
 */
class SophgoDecode : public multimedia::Process {
  public :
    SophgoDecode();

    common::ErrorCode init(multimedia::Context& context);
    /**
     * preprocess
     * @param[in] context: input and output config
     * @param[in] objectMetadatas: inputData
     * @return preprocess error code or common::ErrorCode::SUCCESS
     */
    common::ErrorCode process(multimedia::Context& context,
                                 std::shared_ptr<common::ObjectMetadata>& objectMetadata) override;
    void uninit();

  private:
    std::string mStrError;
    bm_handle_t m_handle;
    VideoDecFFM decoder;
    // static std::map<int, std::shared_ptr<decode::gpu::FFmpegHwDevice>> mSMapHwDevice;
    // std::shared_ptr<decode::gpu::FFmpegFormatInput> mSpFormatInput = nullptr;
    // std::shared_ptr<decode::gpu::FFmpegDecoder> mSpDecoder = nullptr;

    std::string mUrl;
    float mResizeRate = 1.0f;
    int mTimeout = 0;
    int mSourceType = 0;//0视频文件1是文件夹2是rtsp或rtmp

};

} // namespace process
} // namespace multimedia
} // namespace sophon_stream

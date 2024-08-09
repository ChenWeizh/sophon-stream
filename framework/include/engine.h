//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//
//版权声明: 这部分说明了代码的版权归属，并表明代码在 2-Clause BSD License 下发布。
//头文件保护: 这个机制用于防止头文件被多次包含，避免重复定义。

//头文件保护宏 (ifndef 和 define): 防止头文件在编译时被多次包含，避免重复定义和冲突
#ifndef SOPHON_STREAM_FRAMEWORK_ELEMENT_ENGINE_H_
#define SOPHON_STREAM_FRAMEWORK_ELEMENT_ENGINE_H_

//#include: 引入标准库和其他模块的头文件，这些文件提供了所需的类和函数。
//<chrono>: 用于处理时间相关的操作。
//<map>: 提供键值对数据结构。
//<memory>: 提供智能指针（如 std::shared_ptr）。
//<string>: 提供字符串处理功能。
#include <chrono>
#include <map>
#include <memory>
#include <string>

//sophon_stream::common 命名空间下的头文件: 包含了错误代码处理、日志记录、不可复制性（NoCopyable）和单例模式（Singleton）的实现。
//graph.h: 包含了 Graph 类的定义，这是 Engine 类的核心管理对象。
#include "common/error_code.h"
#include "common/logger.h"
#include "common/no_copyable.h"
#include "common/singleton.h"
#include "graph.h"

//namespace sophon_stream::framework: 将 Engine 类放在 sophon_stream::framework 命名空间中，组织代码并避免命名冲突。
//class Engine : public NoCopyable: 定义 Engine 类并继承自 NoCopyable，这意味着 Engine 对象不能被复制或赋值
namespace sophon_stream {
namespace framework {

//using SinkHandler = framework::Graph::SinkHandler;: 定义 SinkHandler 类型别名，表示处理数据输出的回调函数。
//start、stop、pause、resume: 这些方法分别用于启动、停止、暂停和恢复指定 graphId 对应的图。
class Engine : public ::sophon_stream::common::NoCopyable {
 public:
  using SinkHandler = framework::Graph::SinkHandler;

  common::ErrorCode start(int graphId);

  common::ErrorCode stop(int graphId);

  common::ErrorCode pause(int graphId);

  common::ErrorCode resume(int graphId);

//addGraph: 从配置文件（JSON格式）初始化一个有向无环图（DAG），并将线程状态设置为 RUN。
  /**
   * @brief 从配置文件初始化一个有向无环图，并将线程状态设置为RUN
   */
  common::ErrorCode addGraph(const std::string& json);

//removeGraph: 移除指定 graphId 对应的图。
//graphExist: 检查指定 graphId 对应的图是否存在。
  void removeGraph(int graphId);

  bool graphExist(int graphId);

//pushSourceData: 向指定的图的元素推送数据，通常用于启动解码任务。
  /**
   * @brief 向指定graph的指定element推入数据，用于向decode
   * element发送启动任务的信号
   */
  common::ErrorCode pushSourceData(int graphId, int elementId, int inputPort,
                                   std::shared_ptr<void> data);

//setSinkHandler: 为指定图的指定元素设置 SinkHandler，SinkHandler 只有在元素是 sink 元素时才生效。
/**
   * @brief
   * 为指定graph的指定element设置sinkHandler，sinkHandler当且仅当指定element是sink
   * element时才生效
   */
  void setSinkHandler(int graphId, int elementId, int outputPort,
                      SinkHandler sinkHandler);

//getSideAndDeviceId: 获取指定图和元素的设备ID和侧边信息。
//getGraphIds: 返回所有图的ID列表。
  std::pair<std::string, int> getSideAndDeviceId(int graphId, int elementId);

  std::vector<int> getGraphIds();

//getListener 和 setListener: 获取或设置监听线程，用于异步处理和监控系统状态。
  inline ListenThread* getListener() { return listenThreadPtr; }

  inline void setListener(ListenThread* p) { listenThreadPtr = p; }

//JSON_GRAPH_ID_FIELD: 用于标识 JSON 配置文件中的 graph_id 字段。
  static constexpr const char* JSON_GRAPH_ID_FIELD = "graph_id";

 private:
//friend class common::Singleton<Engine>: 允许 Singleton 类访问 Engine 的私有成员，支持单例模式。
//Engine() 和 ~Engine(): 构造函数和析构函数，构造函数为私有，确保 Engine 只能通过单例模式实例化。
  friend class common::Singleton<Engine>;

  Engine();

  ~Engine();

//mGraphMap: 用于存储图的映射，graphId 映射到 Graph 对象的共享指针。
//mGraphMapLock: 互斥锁，用于保护对 mGraphMap 的访问，确保线程安全。
//mGraphIds: 存储图的ID列表。
listenThreadPtr: 指向监听线程的指针。
  std::map<int /* graphId */, std::shared_ptr<framework::Graph> > mGraphMap;
  std::mutex mGraphMapLock;

  std::vector<int> mGraphIds;

  ListenThread* listenThreadPtr;
};

//using SingletonEngine = common::Singleton<Engine>;: 定义 SingletonEngine 为 Engine 类的单例实例类型，确保全局只有一个 Engine 实例。
//命名空间关闭: 结束 framework 和 sophon_stream 命名空间的定义。
using SingletonEngine = common::Singleton<Engine>;

}  // namespace framework
}  // namespace sophon_stream

//#endif: 结束头文件保护宏。
#endif  // SOPHON_STREAM_FRAMEWORK_ENGINE_H_

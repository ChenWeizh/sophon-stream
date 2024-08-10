//这段代码实现了 Engine 类的方法。Engine 类是 SOPHON-STREAM 框架的核心组件之一，负责管理图（Graph）的生命周期
//#include "engine.h": 引入 Engine 类的头文件，包含类的声明。
//#include "common/logger.h": 引入日志记录模块，提供日志记录的功能。
#include "engine.h"

#include "common/logger.h"
//namespace sophon_stream::framework: 将代码置于 sophon_stream::framework 命名空间中，组织代码并避免命名冲突。
namespace sophon_stream {
namespace framework {

//Engine::Engine(): 构造函数，初始化 Engine 对象。这里未做任何操作。
//Engine::~Engine(): 析构函数，销毁 Engine 对象。这里未做任何操作。
Engine::Engine() {}

Engine::~Engine() {}

//IVS_INFO: 记录信息级别的日志，表示操作的开始和结束。{0:d} 是格式化字符串，表示一个整数。
//std::lock_guard<std::mutex> lk(mGraphMapLock): 使用锁保护 mGraphMap，确保在多线程环境中安全访问。
//auto graphIt = mGraphMap.find(graphId): 在 mGraphMap 中查找 graphId 对应的图对象。
//if (mGraphMap.end() == graphIt): 如果找不到 graphId，记录错误日志并返回错误代码 NO_SUCH_GRAPH_ID。
//auto graph = graphIt->second: 获取找到的图对象。
//if (!graph): 如果图对象为空，记录错误日志并返回错误代码 UNKNOWN。
//return graph->start(): 启动图对象，返回操作结果。
common::ErrorCode Engine::start(int graphId) {
  IVS_INFO("Engine start graph thread start, graph id: {0:d}", graphId);
  std::lock_guard<std::mutex> lk(mGraphMapLock);
  auto graphIt = mGraphMap.find(graphId);
  if (mGraphMap.end() == graphIt) {
    IVS_ERROR("Can not find graph, graph id: {0:d}", graphId);
    return common::ErrorCode::NO_SUCH_GRAPH_ID;
  }

  auto graph = graphIt->second;
  if (!graph) {
    IVS_ERROR("Graph is null, graph id: {0:d}", graphId);
    return common::ErrorCode::UNKNOWN;
  }

  IVS_INFO("Engine start graph thread finish, graph id: {0:d}", graphId);
  return graph->start();
}

//逻辑与 start 方法类似，但这个方法是停止图对象
common::ErrorCode Engine::stop(int graphId) {
  IVS_INFO("Engine stop graph thread start, graph id: {0:d}", graphId);

  std::lock_guard<std::mutex> lk(mGraphMapLock);
  auto graphIt = mGraphMap.find(graphId);
  if (mGraphMap.end() == graphIt) {
    IVS_ERROR("Can not find graph, graph id: {0:d}", graphId);
    return common::ErrorCode::NO_SUCH_GRAPH_ID;
  }

  auto graph = graphIt->second;
  if (!graph) {
    IVS_ERROR("Graph is null, graph id: {0:d}", graphId);
    return common::ErrorCode::UNKNOWN;
  }

  IVS_INFO("Engine stop graph thread finish, graph id: {0:d}", graphId);
  return graph->stop();
}

//逻辑与 start 方法类似，但这个方法是暂停图对象
common::ErrorCode Engine::pause(int graphId) {
  IVS_INFO("Engine pause graph thread start, graph id: {0:d}", graphId);

  std::lock_guard<std::mutex> lk(mGraphMapLock);
  auto graphIt = mGraphMap.find(graphId);
  if (mGraphMap.end() == graphIt) {
    IVS_ERROR("Can not find graph, graph id: {0:d}", graphId);
    return common::ErrorCode::NO_SUCH_GRAPH_ID;
  }

  auto graph = graphIt->second;
  if (!graph) {
    IVS_ERROR("Graph is null, graph id: {0:d}", graphId);
    return common::ErrorCode::UNKNOWN;
  }

  IVS_INFO("Engine pause graph thread finish, graph id: {0:d}", graphId);
  return graph->pause();
}


//逻辑与 start 方法类似，但这个方法是恢复图对象。
common::ErrorCode Engine::resume(int graphId) {
  IVS_INFO("Engine resume graph thread start, graph id: {0:d}", graphId);

  std::lock_guard<std::mutex> lk(mGraphMapLock);
  auto graphIt = mGraphMap.find(graphId);
  if (mGraphMap.end() == graphIt) {
    IVS_ERROR("Can not find graph, graph id: {0:d}", graphId);
    return common::ErrorCode::NO_SUCH_GRAPH_ID;
  }

  auto graph = graphIt->second;
  if (!graph) {
    IVS_ERROR("Graph is null, graph id: {0:d}", graphId);
    return common::ErrorCode::UNKNOWN;
  }

  IVS_INFO("Engine resume graph thread finish, graph id: {0:d}", graphId);
  return graph->resume();
}

//IVS_INFO("Add graph start, json: {0}", json): 记录添加图的操作开始，打印 JSON 配置信息。
//common::ErrorCode errorCode = common::ErrorCode::SUCCESS: 初始化错误码为 SUCCESS。
//do { ... } while (false): 使用 do-while 模式确保代码块内的逻辑至少执行一次。
//std::lock_guard<std::mutex> lk(mGraphMapLock): 锁定互斥量以确保线程安全。
//auto graph = std::make_shared<framework::Graph>(): 创建一个新的 Graph 对象的共享指针。
//graph->setListener(listenThreadPtr): 为图设置监听线程。
//errorCode = graph->init(json): 使用传入的 JSON 配置初始化图对象。
//listenThreadPtr->report_status(errorCode): 报告初始化的状态。
//if (common::ErrorCode::SUCCESS != errorCode): 如果初始化失败，记录错误并返回错误码。
//errorCode = graph->start(): 尝试启动图对象。
//mGraphMap[graph->getId()] = graph: 将新创建的图对象添加到图映射表中。
//mGraphIds.push_back(graph->getId()): 将图的 ID 添加到图 ID 列表中。
common::ErrorCode Engine::addGraph(const std::string& json) {
  IVS_INFO("Add graph start, json: {0}", json);

  common::ErrorCode errorCode = common::ErrorCode::SUCCESS;

  do {
    std::lock_guard<std::mutex> lk(mGraphMapLock);

    auto graph = std::make_shared<framework::Graph>();
    graph->setListener(listenThreadPtr);
    errorCode = graph->init(json);
    listenThreadPtr->report_status(errorCode);
    if (common::ErrorCode::SUCCESS != errorCode) {
      IVS_ERROR("Graph init fail, json: {0}", json);
      return errorCode;
    }

    errorCode = graph->start();
    listenThreadPtr->report_status(errorCode);

    if (common::ErrorCode::SUCCESS != errorCode) {
      IVS_ERROR("Graph start fail");
      return errorCode;
    }

    mGraphMap[graph->getId()] = graph;
    IVS_INFO("Add graph finish, json: {0}", json);
    mGraphIds.push_back(graph->getId());

  } while (false);

  return errorCode;
}

//mGraphMap.erase(graphId): 从图映射表中移除指定的图对象。
void Engine::removeGraph(int graphId) {
  std::lock_guard<std::mutex> lk(mGraphMapLock);
  IVS_INFO("Remove graph start, graph id: {0:d}", graphId);
  mGraphMap.erase(graphId);
  IVS_INFO("Remove graph finish, graph id: {0:d}", graphId);
}


//mGraphMap.find(graphId): 检查图映射表中是否存在指定的图 ID。
bool Engine::graphExist(int graphId) {
  std::lock_guard<std::mutex> lk(mGraphMapLock);

  if (mGraphMap.end() != mGraphMap.find(graphId)) {
    return true;
  }
  return false;
}

//setSinkHandler: 设置图中指定元素的输出端口的处理器。
void Engine::setSinkHandler(int graphId, int elementId, int outputPort,
                            SinkHandler sinkHandler) {
  IVS_INFO(
      "Set data handler, graph id: {0:d}, element id: {1:d}, output port: "
      "{2:d}",
      graphId, elementId, outputPort);

  auto graphIt = mGraphMap.find(graphId);
  if (mGraphMap.end() == graphIt) {
    IVS_ERROR("Can not find graph, graph id: {0:d}", graphId);
    return;
  }

  auto graph = graphIt->second;
  if (!graph) {
    IVS_ERROR("Graph is null, graph id: {0:d}", graphId);
    return;
  }

  graph->setSinkHandler(elementId, outputPort, sinkHandler);
}

//pushSourceData: 向图中的指定元素输入数据
common::ErrorCode Engine::pushSourceData(int graphId, int elementId,
                                         int inputPort,
                                         std::shared_ptr<void> data) {
  IVS_DEBUG(
      "send data, graph id: {0:d}, element id: {1:d}, input port: {2:d}, "
      "data: {3:p}",
      graphId, elementId, inputPort, data.get());

  auto graphIt = mGraphMap.find(graphId);
  if (mGraphMap.end() == graphIt) {
    IVS_ERROR("Can not find graph, graph id: {0:d}", graphId);
    return common::ErrorCode::NO_SUCH_GRAPH_ID;
  }

  auto graph = graphIt->second;
  if (!graph) {
    IVS_ERROR("Graph is null, graph id: {0:d}", graphId);
    return common::ErrorCode::UNKNOWN;
  }

  return graph->pushSourceData(elementId, inputPort, data);
}

//getSideAndDeviceId: 获取图中指定元素的设备信息。   
std::pair<std::string, int> Engine::getSideAndDeviceId(int graphId,
                                                       int elementId) {
  IVS_INFO("Get side and device id, graph id: {0:d}, element id: {1:d}",
           graphId, elementId);

  auto graphIt = mGraphMap.find(graphId);
  if (mGraphMap.end() == graphIt) {
    IVS_ERROR("Can not find graph, graph id: {0:d}", graphId);
    return std::make_pair("", -1);
  }

  auto graph = graphIt->second;
  if (!graph) {
    IVS_ERROR("Graph is null, graph id: {0:d}", graphId);
    return std::make_pair("", -1);
  }

  return graph->getSideAndDeviceId(elementId);
}
//getGraphIds: 返回当前所有图的 ID 列表
std::vector<int> Engine::getGraphIds() { return mGraphIds; }

}  // namespace framework
}  // namespace sophon_stream

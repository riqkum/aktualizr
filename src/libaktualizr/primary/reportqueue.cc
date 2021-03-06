#include "reportqueue.h"

#include <chrono>

#include "config/config.h"

ReportQueue::ReportQueue(const Config& config_in, std::shared_ptr<HttpInterface> http_client)
    : config(config_in), http(std::move(http_client)), shutdown_(false) {
  thread_ = std::thread(std::bind(&ReportQueue::run, this));
}

ReportQueue::~ReportQueue() {
  shutdown_ = true;
  {
    std::unique_lock<std::mutex> thread_lock(thread_mutex_);
    cv_.notify_all();
  }
  thread_.join();
}

void ReportQueue::run() {
  // Check if queue is nonempty. If so, move any reports to the Json array and
  // try to send it to the server. Clear the Json array only if the send
  // succeeds.
  Json::Value report_array(Json::arrayValue);
  while (!shutdown_) {
    std::unique_lock<std::mutex> thread_lock(thread_mutex_);
    if (!report_queue_.empty()) {
      std::lock_guard<std::mutex> queue_lock(queue_mutex_);
      while (!report_queue_.empty()) {
        report_array.append(*report_queue_.front());
        report_queue_.pop();
      }
    }
    if (report_array.size() > 0) {
      HttpResponse response = http->post(config.tls.server + "/events", report_array);
      // 404 implies the server does not support this feature. Nothing we can
      // do, just move along.
      if (response.isOk() || response.http_status_code == 404) {
        report_array = Json::arrayValue;
      }
    }
    cv_.wait_for(thread_lock, std::chrono::seconds(10));
  }
}

void ReportQueue::enqueue(std::unique_ptr<Json::Value> report) {
  {
    std::lock_guard<std::mutex> queue_lock(queue_mutex_);
    report_queue_.push(std::move(report));
  }
  cv_.notify_all();
}

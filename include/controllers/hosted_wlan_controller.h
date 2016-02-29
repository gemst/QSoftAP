#pragma once

#include <string>
#include <memory>

#include "wlan/hosted_wlan.h"

namespace controllers {

class HostedWlanController {
 public:
  HostedWlanController(std::shared_ptr<wlan::HostedWlan> hosted_wlan)
      : wlan_(hosted_wlan) {}

  /**
   * Ӧ������
   * 
   * @param[in] ssid
   * @param[in] key
   * @param[in] sharing_conn �������ӵ�id�����Ϊ����ȡ������
   * @return ������Ϣ���մ���ʾ�ɹ�
   */
  QString applyConfigs(const std::string &ssid, const std::string &key,
                            const std::string &sharing_conn);

 private:
  std::shared_ptr<wlan::HostedWlan> wlan_;
};
}
#pragma once

#include <string>

#include <QString>

#include <windows.h>
#include <Wlanapi.h>

namespace wlan {

/**
 * ��ʾ����wlan�е�һ̨����
 */
class WlanHost {
 public:
  WlanHost() {}
  WlanHost(const WLAN_HOSTED_NETWORK_PEER_STATE &peer);
  WLAN_HOSTED_NETWORK_PEER_STATE peerState() const { return this->peer_state_; }
  std::string mac_address_hex;

 private:
  WLAN_HOSTED_NETWORK_PEER_STATE peer_state_;
};
}

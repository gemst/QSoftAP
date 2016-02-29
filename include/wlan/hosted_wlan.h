#pragma once

#include <memory>
#include <map>

#include <QObject>

#include <windows.h>
#include <Wlanapi.h>

#include "commons.h"
#include "wlan/wlan_host.h"

namespace wlan {

/**
 * �й�WLAN����native api�ķ�װ
 */
class HostedWlan : public QObject,
                   public std::enable_shared_from_this<HostedWlan> {
  Q_OBJECT

  NON_COPYABLE(HostedWlan)
 public:
  static QString getErrorMsg(DWORD code);
  static std::string getFailReason(WLAN_HOSTED_NETWORK_REASON code);

  HostedWlan();
  ~HostedWlan();

  /**
   * ��������
   */
  WLAN_HOSTED_NETWORK_REASON setProperties(const std::string &ssid,
                                           const std::string &key,
                                           DWORD dwMaxNumberOfPeers = 100);

  /**
   * �����Ƿ�����
   *
   * @param enable,TURE �� FALSE
   */
  WLAN_HOSTED_NETWORK_REASON setEnabled(BOOL enable);

  //��
  WLAN_HOSTED_NETWORK_REASON turnOn();

  //�ر�
  WLAN_HOSTED_NETWORK_REASON turnOff();

  //�ڡ����������ء����л�
  WLAN_HOSTED_NETWORK_REASON toggle();

  /**
   * ���¶�ȡ������Ϣ������ssid�������.
   *
   * @return ���ش����룬�ɹ�ΪERROR_SUCCESS����ͨ��getErrorMsg��ȡ��������
   */
  DWORD updateProperties();

  /**
   * ���µ�ǰ״̬.
   *
   * @return ���ش����룬�ɹ�ΪERROR_SUCCESS����ͨ��getErrorMsg��ȡ��������
   */
  DWORD updateStatus();

  std::string ssid() const { return this->ssid_; }

  //��ȡ��Կ
  std::string secondaryKey() const { return this->secondary_key_; }

  std::string adapterID() const { return this->adapter_id_; }

  WLAN_HOSTED_NETWORK_STATE state() const { return this->state_; }

  /**
   * ����wlan�е��������ϣ�����mac
   */
  std::map<std::string, std::shared_ptr<WlanHost>> peers;

 signals:
  //����������
  void peerJoined(std::shared_ptr<HostedWlan> sender,
                  std::shared_ptr<WlanHost> peer);
  //�������Ͽ�
  void peerLeaved(std::shared_ptr<HostedWlan> sender, std::string mac);
  //����wifi״̬�����ı�
  void stateChanged(std::shared_ptr<HostedWlan> sender);

 private:
  static void wlanCallback(PWLAN_NOTIFICATION_DATA data, PVOID ctx);

  void init();
  void release();

  HANDLE client_handle_ = nullptr;
  std::string ssid_;
  std::string secondary_key_;
  DWORD dw_number_of_max_peers_    = 100;
  WLAN_HOSTED_NETWORK_STATE state_ = {};
  std::string adapter_id_;
  GUID adapter_id_raw_           = {};
  DOT11_MAC_ADDRESS mac_address_ = {};
};
}

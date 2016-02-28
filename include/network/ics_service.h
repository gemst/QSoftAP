#pragma once

#include <vector>

#include <QObject>

#include "commons.h"

struct INetSharingManager;

namespace network {

class IcsConnection;

//�ṩICS����Ĺ���
class IcsService : public QObject {
  Q_OBJECT

  SINGLETON(IcsService)
 public:
  ~IcsService();

  void reloadConnections();

  /**
   * ���ù���
   *
   * @param[in] pub ��Ϊpublic���ӵ�guid
   * @param[in] prv ��Ϊprivate���ӵ�guid
   */
  void enableSharing(const std::string &pub, const std::string &prv);
  void disableAll();

  /**
   * ����id������
   *
   * @param[in] id
   * @param[out] result
   * @return �Ƿ��ҵ�
   */
  bool findById(const std::string &id, IcsConnection &result);
  std::vector<IcsConnection> connections;

 signals:
  //���Ӽ�����󷢳�
  void reloadFinished(IcsService &sender);

 protected:
  IcsService();

 private:
  INetSharingManager *sharing_manager_ = nullptr;
};
}
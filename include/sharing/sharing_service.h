#pragma once

#include <vector>

#include <QObject>

#include <windows.h>
#include <Lmshare.h>

#include "commons.h"

class QString;

namespace sharing
{
struct SharedResource;

/**
 * �ṩ������Դ�����ܡ���native api�ļ򵥷�װ
 */
class SharingService : public QObject
{
    Q_OBJECT

    SINGLETON(SharingService)
  signals:
    void sharesChanged();
  public:
    static QString getErrorString(DWORD code);

    NET_API_STATUS add(const SharedResource &res);
    NET_API_STATUS remove(const std::wstring &name);
    std::vector<SharedResource> all();
  protected:
    SharingService() {}
};
}
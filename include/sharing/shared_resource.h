#pragma once

#include <string>

#include <windows.h>
#include <Lmshare.h>
#include <LMaccess.h>

namespace sharing {

/**
 * ��ʾһ��������Դ
 */
struct SharedResource {
  std::wstring name;
  std::wstring path;

  /**
   * ��Դ����
   *
   * STYPE_DISKTREE:Disk drive.
   * STYPE_PRINTQ:Print queue.
   * STYPE_DEVICE:Communication device.
   * STYPE_IPC:Interprocess communication(IPC).
   */
  DWORD type = STYPE_DISKTREE;

  DWORD permissions = ACCESS_ALL;
  int max_uses      = -1;
};

}
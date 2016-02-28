#pragma once

#include <string>

#include <windows.h>

namespace network
{
/**
 * �ṩ������صĹ��ߺ���
 */
class NetUtils
{
  public:
    /**
     * ��mac��ַת��16�����ַ�����ʽ
     */
    static std::string readableMacAddress(unsigned char bytes[], size_t len = 6, char delim = '-');

    /**
     * ����macȡip
     * @param[in] mac 6���ֽ�,����ʮ�������ַ���
     */
    static DWORD findIpByMac(unsigned char mac[6]);
};
}

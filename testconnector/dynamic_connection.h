/**
  Copyright (c) 2026 Melin Software HB
*/

#pragma once

#include "../connector_interface.h"

#include <Windows.h>
#include <string>
#include <vector>
#include <utility>

class DynamicConnection {
  std::wstring name;
  std::wstring shortName;

  void(__cdecl *initPtr)(MConDeviceProps *supportProps, MConWString *deviceHelp, MConWString *startParameterName, MConAddCard addCard, MConAddPunch addPunch) = nullptr;
  int(__cdecl *enumerateDevicesPtr)(int maxNumEntries, MConWString *deviceVectorDescr, bool *deviceVectorConnected) = nullptr;
  void(__cdecl *statusPtr)(int deviceIndex, MConDeviceStatus *status) = nullptr;
  int(__cdecl *autoDetectPtr)(int maxNumEntries, int *deviceIx) = nullptr;

  int(__cdecl *openConnectionPtr)(int deviceIndex, bool listenOnly, MConWString parameter, MConLogMessage logger) = nullptr;
  void(__cdecl *closeConnectionPtr)(int deviceIndex) = nullptr;
  MConWString(__cdecl *monitorThreadPtr)(int deviceIndex) = nullptr;
  void(__cdecl *unloadPtr)() = nullptr;


  HMODULE hModule = 0;

  FARPROC loadProc(const char *name, bool allowMissing = false) const;

  std::wstring parameterName;
  std::wstring parameterDefault;
  std::wstring parameterHelp;

  std::wstring deviceHelpStr;

  bool supportAutoDetect = false;

  int numDevices = 0;

public:

  struct DeviceStatus {
    std::wstring name;
    std::vector<std::pair<std::wstring, bool>> messages;
    bool connected = false;
    bool requireFunctionMapping = false;
  };

  DynamicConnection(const std::wstring &library);
  ~DynamicConnection();

  void unload();
  void init(bool useSubSeconds, MConAddCard addCard, MConAddPunch addPunch);

  std::vector<std::pair<std::wstring, bool>> enumerateDevices();
  std::vector<int> autoDetect();
  void status(int deviceIndex, DeviceStatus &status) const;
  int openConnection(int deviceIndex, bool listenOnly, const std::wstring &parameter, MConLogMessage logger);
  void closeConnection(int deviceIndex);
  std::wstring monitorThread(int deviceIndex);

  const std::wstring &getName() const {
    return name;
  }

  const std::wstring &getShortName() const {
    return shortName;
  }

  const std::wstring &deviceHelp() const {
    return deviceHelpStr;
  }

  bool needInputParameter(std::wstring &name, std::wstring &defValue, std::wstring &help) const;

  std::wstring getDeviceName(int deviceIndex) const;

  bool hasAnyOpenUnkownUnit() const;
};

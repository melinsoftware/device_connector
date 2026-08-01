/**
  Copyright (c) 2026 Melin Software HB
*/

#include <Windows.h>
#include "dynamic_connection.h"
#include <iostream>

using namespace std;

DynamicConnection::DynamicConnection(const wstring& library) {
  hModule = LoadLibraryEx(library.c_str(), 0, 0);
  if (hModule == nullptr) {
    auto err = GetLastError();

    if (err == ERROR_BAD_EXE_FORMAT)
      wcerr << library << " is not a valid libary," << endl;

    wcerr << "Trying to load library " << library << " return error code " << err << endl;
    exit(1);
  }
  try {
    FARPROC ff;

    ff = loadProc("getName");
    MConWString(__cdecl * getName)() = (MConWString(__cdecl*)())ff;
    name = getName();

    ff = loadProc("getShortName");
    MConWString(__cdecl * getShortName)() = (MConWString(__cdecl*)())ff;
    shortName = getShortName();

    ff = loadProc("init");
    enumerateDevicesPtr = (int(__cdecl*)(int, MConWString*, bool*))ff;
    initPtr = (void(__cdecl*)(MConDeviceProps*, MConWString *, MConWString *, MConAddCard, MConAddPunch))ff;

    ff = loadProc("enumerateDevices");
    enumerateDevicesPtr =  (int(__cdecl*)(int, MConWString *, bool* ))ff;

    ff = loadProc("status");
    statusPtr = (void(__cdecl*)(int, MConDeviceStatus*))ff;

    ff = loadProc("openConnection");
    openConnectionPtr = (int(__cdecl*)(int, bool, MConWString, MConLogMessage))ff;

    ff = loadProc("autoDetect", true);
    autoDetectPtr = (int(__cdecl *)(int, int *))ff;

    ff = loadProc("closeConnection");
    closeConnectionPtr = (void(__cdecl*)(int))ff;

    ff = loadProc("unload");
    unloadPtr = (void(__cdecl*)())ff;

    ff = loadProc("monitorThread");
    monitorThreadPtr = (MConWString(__cdecl*)(int))ff;
  }
  catch (const std::exception &ex) {
    FreeLibrary(hModule);
    hModule = 0;
    cerr << ex.what() << endl;
    exit(1);
  }
}

DynamicConnection::~DynamicConnection() {
  if (unloadPtr)
    unloadPtr();
}

void DynamicConnection::unload() {
  if (unloadPtr)
    unloadPtr();
}

FARPROC DynamicConnection::loadProc(const char* name, bool allowMissing) const {
  FARPROC pa = GetProcAddress(hModule, name);

  if (!pa && !allowMissing)
    throw std::exception(("Connector error, missing " + string(name)).c_str());
    
  return pa;
}

void DynamicConnection::init(bool useSubSeconds, MConAddCard addCard, MConAddPunch addPunch) {
  MConWString startParameterName, deviceHelpPtr;
  MConDeviceProps props;
  props.useSubSecond = useSubSeconds;
  initPtr(&props, &deviceHelpPtr, &startParameterName, addCard, addPunch);
  supportAutoDetect = props.supportAutoDetect;
  if (startParameterName != nullptr) {
    parameterName = startParameterName;
    size_t sep = parameterName.find_first_of('|');
    if (sep != wstring::npos) {
      parameterDefault = parameterName.substr(sep+1);
      parameterName = parameterName.substr(0, sep);
      sep = parameterDefault.find_first_of('|');
      if (sep != wstring::npos) {
        parameterHelp = parameterDefault.substr(sep + 1);
        parameterDefault = parameterDefault.substr(0, sep);
      }
    }
  }
  if (deviceHelpPtr)
    deviceHelpStr = deviceHelpPtr;
}

bool DynamicConnection::needInputParameter(wstring &name, wstring &parDef, wstring &help) const {
  if (parameterName.empty())
    return false;
  name = parameterName;
  help = parameterHelp;
  parDef = parameterDefault;
  return true;
}

vector<pair<wstring, bool>> DynamicConnection::enumerateDevices() {
  int maxNum = 32;
  vector<MConWString> stringArray(maxNum);
  vector<uint8_t> boolArray(maxNum);
  numDevices = enumerateDevicesPtr(maxNum, stringArray.data(), (bool*)boolArray.data());
  if (numDevices < 0 || numDevices > maxNum) {
    cerr << "Error in connector call enumerateDevices" << endl;
    exit(1);
  }
  vector<pair<wstring, bool>> out(numDevices);
  for (int i = 0; i < numDevices; i++) {
    out[i].first = stringArray[i];
    out[i].second = boolArray[i];
  }
  return out;
}


vector<int> DynamicConnection::autoDetect() {
  vector<int> intArray;
  if (autoDetectPtr) {
    int maxNum = 32;
    intArray.resize(maxNum);
    numDevices = autoDetectPtr(maxNum, intArray.data());
    if (numDevices < 0 || numDevices > maxNum) { 
      cerr << "Error in connector call autoDetect" << endl;
      exit(1);
    }
     
    intArray.resize(numDevices);
  }
  return intArray;
}


void DynamicConnection::status(int deviceIndex, DeviceStatus& status) const {
  MConDeviceStatus conStatus;
  statusPtr(deviceIndex, &conStatus);
  status.name = conStatus.name;
  status.connected = conStatus.connected;
  status.requireFunctionMapping = conStatus.requireFunctionMapping;
  status.messages.clear();
  
  if (conStatus.numStatusMessage < 0 || conStatus.numStatusMessage > (sizeof(conStatus.message) / sizeof(conStatus.message[0]))) {
    cerr << "Error in connector call status" << endl;
    exit(1);
  }
  for (int i = 0; i < conStatus.numStatusMessage; i++) {
    wstring msg = conStatus.message[i];
    bool warn = conStatus.isWarningMessage[i];
    status.messages.emplace_back(msg, warn);
  }
}

int DynamicConnection::openConnection(int deviceIndex, bool listenOnly, const wstring &parameter, MConLogMessage logger) {
  return openConnectionPtr(deviceIndex, listenOnly, parameter.c_str(), logger);
}

void DynamicConnection::closeConnection(int deviceIndex) {
  closeConnectionPtr(deviceIndex);
}

wstring DynamicConnection::monitorThread(int deviceIndex) {
  wstring res;
  auto resPtr = monitorThreadPtr(deviceIndex);
  if (resPtr)
    res = resPtr;
  return res;
}

wstring DynamicConnection::getDeviceName(int deviceIndex) const {
  DeviceStatus s;
  status(deviceIndex, s);
  return s.name;
}

bool DynamicConnection::hasAnyOpenUnkownUnit() const {
  DeviceStatus st;
  for (int i = 0; i < numDevices; i++) {
    status(i, st);
    if (st.connected && st.requireFunctionMapping)
      return true;
  }
  return false;
}
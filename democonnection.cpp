/**
  Copyright (c) 2026 Melin Software HB
*/


#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <atomic>

using namespace std::chrono_literals;

#define EXPORT_CONNECTOR

#include "connector_interface.h"

using namespace std;

EXPORT MConWString __cdecl getName() {
  static const wchar_t *name = L"Demo Connector";
  return name;
}

EXPORT MConWString __cdecl getShortName() {
  static const wchar_t *shortName = L"demo";
  return shortName;
}

static const wchar_t *finish = L"FINISH";
static const wchar_t *readout = L"READOUT";

class DemoConnection {
  mutable wstring name;
  mutable wstring statusStr;
  MConAddCard addCardPtr = nullptr;
  MConAddPunch addPunchPtr = nullptr;

  atomic_bool open[2];
  int card[2] = { 0,0 };
  
public:
  DemoConnection() = default;
  ~DemoConnection();

  void init(MConAddCard addCard, MConAddPunch addPunch) {
    addCardPtr = addCard;
    addPunchPtr = addPunch;
  }

  void status(int ix, MConDeviceStatus &status) const;

  int openConnection(int ix, int card, MConLogMessage logger);
  void closeConnection(int ix);

  void unload() {
    closeConnection(0);
    closeConnection(1);
  }

  bool connected(int ix) const {
    return open[ix].load();
  }

  MConWString monitorThread(int ix);
};

DemoConnection::~DemoConnection() {
  unload();
}

void DemoConnection::status(int ix, MConDeviceStatus &status) const {
  status.name = ix == 0 ? finish : readout;
  status.connected = connected(ix);
  status.requireFunctionMapping = false;

  if (status.connected) {
    statusStr = L"Demo: $ej aktiv$.";
  }
  else {
    wchar_t bf[128];
    swprintf_s(bf, L"Demo");
    statusStr = bf;
  }
  
  status.numStatusMessage = 1;
  status.message[0] = statusStr.c_str();
  status.isWarningMessage[0] = false;
}

int DemoConnection::openConnection(int ix, int firstCard, MConLogMessage logger) {
  card[ix] = firstCard;
  return 1;
}

void DemoConnection::closeConnection(int ix) {
  if (open[ix].load()) {
    open[ix].store(false);
    std::this_thread::sleep_for(300ms);
  }
}

MConWString DemoConnection::monitorThread(int ix) {
  open[ix].store(true);

  int first = card[ix];
  int maxSend = 5;
  int iter = 0;
  while (open[ix].load()) {
    if (++iter == (20 + 10 * ix)) {
      if (0 >= maxSend--) {
        // Exit with an "error"
        static auto *err = L"Demo ran out of card numbers!";
        open[ix].store(false);
        return err;
      }

      std::time_t time = std::time(nullptr);
      auto lt = std::localtime(&time);
      int now = lt->tm_hour * 3600 + lt->tm_min * 60 + lt->tm_sec;

      auto toMS = [](int sec) {
        // Convert to number of ms since 00:00:00
        return ((sec + 3600 * 24) % (3600 * 24)) * 1000;
      };

      if (ix == 0) {
        MConFreePunch fp;
        fp.cardNo = card[ix]++;
        fp.type = MConPunchType::PunchFinish;
        fp.unit = 1;
        fp.code = 1;
        fp.time = toMS(now);
        addPunchPtr(&fp);
      }
      else {
        MConCard crd;
        crd.cardNo = card[ix]++;
        crd.nPunch = 8;
        int start = toMS(now - 1800);
        crd.start.code = 10;
        crd.start.time = start;

        for (int i = 0; i < 8; i++) {
          double par = double(i + 1) / 9.0;
          crd.punch[i].time = toMS(int(start * (1.0 - par) + now * par));
          crd.punch[i].code = 31 + (unsigned(i + first) * 997u) % 100;
        }

        crd.finish.code = 11;
        crd.finish.time = toMS(now);

        addCardPtr(&crd);
      }

      iter = 0;
    }
    
    std::this_thread::sleep_for(200ms);
  }

  // Clean exit
  open[ix].store(false);
  return nullptr;
}

DemoConnection ipcon;

EXPORT int __cdecl enumerateDevices(int maxNumEntries, MConWString *deviceVectorDescr, bool *deviceVectorConnected) { 
  
  deviceVectorDescr[0] = finish;
  deviceVectorConnected[0] = ipcon.connected(0);

  deviceVectorDescr[1] = readout;
  deviceVectorConnected[1] = ipcon.connected(1);

  return 2; // Number of devices
}

EXPORT int __cdecl autoDetect(int maxNumEntries, int *deviceIndices) {
  deviceIndices[0] = 0;
  deviceIndices[1] = 1;
  return 2;
}

EXPORT void __cdecl status(int deviceIndex, MConDeviceStatus *status) {
  ipcon.status(deviceIndex, *status);
}

EXPORT int __cdecl openConnection(int deviceIndex, bool listenOnly, MConWString parameter, MConLogMessage logger) {
  return ipcon.openConnection(deviceIndex, _wtoi(parameter), logger);
}

EXPORT void __cdecl closeConnection(int deviceIndex) {
  ipcon.closeConnection(deviceIndex);
}

EXPORT MConWString __cdecl monitorThread(int deviceIndex) {
  return ipcon.monitorThread(deviceIndex);
}

EXPORT void __cdecl init(MConDeviceProps *deviceProps, MConWString *deviceHelp,
                         MConWString *startParameterName,
                         MConAddCard addCard, MConAddPunch addPunch) {
  deviceProps->supportAutoDetect = true;
  static auto par = L"First card number:|99999000|Specify first card number to use";
  *startParameterName = par;

  static const wchar_t *help = L"Demo punching system sending either \"random\" finish punches or readout cards.";
  *deviceHelp = help;

  ipcon.init(addCard, addPunch);
}

EXPORT void __cdecl unload() {
  ipcon.unload();
}

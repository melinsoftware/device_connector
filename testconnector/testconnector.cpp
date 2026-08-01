/** 
  Copyright (c) 2026 Melin Software HB 
*/

#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>

#include "dynamic_connection.h"

using namespace std;

void logger(MConWString message, bool isWarning);
void startThread(DynamicConnection* con, int deviceIndex);

void addCard(const MConCard* card);
void addPunch(const MConFreePunch* punch);

int wmain(int argc, wchar_t* argv[]) {
  cout << "MeOS Timing System Device Demo" << endl;
  if (argc == 1) {
      cout  << "\n\nUsage:\n testconnector [library] [deviceIndex]\n" << endl
      << "          [library]      path to a DLL file with the device communuication library" << endl
      << "          [deviceIndex]  index (starting from 0) of the devices detected by the library,\n" 
      << "                         or 'A' for auto detect and start all" << endl;
      exit(0);
  }

  wstring lib;
  if (argc > 1)
    lib = argv[1];

  wstring command;
  int deviceIndex = -2;
  if (argc > 2) {
    command = argv[2];
    deviceIndex = _wtoi(command.c_str());
    if (command == L"A")
      deviceIndex = -1;

    bool valid = deviceIndex > 0 || command == L"0" || command == L"A";
    if (!valid) {
      wcerr << "Invalid command " << command << endl;
      exit(1);
    }
  }

  wcout << "\nLoading " << lib << "..." << endl;

  DynamicConnection con(lib);

  con.init(false, addCard, addPunch);

  cout << "OK!" << endl;

  wcout << con.deviceHelp() << endl;;

  int ix = 0;
  for (auto [name, status] : con.enumerateDevices()) 
    wcout << ix++ << ": " << name << (status ? " (connected)" : " (not connected)") << endl;

  if (deviceIndex == -2)
    exit(0);

  if (deviceIndex >= 0) {
    wstring name, def, help;
    wstring param = def;
    if (con.needInputParameter(name, def, help)) {
      wcout << help << endl;
      wcout << name << " " << flush;
      wcin >> param;
    }
    if (con.openConnection(deviceIndex, false, param, logger) == 0) {
      cout << "Open connection failed" << endl;
      exit(1);
    }

    thread th(startThread, &con, deviceIndex);
    th.detach();
  }
  else {
    auto dev = con.autoDetect();
    for (int d : dev) {
      wcout << "Starting " << con.getDeviceName(d) << endl;

      if (con.openConnection(d, false, L"", logger) == 0) {
        cout << "Open connection failed" << endl;
        exit(1);
      }

      thread th(startThread, &con, d);
      th.detach();
    }
  }

  Sleep(100);

  ix = 0;
  for (auto [name, status] : con.enumerateDevices()) {
    DynamicConnection::DeviceStatus st;
    con.status(ix, st);

    wcout << ix++ << " " << name << (status ? " (connected)" : " (not connected)") << endl;
    for (auto msg : st.messages) {
      wcout << "   " << msg.first << endl;
    }
    cout << endl;
  }
  cout << "Running, press Q to quit" << endl;

  while (true) {
    char ch;
    cin >> ch;
    if (ch == 'q' || ch == 'Q')
      break;
  }

  con.unload();

  cout << "Connection closed. Bye" << endl;
}

void logger(MConWString message, bool isWarning) {
  if (isWarning)
    wcerr << message << endl;
  else
    wcout << message << endl;
}

void startThread(DynamicConnection* con, int deviceIndex) {
  auto res = con->monitorThread(deviceIndex);
  if (!res.empty())
    wcerr << res << endl;
}

string getTime(int ms) {
  int tseconds = ms / 100;
  string time = to_string(tseconds / 36000) + ":" +
    to_string((tseconds % 36000) / 600) + ":" +
    to_string((tseconds % 600) / 10) + "." +
    to_string(tseconds % 10);

  return time;
}

void addCard(const MConCard* card) {
  cout << "Readout " << card->cardNo << endl;
  if (card->start.time >= 0)
    cout << "Start: " << getTime(card->start.time) << "\n";

  for (int i = 0; i < card->nPunch; i++) {
    cout << card->punch[i].code << ": " << getTime(card->punch[i].time) << "\n";
  }

  if (card->finish.time >= 0)
    cout << "Finish: " << getTime(card->finish.time);

  cout << endl;
}

void addPunch(const MConFreePunch* punch) {
  string type = "?";
  if (punch->type == MConPunchType::PunchFinish)
    type = "Finish";
  else if (punch->type == MConPunchType::PunchStart)
    type = "Start";
  else if (punch->type == MConPunchType::PunchControl)
    type = "Control";
  else if (punch->type == MConPunchType::PunchCheck)
    type = "Check";

  string time = getTime(punch->time);
  cout << punch->cardNo << " punched " << type << "/" << punch->code << " " << time << endl;
}

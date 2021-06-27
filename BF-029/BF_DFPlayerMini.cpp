// copyright 2019 BotanicFields, Inc.
// DFRobot DFPlayerMini driver on ESP32/M5Stack
//
#include "BF_DfplayerMini.h"

DfplayerMini::DfplayerMini()
{
}

// &stream .. UART
// feedback .. 0: No, 1: Yes
void DfplayerMini::Begin(Stream &stream, bool feedback)
{
  m_stream = &stream;

  m_recv_timeout_ms = 1000;  // 1sec
  m_recv_start_ms   = 0;
  m_recv_count      = 0;
  m_expected        = 0;
  m_feedback        = 0;

  m_send_buf[0] = 0x7e;                  // constant: start byte
  m_send_buf[1] = 0xff;                  // constant: version
  m_send_buf[2] = 0x06;                  // constant: length
  m_send_buf[3] = 0;                     // command
  m_send_buf[4] = feedback? 0x01: 0x00;  // 0x01:enable feedback
  m_send_buf[5] = 0;                     // parameter MSB
  m_send_buf[6] = 0;                     // parameter LSB
  m_send_buf[7] = 0;                     // check sum MSB
  m_send_buf[8] = 0;                     // check sum LSB
  m_send_buf[9] = 0xef;                  // constant: end byte

  for (int i = 0; i < 10; ++i)
    m_recv_buf[i] = 0;

  m_dev_plug           = 0;
  m_dev_pull           = 0;
  m_finish_usb         = 0;
  m_finish_sd          = 0;
  m_device             = 0;
  m_error              = 0;
  m_status             = 0;
  m_volume             = 0;
  m_eq                 = 0;
  m_tracks_usb         = 0;
  m_tracks_sd          = 0;
  m_current_track_usb  = 0;
  m_current_track_sd   = 0;
  m_tracks             = 0;
  m_folders            = 0;

  Serial.print("[DFPlayerMini] Begin\n");
}

// call as often as possible to get response
void DfplayerMini::Update()
{
  while (m_stream->available() != 0) {
    uint8_t recv_data = m_stream->read();
    m_recv_buf[m_recv_count++] = recv_data;

    // check command code
    if (m_recv_count == 1) {
      if (recv_data == 0x7e)
        m_recv_start_ms = millis();
      else {
        m_recv_count = 0;
        Serial.printf("[DFPlayerMini] Update: skip %02x\n", recv_data);
      }
    }

    // entire command received
    if (m_recv_count == 10) {
      m_recv_count = 0;
      Receive();
    }
  }

  // check timeout
  if (m_recv_count != 0 && m_stream->available() == 0 && millis() - m_recv_start_ms > m_recv_timeout_ms) {
    m_recv_count = 0;
    Serial.print("[DFPlayerMini] Update: timeout during receiving\n");
  }
}

void DfplayerMini::Print()
{
  Serial.print("[DFPlayerMini] <online>");
  PrintDevice(m_device);

  Serial.print(" <status>");
  PrintStatus(m_status);

  Serial.printf(" <volume> %d <EQ>", m_volume);
  PrintEq(m_eq);

  if ((m_status & 0x0100) == 0) {  // not sleep
    if ((m_status & 0x0200) != 0) {  // SD online
      Serial.printf(" <SD tracks> %d <finished> %d <current> %d", m_tracks_sd,  m_finish_sd,  m_current_track_sd );
    }
    if ((m_status & 0x0100) != 0) {  // USB online
      Serial.printf(" <USB tracks> %d <finished> %d <current> %d", m_tracks_usb, m_finish_usb, m_current_track_usb);
    }
  }
  Serial.println();
}

bool DfplayerMini::Busy()
{
  if (m_feedback != 0 || m_expected != 0)
    return true;
  return false;
}

int DfplayerMini::Online()          { return m_device;            }
int DfplayerMini::Status()          { return m_status;            }
int DfplayerMini::Volume()          { return m_volume;            }
int DfplayerMini::Eq()              { return m_eq;                }
int DfplayerMini::TracksUsb()       { return m_tracks_usb;        }
int DfplayerMini::TracksSd()        { return m_tracks_sd;         }
int DfplayerMini::CurrentTrackUsb() { return m_current_track_usb; }
int DfplayerMini::CurrentTrackSd()  { return m_current_track_sd;  }
int DfplayerMini::Tracks()          { return m_tracks;            }
int DfplayerMini::Folders()         { return m_folders;           }

void DfplayerMini::Reset()                                { Send(0x0C);                }
void DfplayerMini::Sleep()                                { Send(0x0A);                }

void DfplayerMini::Play()                                 { Send(0x0D);                }
void DfplayerMini::Pause()                                { Send(0x0E);                }
void DfplayerMini::Stop()                                 { Send(0x16);                }
void DfplayerMini::Next()                                 { Send(0x01);                }
void DfplayerMini::Previous()                             { Send(0x02);                }
void DfplayerMini::Random()                               { Send(0x18);                }
void DfplayerMini::Repeat(int turn_off)                   { Send(0x19, turn_off);      }

void DfplayerMini::PlayRoot(int track)                    { Send(0x03, track);         }
void DfplayerMini::PlayFolder(int folder, int track)      { Send(0x0F, track, folder); }
void DfplayerMini::PlayFolderMp3(int track)               { Send(0x12, track);         }
void DfplayerMini::playFolder3000(int folder, int track)  { Send(0x14, track, folder); }

void DfplayerMini::RepeatRoot(int track)                  { Send(0x08, track);         }
void DfplayerMini::RepeatRootAll(int start)               { Send(0x11, start);         }
void DfplayerMini::RepeatFolder(int folder)               { Send(0x17, folder);        }

void DfplayerMini::InsertAdvert(int track)                { Send(0x13, track);         }
void DfplayerMini::Resume()                               { Send(0x15);                }

void DfplayerMini::IncreaseVolume()                       { Send(0x04);                }
void DfplayerMini::DecreaseVolume()                       { Send(0x05);                }
void DfplayerMini::SetVolume(int volume)                  { Send(0x06, volume);        }
void DfplayerMini::SetEq(int eq)                          { Send(0x07, eq);            }

void DfplayerMini::SetDac(int turn_off)                   { Send(0x1A, turn_off);      }
void DfplayerMini::SetAudio(int gain, int enable)         { Send(0x10, gain, enable);  }
void DfplayerMini::SelectDevice(int device)               { Send(0x09, device);        }

void DfplayerMini::QueryDevice()                          { Send(0x3F);                }
void DfplayerMini::QueryStatus()                          { Send(0x42);                }
void DfplayerMini::QueryVolume()                          { Send(0x43);                }
void DfplayerMini::QueryEq()                              { Send(0x44);                }

void DfplayerMini::QueryCurrentTrackOfUsb()               { Send(0x4B);                }
void DfplayerMini::QueryCurrentTrackOfSd()                { Send(0x4C);                }

void DfplayerMini::QueryTracksInUsb()                     { Send(0x47);                }
void DfplayerMini::QueryTracksInSd()                      { Send(0x48);                }
void DfplayerMini::QueryTracks(int folder)                { Send(0x4E, folder);        }
void DfplayerMini::QueryFolders()                         { Send(0x4F);                }

void DfplayerMini::Send(int command, int param_lsb, int param_msb)
{
  // make command
  m_send_buf[3] = command;
  switch (m_send_buf[3]) {
    case 0x03:
    case 0x12:
    case 0x13: m_send_buf[6] = param_lsb;  m_send_buf[5] =                  param_lsb >> 8;  break;
    case 0x14: m_send_buf[6] = param_lsb;  m_send_buf[5] = param_msb << 4 | param_lsb >> 8;  break;
    default:   m_send_buf[6] = param_lsb;  m_send_buf[5] = param_msb;                        break;
  }

  // make check sum
  int sum = 0;
  for (int i = 1; i <= 6; ++i)
    sum += m_send_buf[i];
  m_send_buf[7] = -sum >> 8;
  m_send_buf[8] = -sum;

  // send command
  m_stream->write(m_send_buf, 10);

  if (m_send_buf[3] >= 0x3f)
    m_expected = m_send_buf[3];

  if (m_send_buf[4] == 0x01)
    m_feedback = 1;

  PrintCommand("Send   ", m_send_buf);
  Serial.println();
}

int DfplayerMini::Receive()
{
  // sum check
  int sum = 0;
  for (int i = 1; i <= 6; ++i)
    sum += m_recv_buf[i];
  uint8_t sum_msb = -sum >> 8;
  uint8_t sum_lsb = -sum;
  if (   m_recv_buf[7] != sum_msb
      || m_recv_buf[8] != sum_lsb) {
    Serial.print("[DFPlayerMini] Receive: check sum error\n");
    return 1;
  }

  int param = (uint16_t)m_recv_buf[5] << 8 | m_recv_buf[6];
  switch(m_recv_buf[3]) {
    case 0x3a: m_dev_plug          = param;  break;
    case 0x3b: m_dev_pull          = param;  break;
    case 0x3c: m_finish_usb        = param;  break;
    case 0x3d: m_finish_sd         = param;  break;
    case 0x3f: m_device            = param;  break;
    case 0x40: m_error             = param;  break;
    case 0x41: m_feedback          = 0;      break;
    case 0x42: m_status            = param;  break;
    case 0x43: m_volume            = param;  break;
    case 0x44: m_eq                = param;  break;
    case 0x47: m_tracks_usb        = param;  break;
    case 0x48: m_tracks_sd         = param;  break;
    case 0x4b: m_current_track_usb = param;  break;
    case 0x4c: m_current_track_sd  = param;  break;
    case 0x4e: m_tracks            = param;  break;
    case 0x4f: m_folders           = param;  break;
    default:  break;
  }
  PrintCommand("Receive", m_recv_buf);
  PrintParam();
  Serial.println();

  if (m_recv_buf[3] >= 0x3f && m_recv_buf[3] != 0x40 && m_recv_buf[3] != 0x41) {
    if (m_recv_buf[3] != m_expected)
      Serial.print("[DFPlayerMini] Receive: command not expected\n");
    m_expected = 0;
  }
  return 0;
}

void DfplayerMini::PrintCommand(const char* title, const uint8_t buf[])
{
  Serial.printf("[DFPlayerMini] %s:", title);
  for (int i = 0; i < 10; ++i)
    Serial.printf("%02x ", buf[i]);

  String command_name;
  switch (buf[3]) {
    case 0x01: command_name = "Next";                        break;
    case 0x02: command_name = "Previous";                    break;
    case 0x03: command_name = "Play root";                   break;
    case 0x04: command_name = "Increase volume";             break;
    case 0x05: command_name = "Decrease volume";             break;
    case 0x06: command_name = "Set volume";                  break;
    case 0x07: command_name = "Set EQ";                      break;
    case 0x08: command_name = "Repeat root";                 break;
    case 0x09: command_name = "Select device";               break;
    case 0x0a: command_name = "Sleep";                       break;
    case 0x0c: command_name = "Reset";                       break;
    case 0x0d: command_name = "Play";                        break;
    case 0x0e: command_name = "Pause";                       break;
    case 0x0f: command_name = "Play folder";                 break;
    case 0x10: command_name = "Set audio";                   break;
    case 0x11: command_name = "Repeat root all";             break;
    case 0x12: command_name = "Play folder MP3";             break;
    case 0x13: command_name = "Insert ADVERT";               break;
    case 0x14: command_name = "Play folder 3000";            break;
    case 0x15: command_name = "Resume";                      break;
    case 0x16: command_name = "Stop";                        break;
    case 0x17: command_name = "Repeat folder";               break;
    case 0x18: command_name = "Random";                      break;
    case 0x19: command_name = "Repeat";                      break;
    case 0x1a: command_name = "Set DAC";                     break;
    case 0x3a: command_name = "Device plugged in";           break;
    case 0x3b: command_name = "Device pulled out";           break;
    case 0x3c: command_name = "Finished playing USB";        break;
    case 0x3d: command_name = "Finished playing SD";         break;
    case 0x3f: command_name = "Query device";                break;
    case 0x40: command_name = "Return error";                break;
    case 0x41: command_name = "Report feedback";             break;
    case 0x42: command_name = "Query status";                break;
    case 0x43: command_name = "Query volume";                break;
    case 0x44: command_name = "Query EQ";                    break;
    case 0x47: command_name = "Query tracks in USB";         break;
    case 0x48: command_name = "Query tracks in SD";          break;
    case 0x4b: command_name = "Query current track of USB";  break;
    case 0x4c: command_name = "Query current track of SD";   break;
    case 0x4e: command_name = "Query tracks";                break;
    case 0x4f: command_name = "Query folders";               break;
    default:   command_name = "N/A";                         break;
  }
  Serial.print(command_name.c_str());
}

void DfplayerMini::PrintParam()
{
  int param = (uint16_t)m_recv_buf[5] << 8 | m_recv_buf[6];
  switch(m_recv_buf[3]) {
    case 0x3a:
    case 0x3b:
    case 0x3f: PrintDevice(param);               break;
    case 0x3c:
    case 0x3d:
    case 0x43:
    case 0x47:
    case 0x48:
    case 0x4b:
    case 0x4c:
    case 0x4e:
    case 0x4f: Serial.printf(" %d", param);      break;
    case 0x40: PrintError(param);                break;
    case 0x41:                                   break;
    case 0x42: PrintStatus(param);               break;
    case 0x44: PrintEq(param);                   break;
    default:   Serial.print("Unknown command");  break;
  }
}

void DfplayerMini::PrintDevice(int param)
{
  if ((param & 0x04) != 0) Serial.print(" PC");
  if ((param & 0x02) != 0) Serial.print(" SD");
  if ((param & 0x01) != 0) Serial.print(" USB");
}

void DfplayerMini::PrintStatus(int param)
{
  String status_name;
  if ((param & 0x1000) != 0) Serial.print(" Sleep");
  if ((param & 0x0200) != 0) Serial.print(" SD");
  if ((param & 0x0100) != 0) Serial.print(" USB");
  if ((param & 0x0002) != 0) Serial.print(" Pause");
  if ((param & 0x0001) != 0) Serial.print(" Playing");
  if ((param & 0x10FF) == 0) Serial.print(" Stop");
}

void DfplayerMini::PrintEq(int param)
{
  String eq_name;
  switch (param) {
    case  0: eq_name = " Normal";      break;
    case  1: eq_name = " Pop";         break;
    case  2: eq_name = " Rock";        break;
    case  3: eq_name = " Jazz";        break;
    case  4: eq_name = " Classic";     break;
    case  5: eq_name = " Bass";        break;
    default: eq_name = " EQ unknown";  break;
  }
  Serial.print(eq_name.c_str());
}

void DfplayerMini::PrintError(int param)
{
  String error_name;
  switch (param) {
    case  1: error_name = " Busy initializing";        break;
    case  2: error_name = " In sleep mode";            break;
    case  3: error_name = " Serial receiving";         break;
    case  4: error_name = " Check sum";                break;
    case  5: error_name = " Track out of scope";       break;
    case  6: error_name = " Track not found";          break;
    case  7: error_name = " Insertion";                break;
    case  8: error_name = " SD failed";                break;
    case 10: error_name = " Entered into sleep mode";  break;
    default: error_name = " Error unknown";            break;
  }
  Serial.print(error_name.c_str());
}

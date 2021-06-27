// copyright 2021 BotanicFields, Inc.
// sense and control DFPlayer-Mini

#ifndef BF_DFPLAYER_MINI_H_INCLUDED
#define BF_DFPLAYER_MINI_H_INCLUDED

#include <Arduino.h>

class DfplayerMini {
 public:
  DfplayerMini();
  void Begin(Stream &stream, bool feedback = false);    //
  void Update();
  void Print();
  bool Busy();

  int Online();
  int Status();
  int Volume();
  int Eq();
  int TracksUsb();
  int TracksSd();
  int CurrentTrackUsb();
  int CurrentTrackSd();
  int Tracks();
  int Folders();

  void Reset();                                    // 0x0C
  void Sleep();                                    // 0x0A Set sleep

  void Play();                                     // 0x0D
  void Pause();                                    // 0x0E
  void Stop();                                     // 0x16 Stop playback
  void Next();                                     // 0x01 Play next
  void Previous();                                 // 0x02 Play previous
  void Random();                                   // 0x18 Set random playback
  void Repeat(int turn_off = 0);                   // 0x19 Set repeat playback of current track

  void PlayRoot(int track = 1);                    // 0x03 Specify playback of a track in the root directory
  void PlayFolder(int folder, int track = 1);      // 0x0F Specify playback a track in a folder
  void PlayFolderMp3(int track = 1);               // 0x12 Specify playback of folder named "MP3"
  void playFolder3000(int folder, int track = 1);  // 0x14 Specify playback a track in a folder that supports 3000 tracks

  void RepeatRoot(int track = 1);                  // 0x08 Specify single repeat playback
  void RepeatRootAll(int start = 1);               // 0x11 Set all repeat playback
  void RepeatFolder(int folder);                   // 0x17 Specify repeat playback of a folder

  void InsertAdvert(int track = 1);                // 0x13 Insert an advertisement
  void Resume();                                   // 0x15 Stop playing inserted advertisement and go back to play the music interrupted

  void IncreaseVolume();                           // 0x04
  void DecreaseVolume();                           // 0x05
  void SetVolume(int volume = 30);                 // 0x06 Specify volume
  void SetEq(int eq = 0);                          // 0x07 Specify EQ

  void SetDac(int turn_off = 0);                   // 0x1A
  void SetAudio(int gain, int enable = 1);         // 0x10 Audio amplification setting
  void SelectDevice(int device);                   // 0x09 Specify playback of a device(USB/SD)

  void QueryDevice();                              // 0x3F Query current online storage device
  void QueryStatus();                              // 0x42 Query current status
  void QueryVolume();                              // 0x43 Query current volume
  void QueryEq();                                  // 0x44 Query current EQ

  void QueryCurrentTrackOfUsb();                   // 0x4B Query current tracks in the USB flash drive
  void QueryCurrentTrackOfSd();                    // 0x4C Query current tracks in the micro SD card

  void QueryTracksInUsb();                         // 0x47 Query number of tracks in the root of USB flash drive
  void QueryTracksInSd();                          // 0x48 Query number of tracks in the root of micro SD card
  void QueryTracks(int folder);                    // 0x4E Query number of tracks in a folder
  void QueryFolders();                             // 0x4F Query number of folders in the current storage device

 private:
  Stream* m_stream;

  int m_recv_timeout_ms;    // time to reset receiving count
  int m_recv_start_ms;      // time stamp at receiving started
  int m_recv_count;         // 0: not receiving
  int m_expected;           // expected return command: 0x00 == idle
  int m_feedback;           // feedback expected

  uint8_t m_send_buf[10];   // data to send
  uint8_t m_recv_buf[10];   // data received

  int m_dev_plug;           // 0b000000SU: SD, USB plugged in
  int m_dev_pull;           // 0b000000SU: SD, USB pulled out
  int m_finish_usb;         // track number finished at USB
  int m_finish_sd;          // track number finished at SD
  int m_device;             // 0b00000PSU: PC, SD, USB online
  int m_error;              // error code
  int m_status;             // 0b000L00SU000000AY: sleep, Sd, Usb, pAause, plaY
  int m_volume;             // current volume
  int m_eq;                 // current EQ
  int m_tracks_usb;         // number of tracks of USB
  int m_tracks_sd;          // number of tracks of SD
  int m_current_track_usb;  // current track of SD
  int m_current_track_sd;   // current track of USB
  int m_tracks;             // number of tracks
  int m_folders;            // number of folders

  void Send(int command, int param_lsb = 0, int Param_msb = 0);
  int  Receive();
  void PrintCommand(const char* title, const uint8_t buf[]);
  void PrintParam();
  void PrintDevice(int param);
  void PrintStatus(int param);
  void PrintEq(int param);
  void PrintError(int param);
};

#endif // BF_DFPLAYER_MINI_H_INCLUDED

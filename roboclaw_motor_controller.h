#ifndef ROBOCLAW_MOTOR_CONTROLLER_H
#define ROBOCLAW_MOTOR_CONTROLLER_H

#include <nan.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

class RoboClaw : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);

 private:
  explicit RoboClaw(unsigned char addr = 0x80, int baud_rate = 115200);
  ~RoboClaw();

  static Nan::Persistent<v8::Function> constructor;

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Connect(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Disconnect(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Drive(const Nan::FunctionCallbackInfo<v8::Value>& info);

  unsigned int crc16(unsigned char *packet, int nBytes);
  bool rc_write(int device, unsigned char address, unsigned char command, unsigned char* data, int length);
  bool rc_read(int device, unsigned char* buf, int length);
  int convertIntegerToBytes(int value, unsigned char* buf, int offset = 0);

  int               _drive_controller_device;
  unsigned char     _drive_controller_address;
  const char*       _drive_controller_path;
  int               _baud_rate;
};

#endif

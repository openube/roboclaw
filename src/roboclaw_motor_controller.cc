#include "roboclaw_motor_controller.h"

Nan::Persistent<v8::Function> RoboClaw::constructor;

// Constructor
RoboClaw::RoboClaw(unsigned char addr, int baud_rate) {
    _roboclaw_address = addr;
    _roboclaw_baud_rate = baud_rate;
}

// Deconstructor
RoboClaw::~RoboClaw() {
}

// Init
void RoboClaw::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("RoboClaw").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "connect", Connect);
  Nan::SetPrototypeMethod(tpl, "disconnect", Disconnect);
  Nan::SetPrototypeMethod(tpl, "drive", Drive);

  constructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("RoboClaw").ToLocalChecked(), tpl->GetFunction());
}

// New
void RoboClaw::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  if (info.IsConstructCall()) {
    RoboClaw* rc = new RoboClaw();
    rc->Wrap(info.This());
    // set roboclaw board address; or use default: 0x80
    rc->_roboclaw_address = info[0]->IsUndefined() ? rc->_roboclaw_address : (unsigned char)(info[0]->NumberValue());

    info.GetReturnValue().Set(info.This());
  } else {
    // Invoked as plain function `RoboClaw(...)`, turn into construct call.
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { info[0] };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    info.GetReturnValue().Set(cons->NewInstance(argc, argv));
  }
}

// Connect
void RoboClaw::Connect(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RoboClaw* rc = ObjectWrap::Unwrap<RoboClaw>(info.Holder());
  v8::String::Utf8Value device_path(info[0]->ToString());
  rc->_roboclaw_path = (const char*)(*device_path);
  // drive control communication configuration
  printf("\nRoboClaw Virtual COM port: %s", rc->_roboclaw_path);
  printf("\nRoboClaw Bus address: %i", rc->_roboclaw_address);
  printf("\nConnecting to RoboClaw board...");
  rc->_roboclaw_device = open(rc->_roboclaw_path, O_RDWR | O_NOCTTY | O_NDELAY);
  if (rc->_roboclaw_device == -1)
  {
    printf("\nFailed to connect!");
    perror(rc->_roboclaw_path);
    info.GetReturnValue().Set(false);
    return;
  }
  printf("\nConnected!\n");
  struct termios options;
  tcgetattr(rc->_roboclaw_device, &options);
  options.c_cflag = rc->_roboclaw_baud_rate | CS8 | CLOCAL | CREAD;   //<Set baud rate
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;
  tcflush(rc->_roboclaw_device, TCIFLUSH);
  tcsetattr(rc->_roboclaw_device, TCSANOW, &options);

  info.GetReturnValue().Set(true);
}

// Disconnect
void RoboClaw::Disconnect(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RoboClaw* rc = ObjectWrap::Unwrap<RoboClaw>(info.Holder());

  close(rc->_roboclaw_device);
}

// Calculates CRC16 checksum on nBytes of data
unsigned int RoboClaw::crc16(unsigned char *packet, int nBytes) {
  int byte;
  unsigned int crc = 0;

  for (byte = 0; byte < nBytes; byte++) {
    crc = crc ^ ((unsigned int)packet[byte] << 8);
    unsigned char bit;

    for (bit = 0; bit < 8; bit++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc = crc << 1;
      }
    }
  }
  return crc;
}

// convertIntegerToBytes
int RoboClaw::convertIntegerToBytes(int value, unsigned char* buf, int offset) {
  buf[offset]   = value>>24;
  buf[offset+1] = value>>16;
  buf[offset+2] = value>>8;
  buf[offset+3] = value;

  return offset + 4;
}

// Read from roboclaw board
bool RoboClaw::rc_read(int device, unsigned char* buf, int length) {
  int r_result = read(device, buf, 1);
  if (r_result == -1) {
    return false;
  }

  return true;
}

// Write to roboclaw board
bool RoboClaw::rc_write(int device, unsigned char address, unsigned char command, unsigned char* data, int length) {
  unsigned char _data[2 + length + 2];  // addr(1) + cmd(1) + data(length) + checksum(2)
  // device address
  _data[0] = address;
  // device command
  _data[1] = command;
  // fill data bytes
  for(int i =0; i < length; i++) {
    _data[i+2] = data[i];
  }
  // generate packet checksum
  unsigned int chksum = crc16(_data, (sizeof(_data)-2));
  _data[sizeof(_data)-2] = chksum>>8;
  _data[sizeof(_data)-1] = chksum;
  // write to device
  int w_result = write(device, &_data[0], sizeof(_data));
  if (w_result == -1 || w_result != (int)(sizeof(_data))) {
    return false;
  }
  // read response
  unsigned char response[1];
  response[0] = 0;
  int r_result = read(device, response, 1);
  if (r_result == -1 || response[0] != 0xff) {
    return false;
  }

  return true;
}

/*
  37 - Drive M1 / M2 With Signed Speed

  Drive M1 and M2 in the same command using a signed speed value. The sign indicates which
  direction the motor will turn. This command is used to drive both motors by quad pulses per
  second. Different quadrature encoders will have different rates at which they generate the
  incoming pulses. The values used will differ from one encoder to another. Once a value is sent
  the motor will begin to accelerate as fast as possible until the rate defined is reached.

  Send: [Address, 37, SpeedM1(4 Bytes), SpeedM2(4 Bytes), CRC(2 bytes)]
  Receive: [0xFF]
*/
void RoboClaw::Drive(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  RoboClaw* rc = ObjectWrap::Unwrap<RoboClaw>(info.Holder());
  int SpeedM1 = info[0]->NumberValue();
  int SpeedM2 = info[1]->NumberValue();
  unsigned char data[8];
  int next = 0;
  // motor 1 speed
  next = rc->convertIntegerToBytes(SpeedM1, data, next);
  // motor 2 speed
  next = rc->convertIntegerToBytes(SpeedM2, data, next);
  // write to roboclaw board
  bool success = rc->rc_write(rc->_roboclaw_device, rc->_roboclaw_address, 37, data, sizeof(data));
  // return command success status
  info.GetReturnValue().Set(success);
}

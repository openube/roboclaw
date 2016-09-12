#include <nan.h>

#include "roboclaw_motor_controller.h"

void InitAll(v8::Local<v8::Object> exports){
  RoboClaw::Init(exports);
}

NODE_MODULE(roboclaw, InitAll)

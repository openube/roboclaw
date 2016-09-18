// initialize motor controller with optional hex address; default is 0x80
var roboclaw = require("../../index").RoboClaw(0x80);
// gamepad package for reading gamepad controller
var gamepad = require("gamepad");

var SpeedM1 = 0;
var SpeedM2 = 0;
var AxisX = 0;
var AxisY = 0;
var MAX_SPEED = 5000;
var MIN_SPEED_THRESHOLD = 400;

/*
  The following script will connect to a RoboClaw 2x15A Motor Controller,
  drive two motors, forward and backward, before halting and disconnecting.

  - device path:
      Mac: '/dev/tty.usbmodem1411'
      Ubuntu: '/dev/ttyAMA0'
      Raspi: '/dev/ttyACM0'

  - baud rate:
      defaults to 115200
*/

// connect to motor controller
roboclaw.connect('/dev/tty.usbmodem1411', 115200);

// Initialize the library
gamepad.init();

// Create a game loop and poll for events
setInterval(gamepad.processEvents, 100);
// Scan for new gamepads as a slower rate
setInterval(gamepad.detectDevices, 5000);
// Listen for move events on all gamepads
gamepad.on("move", function (id, axis, value) {
  if (axis === 0) AxisX = value;
  if (axis === 1) AxisY = -value;

  // calculate motor speeds based on gamepad axes positions
  calculateMotorSpeeds();

  // apply motor speeds to RoboClaw with drive command
  setMotorSpeeds();
});

// close roboclaw and application, when any button pushed
gamepad.on("down", function (id, num) {
  // IO stream must be closed before terminating the application
  roboclaw.disconnect();

  // termiate application
  setTimeout(function() {
    process.exit();
  }, 250);
});

var calculateMotorSpeeds = function() {
  SpeedM1 = ((AxisY + AxisX) / 2) * MAX_SPEED;
  SpeedM2 = ((AxisY + (-AxisX)) / 2) * MAX_SPEED;

  if (Math.abs(SpeedM1) < MIN_SPEED_THRESHOLD) SpeedM1 = 0;
  if (Math.abs(SpeedM2) < MIN_SPEED_THRESHOLD) SpeedM2 = 0;
};

var setMotorSpeeds = function() {
  roboclaw.drive(SpeedM1, SpeedM2);
};

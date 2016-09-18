// initialize motor controller with optional hex address; default is 0x80
var roboclaw = require("../../index").RoboClaw(0x80);
// gamepad package for reading gamepad controller
var gamepad = require("gamepad");

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
  if (axis === 1) {
    var speed = 5000 * -value;

    speed = (Math.abs(speed) < 400) ? 0 : speed;
    // dual motor drive command with speed values; requires motor encoders
    roboclaw.drive(speed, speed);
  }
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

// initialize motor controller with optional hex address; default is 0x80
var roboclaw = require("../../index").RoboClaw(0x80);

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

// dual motor drive command with speed values; requires motor encoders
roboclaw.drive(1500, 1500);

setTimeout(function() {

  // reverse; dependent on the connections between encoders and motor controller
  roboclaw.drive(-1500, -1500);

  setTimeout(function() {

    // stops motor movement; motors act as a brake
    roboclaw.drive(0, 0);

    setTimeout(function() {

      // IO stream must be closed before terminating the application
      roboclaw.disconnect();

    }, 1000);
  }, 5000);
}, 5000);

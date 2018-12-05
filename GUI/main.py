import serial, argparse
from gui.controller import Controller

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  # Arduino connecton configuration
  parser.add_argument('port', type=str, default='', help='Where to find the Arduino.')
  parser.add_argument('--baudrate', type=int, default=9600, help='Baudrate for the serial connection.')
  # Maze configuration
  parser.add_argument('--rows', type=int, default=2, help='Number of rows in the maze.')
  parser.add_argument('--cols', type=int, default=3, help='Number of cols in the maze.')
  args = parser.parse_args()

  try:
    # Setup the serial connection to the Arduino
    with serial.Serial(args.port, args.baudrate) as ser:
      # Setup the GUI controller
      controller = Controller(args.rows, args.cols)
      while True:
        # Note: readline blocks.. If you do not terminate your message
        # with a newline, this will block forever...
        msg = ser.readline()
        print 'Received message: %s' % msg.strip()
        controller.handle_msg(msg)
  except serial.serialutil.SerialException as e:
    print 'Could not connect to the Arduino.'
    print e
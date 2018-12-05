from model import Model
from model import COLUMNS
from view import View
import os, re
# Regular expression to check for legal message formatting
LEGAL_MSG = re.compile("^([0-9]+,[0-9]+)(,[a-z]+=[a-z]+,?){0,}")

class Controller():
  def __init__(self, rows, cols):
    self.rows, self.cols = rows, cols
    self._model = Model(rows, cols)
    self._view = View(rows, cols, open_browser=True)

  def _update_model(self, row, col, attrs):
    return self._model.update_cell(row, col, **attrs)

  def _cell_sprite(self, cell_state):
    if not cell_state['explored']:
      return './sprites/unexplored.jpg'
    return os.path.join('./sprites', '_'.join(str(cell_state[col]) for col in COLUMNS) + '.jpg')

  def _update_view(self, row, col, cell_state):
    sprite = self._cell_sprite(cell_state)
    self._view.update_cell(row, col, sprite)

  def handle_msg(self, msg):
    # Drop white space
    msg = msg.strip()
    # For simplicity, all messages are handled in lower-case
    msg = msg.lower()
    # reset is a special message that sets the GUI back to its intial state
    if 'reset' in msg:
      self.reset()
    elif LEGAL_MSG.match(msg):
      tokens = msg.split(',')
      # Drop empty tokens resulting from extra commas
      tokens = filter(lambda t: t, tokens)
      # Structure the tokens to be passed to the model
      row, col = int(tokens[0]), int(tokens[1])
      attrs = {'iamhere' : True}
      # iamhere=true is automatically added to the message to save you from
      # sending this with your messages; if you are sending information about
      # some cell other than where you are located currently, you MUST send
      # iamhere=false
      for t in tokens[2:]:
        attr, val = t.split('=')
        # Convert to a boolean if that's the intention indicated by val
        val = True if val == 'true' else False if val == 'false' else val
        attrs[attr] = val

      # Ensure legal coordinates
      if row < self.rows and col < self.cols and row >= 0 and col >= 0:
        # Update the model and get the new state of the cell
        self._update_model(row, col, attrs)
        # Update the view with the full maze state; this is necessary now that
        # we are tracking dynamic state (robot position)
        for r in xrange(self.rows):
          for c in xrange(self.cols):
            self._update_view(r, c, self._model.get_cell_state(r, c))
        # Render the updated view
        self._view.render()
      else:
        print 'Message ignored: Illegal maze coordinates: (%d, %d).' % (row, col)
    else:
      print 'Message ignored: Does not match the API requirements.'

  def reset(self):
    self._model = Model(self.rows, self.cols)
    self._view = View(self.rows, self.cols)

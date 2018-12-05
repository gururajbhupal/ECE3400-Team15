import pandas as pd
import itertools
import warnings
from collections import defaultdict
# Ignore warnings about set_value being deprecated.
warnings.filterwarnings("ignore")

COLUMNS = ['iamhere', 'west', 'north', 'east', 'south', 'tshape', 'tcolor', 'robot', 'explored']
# Note that the default value is always last
ALLOWED_VALUES = defaultdict(lambda: [True, False])
ALLOWED_VALUES['tshape'] = ['square', 'triangle', 'diamond', 'none']
ALLOWED_VALUES['tcolor'] = ['red', 'blue', 'none']

def empty_maze(rows, cols):
  # Generate an empty maze
  num_cells = rows * cols
  empty_maze = {}
  for col in COLUMNS:
    empty_maze[col] = [ALLOWED_VALUES[col][-1]] * num_cells
  # Cells are referenced by an index of the form (row, col)
  return pd.DataFrame(empty_maze, index=itertools.product(xrange(rows), xrange(cols)), columns=COLUMNS)

class Model():
  def __init__(self, rows, cols):
    self._maze = empty_maze(rows, cols)

  def _set_cell_attr(self, row, col, attr, val):
    # Only set an attribute if there is a column for it
    if attr in self._maze:
      # Before setting the robot's current location, forget the last set location
      if attr == 'iamhere' and val:
        self._maze['iamhere'] = False
      self._maze.set_value((row, col), attr, val)

  def get_cell_state(self, row, col):
    return self._maze.loc[[(row, col)]].to_dict('records')[0]

  def update_cell(self, row, col, **kwargs):
    # Updating a cell's state automatically sets it as 'explored'
    self._set_cell_attr(row, col, 'explored', True)
    for attr, val in kwargs.items():
      self._set_cell_attr(row, col, attr, val)
    return self.get_cell_state(row, col)

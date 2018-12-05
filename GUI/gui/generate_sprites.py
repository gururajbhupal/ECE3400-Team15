import cv2
import os
import itertools
import numpy as np

def create_empty_sprite(height, width):
  return np.zeros((height, width, 3), np.uint8)

def draw_walls(sprite, west, north, east, south, wall_color=(0, 255, 255)):
  height, width, _ = sprite.shape
  if west:
    sprite[:, :int(width/10)] = wall_color
  if north:
    sprite[:int(height/10), :] = wall_color
  if east:
    sprite[:, width-int(width/10):] = wall_color
  if south:
    sprite[height-int(height/10):, :] = wall_color
  return sprite

def draw_treasure(sprite, tshape, tcolor):
  height, width, _ = sprite.shape
  colors = {
    'red': (0, 0, 255),
    'blue': (255, 0, 0),
    'none': (255, 255, 255)
  }
  if tshape is not 'none' or tcolor is not 'none':
    cv2.putText(sprite, tshape.upper()[0] if tshape is not 'none' else '?', (width/10, height - height/7), cv2.FONT_HERSHEY_SIMPLEX, 1, colors[tcolor], 2)
  return sprite

def draw_robot(sprite, iamhere, robot_color=(255, 255, 255)):
  height, width, _ = sprite.shape
  if iamhere:
    cv2.circle(sprite, (width/2, height/2), min(height, width)/8, robot_color, -1)
  return sprite

def name_sprite(combo):
  return '_'.join(map(str, combo))

if __name__ == '__main__':
  import argparse
  parser = argparse.ArgumentParser()
  parser.add_argument('--out_dir', type=str, default='../sprites', help='Where to put the sprites.')
  # Sprite configuration
  parser.add_argument('--height', type=int, default=100)
  parser.add_argument('--width', type=int, default=100)
  args = parser.parse_args()

  # Generate all possible combinations of columns based on allowed values
  from model import COLUMNS
  from model import ALLOWED_VALUES
  state_combinations = itertools.product(*[ALLOWED_VALUES[col] for col in COLUMNS])

  for state_combo in state_combinations:
    sprite = create_empty_sprite(args.height, args.width)

    sprite = draw_walls(sprite,
      west=state_combo[COLUMNS.index('west')],
      north=state_combo[COLUMNS.index('north')],
      east=state_combo[COLUMNS.index('east')],
      south=state_combo[COLUMNS.index('south')])

    # TODO: Implement drawing treasures
    sprite = draw_treasure(sprite,
      tshape=state_combo[COLUMNS.index('tshape')],
      tcolor=state_combo[COLUMNS.index('tcolor')])

    # TODO: Implement drawing robots
    sprite = draw_robot(sprite,
      iamhere=state_combo[COLUMNS.index('iamhere')])

    sprite_name = name_sprite(state_combo)
    cv2.imwrite(os.path.join(args.out_dir, '%s.jpg' % sprite_name), sprite)

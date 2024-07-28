#!/bin/bash

echo "Choose game  type (sp or mp):"
read mod_type

if [ "$mod_type" == "mp" ]; then
  mkdir -p ~/vortexmod/mp
  mv mp ~/vortexmod/mp
  echo "Moved 'mp' to '~/vortexmod/mp'"
elif [ "$mod_type" == "multiplayer" ]; then
  mkdir -p ~/vortexmod/mp
  mv mp ~/vortexmod/mp
  echo "Moved 'mp' to '~/vortexmod/mp'"
elif [ "$mod_type" == "sp" ]; then
  mkdir -p ~/vortexmod/sp
  mv sp ~/vortexmod/sp
  echo "Moved 'sp' to '~/vortexmod/sp'"
elif [ "$mod_type" == "singleplayer" ]; then
  mkdir -p ~/vortexmod/sp
  mv sp ~/vortexmod/sp
  echo "Moved 'sp' to '~/vortexmod/sp'"
else
  echo "Invalid option. Please choose 'sp' or 'mp'."
fi

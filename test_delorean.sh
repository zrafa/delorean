#!/bin/bash

export DISPLAY=:0
xhost +
rm -rf real
rm -rf mp
mkdir -p real/.versions+place
mkdir mp
mount.dlorean real mp -o default_permissions -f -d

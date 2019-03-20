#! /bin/zsh

# This script should only be used while running rv_origin.py alone

echo "Killing gem5 processes..."
out='res'
killall python3 gem5.opt 2> $out
# if process found, remove output
if test ! -s ./res
then
    echo "Running gem5 killed, removing directories..."
    python3 undo.py
else
    echo "No running gem5 detected"
fi

rm $out

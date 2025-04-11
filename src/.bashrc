

# Run robot
if [ -z "$DISPLAY" ] && [ "$(tty)" == "/dev/tty1" ]; then
    ./robot
fi

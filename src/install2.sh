# sudo vi /etc/systemd/system/robot.service
# sudo systemctl enable robot.service
# sudo systemctl start robot.service
# sudo systemctl status robot.service

[Unit]
Description=Robot

[Service]
ExecStart=/bin/bash -c 'cd /home/tom/Documents/deskman/src/build && ./robot'
WorkingDirectory=/home/tom/Documents/deskman/src/build
User=tom
Environment=DISPLAY=:0

[Install]
WantedBy=multi-user.target

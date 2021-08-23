[Unit]
Description=Bootp Daemon
ConditionPathExists=/usr/sbin/bootpd
Before=vnmr.service
Wants=network-online.target
After=network-online.target

[Service]
Type=forking
User=root
ExecStart=/usr/sbin/bootpd -s

[Install]
WantedBy=multi-user.target

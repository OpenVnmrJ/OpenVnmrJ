[Unit]
Description=Acquisition Communication Daemon
ConditionPathExists=/vnmr/acqbin/acqpresent
Wants=network-online.target
After=network-online.target

[Service]
Type=simple
User=root
ExecStart=/vnmr/acqbin/ovjProcs start
ExecStop=/vnmr/acqbin/ovjProcs stop

[Install]
WantedBy=multi-user.target

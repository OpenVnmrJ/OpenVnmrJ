#!/bin/bash

cd
# Tell vnmrj it was started from OpenVnmrJ.app, so it replaces this process
# with Java instead of backgrounding it. The app's own process then becomes
# OpenVnmrJ: one Dock icon, and clicking it activates the running session.
export OVJ_APP_LAUNCH=1
exec /bin/bash -l -c "exec /vnmr/bin/vnmrj"

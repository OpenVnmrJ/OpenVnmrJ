#!/bin/bash

#########################
# switchVJ.sh           #
# John Ryan             #
# Agilent Technologies  #
# john_ryan@agilent.com #
#########################

# Copyright 2016 John Ryan
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Check for root user
if [[ $(id --user) -ne 0 ]]
then
    echo -e "\nERROR: To run this script you will need to be the system's root user.\n" >&2
    exit 1
fi

# Find current target of /vnmr
if [[ -L /vnmr ]]
then
    CURRENT=$(readlink /vnmr)
fi

# Find the highest version of VnmrJ and OpenVnmrJ currently installed
VNMRJ=$(find /home -maxdepth 1 -name 'vnmrj*' | sort | tail --lines 1)
OPENVNMRJ=$(find /home -maxdepth 1 -name 'openvnmrj*' | sort | tail --lines 1)

# The target will default to VnmrJ unless it doesn't exist.
if [[ -d ${VNMRJ} ]]
then
    TARGET=${VNMRJ}
else
    TARGET=${OPENVNMRJ}
fi

# If the link currently points to VnmrJ then switch the target to OpenVnmrJ if it exists
if [[ "x${CURRENT}" == "x${VNMRJ}" ]] && [[ -d ${OPENVNMRJ} ]]
then
    TARGET=${OPENVNMRJ}
fi

echo -e "\n       Current link: ${CURRENT}"
echo "    VnmrJ directory: ${VNMRJ}"
echo "OpenVnmrJ directory: ${OPENVNMRJ}"
echo -e "           New link: ${TARGET}\n"

# Check that the target exists
if [[ ! -d ${TARGET} ]]
then
    echo -e "ERROR: No valid target found.\n" >&2
    exit 1
fi

# Check whether any action is required
if [[ "x${CURRENT}" != "x${TARGET}" ]]
then
    if [[ ! -z ${CURRENT} ]]
    then
        # Stop the procs if required
        PID=$(pgrep Expproc)
        if [[ ! -z ${PID} ]]
        then
            echo "Stopping Acquisition communications."
            PROCS=1
            /vnmr/bin/execkillacqproc >/dev/null 2>&1
        fi

        # Stop the locator database daemon
        if [[ -f /etc/init.d/pgsql ]]
        then
            echo "Stopping locator database daemon."
            /etc/init.d/pgsql stop >/dev/null 2>&1
        fi
    fi

    # Remove the current link
    rm --force /vnmr

    # Create the new link
    ln --symbolic ${TARGET} /vnmr

    # Restart the locator database daemon
    if [[ -f /etc/init.d/pgsql ]]
    then
        echo "Starting locator database daemon."
        /etc/init.d/pgsql start >/dev/null 2>&1
    fi

    # Restart the procs if they were running before
    if [[ ${PROCS} -eq 1 ]]
    then
        echo "Starting Acquisition communications."
        /vnmr/bin/execkillacqproc >/dev/null 2>&1
    fi
fi

# Done
echo "Done."

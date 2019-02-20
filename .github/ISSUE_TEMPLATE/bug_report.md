---
name: Bug report
about: Create a report to help us improve
title: ''
labels: ''
assignees: ''

---

**Describe the bug**
A clear and concise description of what the bug is.

**To Reproduce**
Steps to reproduce the behavior:
1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. See error

**Expected behavior**
A clear and concise description of what you expected to happen.

**Version of OpenVnmrJ you are using.**
- Where did it come from
- How was it installed, additional relevant packages

**Messages and Logs**
If applicable, attach logs to help explain your problem.  Log files can be found
- copy/paste the error message text here,
- in ~/vnmrsys/VnmrjMsgLog,
- by enabling recording while performing the action that produces an error (from the OpenVnmrJ command line):
```
jFunc(55,'/vnmr/tmp/plbug')
debug('c3')
```
and then do whatever you did to generate the error. After the error appears, enter
`debug('C')` to end the recording, and then attach the /vnmr/tmp/plbug file.


**Environment (please complete the following information):**
 - OS: [e.g. CentOS 5, Ubuntu 14.04, etc..]

**Additional context**
Add any other context about the problem here.

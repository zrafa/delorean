delorean
========

Delorean - DELete Often, the system REtAiNs 

Copyright (C) 2008-2009 Rafael Ignacio Zurita rafaelignacio.zurita@gmail.com 

Delorean is free software, you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 as published by the Free Software Foundation.

Read the LICENSE file for details.


Abstract
========

Delorean adds versioning support to any already mounted file system 
under a different directory, so user can always to get a previous
version of any file.

It is a versioning filesystem written as part of my thesis.

Some interesting features are :

- travel tool: it is a command line tool to run applications at different
  past point in time.
- processes with ancestral "traveling" processes inherits the environment,
  so these also "see" the filesystem at the past point in time.
- policy to set the auto delete feature, to remove old copies and recover
  free space.

Check the paper and code to know more details.

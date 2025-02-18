README for NvTriStrip, library version
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To use: 
-#include "NvTriStrip.h"
-put nvtristrip.lib in your library path (the pragma in nvtristrip.h will automatically look for the library).

Check out NvTriStrip.h for the interface.

See the StripTest source code (in function LoadXFileStripped) for an example of using the library.

Features:
-generates strips from arbitrary geometry.
-flexibly optimizes for post TnL vertex caches (16 on GeForce1/2, 24 on GeForce3).
-can stitch together strips using degenerate triangles, or not.
-can output lists instead of strips.
-can optionally throw excessively small strips into a list instead.
-can remap indices to improve spatial locality in your vertex buffers.

On cache sizes:
Note that it's better to UNDERESTIMATE the cache size instead of OVERESTIMATING.
So, if you're targetting GeForce1, 2, and 3, be conservative and use the GeForce1_2 cache 
size, NOT the GeForce3 cache size.
This will make sure you don't "blow" the cache of the GeForce1 and 2.
Also note that the cache size you specify is the "actual" cache size, not the "effective"
cache size you may of heard about.  This is 16 for GeForce1 and 2, and 24 for GeForce3.

Credit goes to Curtis Beeson and Joe Demers for the basis for this stripifier and to Jason Regier and 
Jon Stone at Blizzard for providing a much cleaner version of CreateStrips().

Questions/comments email cem@nvidia.com


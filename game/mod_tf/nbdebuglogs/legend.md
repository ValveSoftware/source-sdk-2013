# Nextbot debugging Legend

## **nbdebuglogs:** 
Folder to keep nextbot debug logs.

## **valve-maps/xx_xxxx:**
 `Valve-maps` folder then `xx` is map type then `_xxxx` map name, place in a folder that debugging took place in. i.e *pl_upward*, and *cp_dustbowl*


## **case-xxxx:** 
Folder for test cases i.e `case-all-engi-24` (case: Test with all bots as engineer with 24 slots for bots.)


## **`nb.xxxx.debug.txt:`** 
Naming convention of the log file with `xxxx` being on of the following **Behavior, Look_At, or Vision.** 

**Note: Locomotion and Path are only good for visual debugging as not much console debugging is given**


## Important Note for `nb.lookat.debug.txt` and `nb.vision.debug.txt`

### use only 2 bots max as `nb_debug look_at` and `nb_debug vision` spams console as it runs for each bot!! and place in a special folder in the map folder called `bot-visuals-debug`

## `nb_debug` Usage

- Valid types: `behavior`, `look_at`, `vision`, `path`, `locomotion`.
- Enable a type:
  - `nb_debug behavior`
  - `nb_debug look_at`
  - `nb_debug vision`
  - `nb_debug path`
  - `nb_debug locomotion`
- Disable debugging: `nb_debug` (no argument).
- Invalid forms (e.g., `nb_debug 1`) will print: `Invalid debug type 'x'`.
- Tips: use `developer 1`, `con_logfile <path>` to capture output; for visual-heavy modes (`look_at`, `vision`) limit bot count to minimize spam.

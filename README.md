# 4v4 PASS Time mod

Hello! Welcome to the source code for the (soon to be) standalone 4v4 PASS Time Team Fortress 2 mod.

Legal disclaimer: Team Fortress 2, Half Life 2, and all associated IPs are owned by Valve Corporation. We are not affiliated with them in any way, shape or form. They have graciously allowed us to use the new Source SDK to distribute our mod non-commercially. [Here's their page on it.](https://partner.steamgames.com/doc/sdk/uploading/distributing_source_engine)

The original README.md distributed with Source SDK Base 2013 can be located at [SDK_README.md](./SDK_README.md). The original LICENSE can be found at [SOURCE1SDK_LICENSE](./SOURCE1SDK_LICENSE).


## Building

### Windows
Prerequisites:

Requirements:
 - Steam
 - Source SDK 2013 Multiplayer installed via Steam
 - Team Fortress 2
 - Visual Studio 2022 with the following workload and components:
   - Desktop development with C++:
     - MSVC v143 - VS 2022 C++ x64/x86 build tools (Latest)
     - Windows 11 SDK (10.0.22621.0) or Windows 10 SDK (10.0.19041.1)
 - Python 3.13 or later

**If you just installed these components restart your computer!**

**If Python is installed make sure you have a PATH variable!**

### 2. Clone the repository
```bash
git clone https://github.com/p4sstime/p4ss/
```
**Make sure you clone the project to a location where there are no spaces in the path!**

Example:

```
C:/My Clones/p4ss   -- Error!
```
```
C:/My_Clones/p4ss   -- Ok!
```

### 3. Generate the project files
Generate the project files by running the `createallprojects.bat` file:
```bash
p4ss/src/createallprojects.bat
```

### 4. Compile the project
Open the `everything.sln` file:
```bash
p4ss/src/everything.sln
```
Switch from Debug to **Release**.

Press the `Build > Build Solution` button or press Ctrl + Shift + B.

### Linux
Prerequisites:
- Steam
- podman (latest is fine, restart after installing)
- Team Fortress 2
- Source SDK Base 2013 Multiplayer


Clone this repository, then run:
```bash
./src/buildallprojects debug clean
# note: `clean` will re-generate the project files. use it whenever you make a change in src/vpc_scripts, to ensure your project files are created correctly.
```
This will build all the code and bundle it into a nice little executable.

Alternatively, to build the project for release, run:
```bash
./src/buildallprojects release clean
```

Then:
```bash
./game/p4ss_linux64
```
and you're up and running!

## How can I help?

Thanks for your interest!

We are currently in need of people to fill these positions:
- Concept artists (for our HUD and UI)
- VGUI HUD developers (if you've made a tf2 hud in the past, you're a perfect fit)

If you think you'd fit in any of the above categories, or want to contribute anyway, join [our discord](https://discord.passtime.tf/), tab into #passtime-mod-discussion and say you read the README and are interested in helping! Let us know what you can do, and we'll reach out.
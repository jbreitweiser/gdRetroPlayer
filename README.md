# RetroPlayer
RetroPlayer is an implementation of the libRetro interface within godot using C++ as the shim.  The aim of this project is to make a libretro library that can easily be used inside of godot so the frontend can be developed in godot's flexible framework.  This is not a full frontend, just a player for godot.

## Status
* Have leveraged the lrcpp library to implement basic interfacing to a core.
* video is currently being writen to an Image object.
* Audio is being played through an AudioStreamPlayer.
* input is captured in GDScript and forwarded to the RetroPlayed via forward_input.  
* It is playable but is slow and sound is out of sync (better now)
* Have only tested against FBNeo and Mame 2003+ so far.


## Usage

Refer to the demo application in the demo folder for how it works with godot.  You simpley pass the render surface (TextureRect or MashInstance) and Audioplayer (AudioStreamPlayer 2D or 3D) to the RetroPlayer before you start calling run.  Input has to be handled manually


## Build

To build the source I am including what I did to build it on windows.  Your process may be different on a different platform.  My tools have beed MSYS2 + VSCode.

* Install the Msys2 environment.  I had this installed to build retroarch previously.  Link -> https://docs.libretro.com/development/retroarch/compilation/windows/
* I added the scons package to the msys2 environment.
* download the github repo There are 2 submodules to this project included.
  * godot-cpp - This is the C++ GDExtension required to use C++ with godot.  I usedtheir template to start.
  * lrcpp - This library wrappers the libretro.h interface so it can be used with C++.  I stubs out all the functions.  I used their demo application and converted it to godot to be used as a player inside godot.  There are many features that are not yet implemented.
* libcpuid - This is a google library used to detect CPU features.  It is implemented as an external dll only now but I should be able to add the linux and macos builds as well.  We only need the library file at the end so it can be built seperately.
* To build the libcpuid library per their instructions.  https://github.com/anrieff/libcpuid?tab=readme-ov-file
* To build retro player run scons from the main folder and godot-cpp, lrcpp, and retroplayer will build together.


## Next Steps

* Done
  * Implement Screen Rotation
  * Render Video to a TextureRect2D
  * Render Audio to an AudioPlayer2D
  * Capture Input from a joypad / controller
  * integrate CPU Feature detection
  * Implement interface for different nodes to display the video. (TextureRect2D and MeshInstance3D)
  * Implement interface for different nodes to display the audio. (AudioStreamPlayer2D and 3D)
  * Tighter Audio and Video sync by rendering to nodes in the C++ code instead of returning data to GDScript.

* Planned
  * design a keyboard and mouse mapping to be able to get rid of the forward_event function.
  * design a way to pass options to the RetroPlayer without the need of configuration files / load the files manually and pass them via func call
  * Test more cores.
  * Implement more libRetro interface features

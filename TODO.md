#TODO List (in order of top priority)

- Refactor GameWorld structures loading
- Make sure that the compiler does not do unnecessary object copies since this wastes performance
  Yes C++, I am talking to you! Why oh why are you calling the destructor when I am using std::move? Damn you, copy elision.

#DONE List

- Fix scene view window overlapping with bottom status bar
- Clear the framebuffer when the level is closed (the framebuffer still displays the previous level) (proposition: just call Clear() somewhere)
- Changing the resolution is not supported (proposition: add a MaxResolution variable, that will be the maximum resolution the user can change to while using the program, it would be the size allocated for the framebuffers in sbHeap)


- Add ray test functionality for room/object selection using a mouse click

- Add new PSO for drawing helpers such as colored lights icons in space
- Add new vertex formats
- Add highlight support (when the user selects an object, it should be highlighted in some way)

- Fix hardcoded perspective matrix aspect ratio



Alan Moczulski
September 2023

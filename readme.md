# Untitled Dungeon Adventure Game

![Demo](https://i.imgur.com/RVObbzL.gif)

## Installation (Windows)

...


## Actually Playing the Game

### Controls

You can use the keyboard or a gamepad (I tested with an XBOX Controller). 
Here is the default button layout: 

| Action        | Keyboard Keys               | Xbox Controller Button           |
|---------------|-----------------------------|----------------------------------|
| Move Up       | ↑ or W                       | Left Stick Up or D-Pad Up        |
| Move Down     | ↓ or S                       | Left Stick Down or D-Pad Down    |
| Move Left     | ← or A                       | Left Stick Left or D-Pad Left    |
| Move Right    | → or D                       | Left Stick Right or D-Pad Right  |
| Action 1      | O                           | A					             |
| Action 2      | P                           | Y								 |
| Action 3      | K                           | X								 |
| Action 4      | L                           | B								 |
| Confirm       | Enter                       | Start							 |
| Cancel        | Backspace                   | Back							 |
| Debug Mode    | F1                          | —                                |

Action 1 is used for confirmation (selecting an item or advancing a text box).
Action 2 uses your currently equipped weapon.

You can adjust the Keyboard keys in settings.json (TODO: not implemented)


## Notes on the Software Design

### The Game object
Game is the overarching data structure that is responsible for the game loop as well as high-level operations above the individual game scenes and objects.
It manages scenes, events, assets, and the main render texture that the scenes can draw to. The game loop consists of events, update, and draw. In the events method, once per frame all the events are popped from the event queue and passed to the scenes. Although I will probably scrap this in favor of an event system that uses event listeners and callbacks. The update method is for the game logic, and most high-level game objects have an update method as well, that gets called here. The deltaTime between frames is passed to all of them to ensure framerate-independency. In draw, the scenes draw to the main render texture.

### Scenes
A scene can encapsulate/abstract any part of the game. Currently, I'm using the following scenes:

#### Preload
This scene runs before all others, and is responsible for loading all assets (external data) that are being used later. Currently I am loading everything up front, because it's not much. I might have to think about loading/streaming additional data later, but the game isn't nearly as big for this to be necessary.
Once the loading is done, this scene stops and calls the TitleSceen.
TODO: I'm planning to make a loading bar animation that runs parallel to the data streaming, but I'll probably need parallelism (std::thread) for this...

### TitleScreen
Nothing to see here, really. Just press any key to advance.
Known bugs: The button you press here gets also registered in the next scene (InGame), but I'm not sure why.

### InGame
This is the most important scene (shocking, I know). It handles updating and displaying of the (visible and invisible) game objects. Most of these are instances of the Sprite class, more on that later.


# Scrap Mechanic Lua Watchdog

This mod adds a Lua callback watchdog to the game's Lua API.  
The watchdog keeps track of the execution times of Lua callbacks in the game script environment and,  
if the time exceeds a compile-time constant (default: 5 seconds), throws a Lua error at the current execution point.

This is useful in cases where you have to track down some nasty game-freezing bug, e.g. an infinite loop, as these  
normally freeze the game, require a restart and are not easily findable due to lack of debugging features in the game.

![timeout error](https://github.com/user-attachments/assets/02a23452-dc49-4029-aa12-fb0855deff8e)

## Notes:
- The terrain script environment is not monitored, only the game script environment.
- This mod is only intended for **debugging**, as it causes a significant drop in game performance due to the extra timing checks being silently injected everywhere.

## How to use
Just inject the mod with any DLL injector of your choice.  
Do note that the mod has to be injected **before a world is loaded**, else it will not work.

## How to compile
You can compile the mod yourself using the provided Visual Studio solution files.


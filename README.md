# WhatRemainsOfEdithFinchInternal
### A free cam for what remains of edith finch game. Does not work in flashbacks

Injection time does not matter except for flashbacks. Dont inject then.  
Use any injector of your choice (the game does not contain an AC).  
I used ProcessHacker :)  

No NOPing to any opcodes were done due to the existance of a fairly strange XMM move which does not only serve the position vector.  
Simple NOPing would make the player freeze eventhough the position vector changes.  
The trainer does contain ```idBadReadPtr``` checks but sometimes (espectially in flashbacks) only one coordinate of the vec3 will become
invalid so there will be a bad read.  
In addition to that the game loads chunks which the flying does not trigger. If you want to make an other part load you have to disable the fly hack 
and walk the "normal path the game wants you to go" until it has fully loaded.  

Use Numpad * to toggle the visibility of the menu.

### Enjoy exploring the world of What Remains Of Edith Finch!

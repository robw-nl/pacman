**Introduction**
I started experimenting with a simple pacman maze. Dots for pellets, 
where one triangle (PacMan) was followed by three circles (ghosts) 
coming from different angles. This simple experiment evolved into  
a full-time, two-month C project during which I used Gemini 3.1 Pro 
as a senior co-developer to fine-tune the code, making it error-free 
and improving the ghost AI for full authenticity.

I'm still improving the game, but it is fully stable, free of memory 
leaks,It features an auto demo mode (which activates if the user 
doesn't press Enter to start). It has all the original sounds, sprites 
and more importantly, a true implementation of the original ghost AI. 
And it is a challenging game right from the start. If you manage to 
reach level 256, you're even treated to the original integer memory 
bug game crash artifacts, after which you can continue. Be prepared 
for a week of non-stop play to achieve this! 

**Installation instructions:**
The pacman_install_linux.sh script installs the pacman binary and all 
required libraries.

The build_win.sh script bundles the PacMan executable and all required 
DLLs in an obfuscated hex dump embedded in an HTML file which can 
actually be mailed by e.g. GMail. To install, simply double-click 
pacman_delivery.html and follow the on-screen instructions. To ensure 
a 100% clean and safe build, you are free to run build_win.sh yourself 
after inspecting the source code. 

**Additional information**
For more information on gameplay and AI rules, these are good sources: 
https://pacman30thanniversary.net/pacman-ghost-movement-pattern/ and 
https://aighost.co.uk/pac-man-ghost-ai-how-the-classic-games-enemies-think/ 

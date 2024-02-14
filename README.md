# Endless Sky

Explore other star systems. Earn money by trading, carrying passengers, or completing missions. Use your earnings to buy a better ship or to upgrade the weapons and engines on your current one. Blow up pirates. Take sides in a civil war. Or leave human space behind and hope to find some friendly aliens whose culture is more civilized than your own...

------

Endless Sky is a sandbox-style space exploration game similar to Elite, Escape Velocity, or Star Control. You start out as the captain of a tiny spaceship and can choose what to do from there. The game includes a major plot line and many minor missions, but you can choose whether you want to play through the plot or strike out on your own as a merchant or bounty hunter or explorer.

See the [player's manual](https://github.com/endless-sky/endless-sky/wiki/PlayersManual) for more information, or the [home page](https://endless-sky.github.io/) for screenshots and the occasional blog post.

## Installing the game

Official releases of Endless Sky are available as direct downloads from [GitHub](https://github.com/endless-sky/endless-sky/releases/latest), on [Steam](https://store.steampowered.com/app/404410/Endless_Sky/), and on [Flathub](https://flathub.org/apps/details/io.github.endless_sky.endless_sky). Other package managers may also include the game, though the specific version provided may not be up-to-date.

## System Requirements

Endless Sky has very minimal system requirements, meaning most systems should be able to run the game. The most restrictive requirement is likely that your device must support at least OpenGL 3.

|| Minimum | Recommended |
|---|----:|----:|
|RAM | 750 MB | 2 GB |
|Graphics | OpenGL 3.0 | OpenGL 3.3 |
|Storage Free | 350 MB | 1.5 GB |

## Building from source

Most development is done on Linux and Windows, using CMake ([build instructions](docs/readme-cmake.md)) to compile the project. For those wishing to use an IDE, project files are provided for [Code::Blocks](https://www.codeblocks.org/) to simplify the project setup, and other IDEs are supported through their respective CMake integration. [SCons](https://scons.org/) was the primary build tool up until 0.9.16, and some files and information continue to be available for it.
For full installation instructions, consult the [Build Instructions](docs/readme-developer.md) readme.

## Functionalities and Features 

Give a summary of the various software capabilites. 

## API Documentation

Give the highlights of the functions in various header files.

class AI: controls all the ships in the game
- void IssueShipTarget(const std::shared_ptr<Ship> &target); // Fleet commands from the player.
- void IssueAsteroidTarget(const std::shared_ptr<Minable> &targetAsteroid); // Fleet commands from the player.
- void IssueMoveTarget(const Point &target, const System *moveToSystem); // Fleet commands from the player.
- void UpdateKeys(PlayerInfo &player, Command &clickCommands); // Commands issued via the keyboard (mostly, to the flagship).
- void UpdateEvents(const std::list<ShipEvent> &events); // Allow the AI to track any events it is interested in.
- void Clean(); // Reset the AI's memory of events.
- void ClearOrders(); // Clear ship orders.
- void Step(Command &activeCommands); // Issue AI commands to all ships for one game step.
- void SetMousePosition(Point position); // Set the mouse position for turning the player's flagship.
- int64_t AllyStrength(const Government *government); // Get the in-system strength of each government's allies and enemies.
- int64_t EnemyStrength(const Government *government); // Get the in-system strength of each government's allies and enemies.	
- static const StellarObject *FindLandingLocation(const Ship &ship, const bool refuel = true); // Find nearest landing location.

## Video Demo and Troubleshooting

Demo Link:

Frequently Asked Questions: 

## Contributing

As a free and open source game, Endless Sky is the product of many people's work. Contributions of artwork, storylines, and other writing are most in-demand, though there is a loosely defined [roadmap](https://github.com/endless-sky/endless-sky/wiki/DevelopmentRoadmap). Those who wish to [contribute](docs/CONTRIBUTING.md) are encouraged to review the [wiki](https://github.com/endless-sky/endless-sky/wiki), and to post in the [community-run Discord](https://discord.gg/ZeuASSx) beforehand. Those who prefer to use Steam can use its [discussion rooms](https://steamcommunity.com/app/404410/discussions/) as well, or GitHub's [discussion zone](https://github.com/endless-sky/endless-sky/discussions).

Endless Sky's main discussion and development area was once [Google Groups](https://groups.google.com/g/endless-sky), but due to factors outside our control, it is now inaccessible to new users, and should not be used anymore.

## Licensing

Endless Sky is a free, open source game. The [source code](https://github.com/endless-sky/endless-sky/) is available under the GPL v3 license, and all the artwork is either public domain or released under a variety of Creative Commons (and similarly permissive) licenses. (To determine the copyright status of any of the artwork, consult the [copyright file](https://github.com/endless-sky/endless-sky/blob/master/copyright).)

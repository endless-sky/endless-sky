# Endless Sky - Delta

This is the Rapid Development and Experimentation fork of the Endless Sky project, and is colloquially known as "Endless Sky Delta" or "Revamp 2.0." The purpose is to be an area where more changes can be worked on, trialed, and experimented with.

If you are looking for a stable version of Endless Sky to play, you should go to https://github.com/endless-sky/endless-sky instead.
If you are looking to try out the cutting edge (and possible failures) of our attempts to make Endless Sky better, this is the place to be.

# Development

Look here to find a list of implemented features and to talk about them: https://github.com/endless-sky/Endless-Sky-Delta/discussions/15

This fork has a number of branches, two of which are our primary work environment.

Mirror : This is our base branch, and is kept synchronized with the main Endless Sky "master" branch.
- Main : This is our long-term branch that will collect all the changes we have tried out and feel should be a permanent part of the game. It is based on "Mirror."
- - Experimental : This is our experimental working area. All our changes start out here where they can be tested in conjunction with many other changes. It is based on "Main."

**All PRs suggesting changes should be made to the "Experimental" branch.**

------

Explore other star systems. Earn money by trading, carrying passengers, or completing missions. Use your earnings to buy a better ship or to upgrade the weapons and engines on your current one. Blow up pirates. Take sides in a civil war. Or leave human space behind and hope to find some friendly aliens whose culture is more civilized than your own...

------

Endless Sky is a sandbox-style space exploration game similar to Elite, Escape Velocity, or Star Control. You start out as the captain of a tiny spaceship and can choose what to do from there. The game includes a major plot line and many minor missions, but you can choose whether you want to play through the plot or strike out on your own as a merchant or bounty hunter or explorer.

See the [player's manual](https://github.com/endless-sky/endless-sky/wiki/PlayersManual) for more information, or the [home page](https://endless-sky.github.io/) for screenshots and the occasional blog post.

## Installing the game

Releases of Endless Sky Delta are available as direct downloads from [GitHub](https://github.com/endless-sky/endless-sky-delta/releases/latest), while the official vanilla releases are on [Steam](https://store.steampowered.com/app/404410/Endless_Sky/), on [GOG](https://gog.com/game/endless_sky), and on [Flathub](https://flathub.org/apps/details/io.github.endless_sky.endless_sky). Other package managers may also include the game, though the specific version provided may not be up-to-date.


## System Requirements

Endless Sky has very minimal system requirements, meaning most systems should be able to run the game. The most restrictive requirement is likely that your device must support at least OpenGL 3.

|| Minimum | Recommended |
|---|----:|----:|
|RAM | 750 MB | 2 GB |
|Graphics | OpenGL 3.0 | OpenGL 3.3 |
|Storage Free | 350 MB | 1.5 GB |

## Building from source

Development is done using [CMake](https://cmake.org) to compile the project. Most popular IDEs are supported through their respective CMake integration. [SCons](https://scons.org/) was the primary build tool up until 0.9.16, and some files and information continue to be available for it.

For full installation instructions, consult the [Build Instructions](docs/readme-cmake.md) readme.

## Contributing

As a free and open source game, Endless Sky is the product of many people's work. Contributions of artwork, storylines, and other writing are most in-demand, though there is a loosely defined [roadmap](https://github.com/endless-sky/endless-sky/wiki/DevelopmentRoadmap). Those who wish to [contribute](docs/CONTRIBUTING.md) are encouraged to review the [wiki](https://github.com/endless-sky/endless-sky/wiki). The best place to ask questions or discuss ideas is GitHub's [discussion zone](https://github.com/endless-sky/endless-sky-delta/discussions).

Endless Sky's main discussion and development area was once [Google Groups](https://groups.google.com/g/endless-sky), but due to factors outside our control, it is now inaccessible to new users, and should not be used anymore.

## Licensing

Endless Sky is a free, open source game. The [source code](https://github.com/endless-sky/endless-sky-delta/) is available under the GPL v3 license, and all the artwork is either public domain or released under a variety of Creative Commons (and similarly permissive) licenses. (To determine the copyright status of any of the artwork, consult the [copyright file](https://github.com/endless-sky/endless-sky-delta/blob/master/copyright).)

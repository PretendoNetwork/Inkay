# Inkay - Aroma patches for Pretendo

[![Pretendo network logo](https://github.com/PretendoNetwork/website/raw/master/public/assets/images/opengraph/opengraph-image.png)](https://pretendo.network)

Inkay is an Aroma/WUPS plugin that patches various Nintendo Network URLs on a Wii U to use Pretendo Network instead. It also (for the time being) bypasses SSL verification in most cases. It redirects Nintendo Network in:

- IOSU-side connections (Friends, SpotPass, accounts etc.)
- Account Settings
- NNCS
- Nintendo eShop
- Miiverse (in-game)
- Miiverse applet

Inkay also includes game-specific patches to add extra features:
- Modpack-specific matchmaking for global, regional rooms (by simulating extra DLC) - **Mario Kart 8**
- P2P port override for better connection stability (if you port forward) - **Minecraft: Wii U Edition**, **Mario Kart 8**, **Splatoon**

## Requirements
Inkay is only supported on the release version of Aroma configured for autoboot/coldboot. Other configurations (specifically lacking coldboot) may cause issues with SpotPass.

## Safety
Inkay's patches are all temporary, and only applied in-memory without modifying your console. The SSL patch, while also temporary, could reduce the overall security of your console while active - this is because it no longer checks if a server is verified. However, this does not apply to the Internet Browser, where SSL still works as expected.

## Compiling - Docker
Inkay's dependencies and build tooling can be handled as a container, which is recommended for WUPS plugins. Using `docker` or `podman`:
```shell
docker build -t inkay .
docker run --rm -v $(pwd):/app inkay make
# you can replace "make" with other commands - e.g. make clean
```
If using `podman` on SELinux systems (like Fedora Linux), you might need to use `$(pwd):/app:Z` instead of `$(pwd):/app`.

## Compiling - System
Inkay has the following dependencies aside from devkitPPC and wut:
- [WiiUPluginSystem](https://github.com/wiiu-env/WiiUPluginSystem)
- [WiiUModuleSystem](https://github.com/wiiu-env/WiiUModuleSystem)
- [libmocha](https://github.com/wiiu-env/libmocha)
- [libkernel](https://github.com/wiiu-env/libkernel/)
- [libnotifications](https://github.com/wiiu-env/libnotifications/)
- [libfunctionpatcher](https://github.com/wiiu-env/libfunctionpatcher)

Each of these should be `make install`-able. After that, you can compile Inkay with `make`.

## TODO
See [Issues](https://github.com/PretendoNetwork/Inkay/issues).

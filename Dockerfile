FROM ghcr.io/wiiu-env/devkitppc:20260504

COPY --from=ghcr.io/wiiu-env/libnotifications:20260404  /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libfunctionpatcher:20260331 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libkernel:20260331 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libmocha:20260331 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:20260418 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260503 /artifacts $DEVKITPRO

WORKDIR /app
CMD make -f Makefile -j$(nproc)

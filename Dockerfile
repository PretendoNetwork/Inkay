FROM ghcr.io/wiiu-env/devkitppc:20231112

COPY --from=ghcr.io/wiiu-env/libnotifications:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libfunctionpatcher:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libkernel:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libmocha:20231127 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20230719 /artifacts $DEVKITPRO

WORKDIR /app
CMD make -f Makefile -j$(nproc)
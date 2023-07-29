FROM ghcr.io/wiiu-env/devkitppc:20230417

COPY --from=wiiuenv/libnotifications:20230126 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libfunctionpatcher:20230621 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libkernel:20220724 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libmocha:20220903 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiupluginsystem:20230126 /artifacts $DEVKITPRO

WORKDIR /app
CMD make -f Makefile -j$(nproc)
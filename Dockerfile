FROM wiiuenv/devkitppc:20210414

COPY --from=wiiuenv/libkernel:20210407 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiupluginsystem:20210417 /artifacts $DEVKITPRO

WORKDIR /app
CMD make -f Makefile -j$(nproc)

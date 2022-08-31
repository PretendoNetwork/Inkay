FROM wiiuenv/devkitppc:20220806

COPY --from=wiiuenv/libkernel:20220724 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libmocha:20220831 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiupluginsystem:20220826 /artifacts $DEVKITPRO

WORKDIR /app
CMD make -f Makefile -j$(nproc)

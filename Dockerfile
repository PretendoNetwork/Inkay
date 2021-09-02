# image 2c1762987ca7, wut 7495a99
FROM wiiuwut/core:latest

# image 2644eca7f89d, Maschell/WiiUPluginSystem e9ea643, last known 0.1 build
COPY --from=wups/core:latest /opt/devkitpro/wups /opt/devkitpro/wups
COPY --from=wiiuwut/libutils:0.1 /artifacts $WUT_ROOT

WORKDIR /app
CMD make -j$(nproc)

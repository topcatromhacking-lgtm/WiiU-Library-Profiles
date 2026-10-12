FROM ghcr.io/wiiu-env/devkitppc:20260225 AS build
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts /opt/devkitpro
WORKDIR /project
COPY standalone_inventory_scanner/ standalone_inventory_scanner/
RUN make -C standalone_inventory_scanner && test -s standalone_inventory_scanner/wlp_inventory_scanner.wuhb
FROM scratch AS artifacts
COPY --from=build /project/standalone_inventory_scanner/wlp_inventory_scanner.wuhb /

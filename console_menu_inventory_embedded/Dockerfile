FROM ghcr.io/wiiu-env/devkitppc:20260225 AS build
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts /opt/devkitpro
WORKDIR /project
COPY console_menu_inventory_embedded/ console_menu_inventory_embedded/
RUN make -C console_menu_inventory_embedded && test -s console_menu_inventory_embedded/wiiu_library_profiles_v10_passive_inventory.wps
FROM scratch AS artifacts
COPY --from=build /project/console_menu_inventory_embedded/wiiu_library_profiles_v10_passive_inventory.wps /

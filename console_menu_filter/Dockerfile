# Exact pinned wiiu-env toolchain/WUPS module releases used upstream.
FROM ghcr.io/wiiu-env/devkitppc:20260225 AS build
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts /opt/devkitpro
WORKDIR /project
COPY console_menu_filter/ console_menu_filter/
RUN make -C console_menu_filter && test -s console_menu_filter/wiiu_library_profiles_five_filter.wps

FROM scratch AS artifacts
COPY --from=build /project/console_menu_filter/wiiu_library_profiles_five_filter.wps /

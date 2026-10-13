FROM ghcr.io/wiiu-env/devkitppc:20260225 AS build
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts /opt/devkitpro
WORKDIR /project
COPY console_menu_db_integrated/ console_menu_db_integrated/
RUN make -C console_menu_db_integrated && test -s console_menu_db_integrated/wiiu_library_profiles_v11_db_readonly.wps
FROM scratch AS artifacts
COPY --from=build /project/console_menu_db_integrated/wiiu_library_profiles_v11_db_readonly.wps /
